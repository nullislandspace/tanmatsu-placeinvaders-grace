#include "game.h"
#include "sprites.h"
#include "audio.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
#include <stdlib.h>

// Pseudo-random number (simple LCG)
static uint32_t rng_state = 12345;
static uint32_t game_rand(void) {
    rng_state = rng_state * 1103515245 + 12345;
    return (rng_state >> 16) & 0x7FFF;
}

void game_init_shields(game_t* game) {
    int shield_spacing = (SCREEN_W - NUM_SHIELDS * SHIELD_SCALED_W) / (NUM_SHIELDS + 1);
    for (int s = 0; s < NUM_SHIELDS; s++) {
        game->shields[s].x = shield_spacing + s * (SHIELD_SCALED_W + shield_spacing);
        game->shields[s].y = SHIELD_Y;
        // Build scaled bitmap from template
        memset(game->shields[s].bitmap, 0, sizeof(game->shields[s].bitmap));
        for (int row = 0; row < SHIELD_TEMPLATE_H; row++) {
            uint32_t bits = shield_template[row];
            for (int col = 0; col < SHIELD_TEMPLATE_W; col++) {
                if (bits & (1 << (SHIELD_TEMPLATE_W - 1 - col))) {
                    // Set SPRITE_SCALE x SPRITE_SCALE block in scaled bitmap
                    for (int sy = 0; sy < SPRITE_SCALE; sy++) {
                        for (int sx = 0; sx < SPRITE_SCALE; sx++) {
                            int px = col * SPRITE_SCALE + sx;
                            int py = row * SPRITE_SCALE + sy;
                            int byte_idx = px / 8;
                            int bit_idx = 7 - (px % 8);
                            game->shields[s].bitmap[py][byte_idx] |= (1 << bit_idx);
                        }
                    }
                }
            }
        }
    }
}

void game_new_wave(game_t* game) {
    game->aliens_alive = 0;
    game->alien_dir = 1;
    game->alien_drop = false;
    game->alien_anim = 0;
    game->sweep_row = ALIEN_ROWS - 1;
    game->sweep_col = 0;

    // Place aliens
    for (int row = 0; row < ALIEN_ROWS; row++) {
        alien_type_t type;
        if (row == 0) type = ALIEN_SQUID;
        else if (row <= 2) type = ALIEN_CRAB;
        else type = ALIEN_OCTOPUS;

        for (int col = 0; col < ALIEN_COLS; col++) {
            alien_t* a = &game->aliens[row][col];
            a->x = ALIEN_START_X + col * ALIEN_SPACING_X;
            a->y = ALIEN_START_Y + row * ALIEN_SPACING_Y;
            a->alive = true;
            a->type = type;
            a->exploding = 0;
            game->aliens_alive++;
        }
    }

    // Reset bullets
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) game->player_bullets[i].active = false;
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        game->alien_bullets[i].active = false;
    }
    game->alien_fire_timer = 60;

    // Reset mystery
    game->mystery.active = false;
    game->mystery_timer = 300 + (game_rand() % 300);

    // Reset shields
    game_init_shields(game);
}

void game_init(game_t* game, uint32_t highscore) {
    memset(game, 0, sizeof(game_t));
    game->state = STATE_PLAYING;
    game->score = 0;
    game->highscore = highscore;
    game->lives = PLAYER_LIVES;
    game->level = 1;
    game->extra_life_given = false;
    game->player_x = PLAYER_START_X;
    game->frame = 0;
    rng_state = (uint32_t)(highscore * 7 + 42);
    game_new_wave(game);
}

// Find the next alive alien in sweep order (bottom-to-top, left-to-right)
static bool sweep_next(game_t* game) {
    // Advance to next position
    game->sweep_col++;
    if (game->sweep_col >= ALIEN_COLS) {
        game->sweep_col = 0;
        game->sweep_row--;
        if (game->sweep_row < 0) {
            // Sweep complete - wrap around
            game->sweep_row = ALIEN_ROWS - 1;
            game->sweep_col = 0;
            game->alien_anim ^= 1;  // Toggle animation frame

            // Apply pending drop
            if (game->alien_drop) {
                for (int r = 0; r < ALIEN_ROWS; r++) {
                    for (int c = 0; c < ALIEN_COLS; c++) {
                        game->aliens[r][c].y += ALIEN_DROP;
                    }
                }
                game->alien_dir = -game->alien_dir;
                game->alien_drop = false;
            }
        }
    }

    // Search for next alive alien
    for (int pass = 0; pass < ALIEN_COUNT; pass++) {
        if (game->aliens[game->sweep_row][game->sweep_col].alive) {
            return true;
        }
        game->sweep_col++;
        if (game->sweep_col >= ALIEN_COLS) {
            game->sweep_col = 0;
            game->sweep_row--;
            if (game->sweep_row < 0) {
                game->sweep_row = ALIEN_ROWS - 1;
                game->alien_anim ^= 1;
                if (game->alien_drop) {
                    for (int r = 0; r < ALIEN_ROWS; r++) {
                        for (int c = 0; c < ALIEN_COLS; c++) {
                            game->aliens[r][c].y += ALIEN_DROP;
                        }
                    }
                    game->alien_dir = -game->alien_dir;
                    game->alien_drop = false;
                }
            }
        }
    }
    return false;  // No alive aliens
}

static void alien_movement(game_t* game) {
    if (game->aliens_alive == 0) return;

    if (!sweep_next(game)) return;

    alien_t* a = &game->aliens[game->sweep_row][game->sweep_col];
    a->x += game->alien_dir * ALIEN_STEP;

    // Check edge
    int sprite_w = alien_widths[a->type] * SPRITE_SCALE;
    if (a->x + sprite_w >= ALIEN_EDGE_RIGHT || a->x <= ALIEN_EDGE_LEFT) {
        game->alien_drop = true;
    }
}

static void alien_shooting(game_t* game) {
    game->alien_fire_timer--;
    if (game->alien_fire_timer > 0) return;

    // Reset timer (faster at higher scores)
    int base_rate = 40 - (game->score / 200);
    if (base_rate < 10) base_rate = 10;
    game->alien_fire_timer = base_rate + (game_rand() % 20);

    // Find a free bullet slot
    int slot = -1;
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (!game->alien_bullets[i].active) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return;

    // Pick a random column that has alive aliens
    int col = game_rand() % ALIEN_COLS;
    for (int attempt = 0; attempt < ALIEN_COLS; attempt++) {
        // Find bottom-most alive alien in this column
        for (int row = ALIEN_ROWS - 1; row >= 0; row--) {
            if (game->aliens[row][col].alive) {
                alien_t* a = &game->aliens[row][col];
                int sw = alien_widths[a->type] * SPRITE_SCALE;
                int sh = alien_heights[a->type] * SPRITE_SCALE;
                game->alien_bullets[slot].x = a->x + sw / 2;
                game->alien_bullets[slot].y = a->y + sh;
                game->alien_bullets[slot].active = true;
                return;
            }
        }
        col = (col + 1) % ALIEN_COLS;
    }
}

static void shield_damage(shield_t* shield, int hit_x, int hit_y, int radius) {
    int local_x = hit_x - shield->x;
    int local_y = hit_y - shield->y;
    for (int dy = -radius; dy <= radius; dy++) {
        for (int dx = -radius; dx <= radius; dx++) {
            if (dx * dx + dy * dy <= radius * radius) {
                int px = local_x + dx;
                int py = local_y + dy;
                if (px >= 0 && px < SHIELD_SCALED_W && py >= 0 && py < SHIELD_SCALED_H) {
                    int byte_idx = px / 8;
                    int bit_idx = 7 - (px % 8);
                    shield->bitmap[py][byte_idx] &= ~(1 << bit_idx);
                }
            }
        }
    }
}

static bool shield_pixel_test(const shield_t* shield, int screen_x, int screen_y) {
    int local_x = screen_x - shield->x;
    int local_y = screen_y - shield->y;
    if (local_x < 0 || local_x >= SHIELD_SCALED_W || local_y < 0 || local_y >= SHIELD_SCALED_H) {
        return false;
    }
    int byte_idx = local_x / 8;
    int bit_idx = 7 - (local_x % 8);
    return (shield->bitmap[local_y][byte_idx] & (1 << bit_idx)) != 0;
}

static void update_explosions(game_t* game) {
    for (int r = 0; r < ALIEN_ROWS; r++) {
        for (int c = 0; c < ALIEN_COLS; c++) {
            if (game->aliens[r][c].exploding > 0) {
                game->aliens[r][c].exploding--;
            }
        }
    }
}

static void check_bullet_vs_aliens(game_t* game) {
    for (int b = 0; b < MAX_PLAYER_BULLETS; b++) {
        if (!game->player_bullets[b].active) continue;

        int bx = game->player_bullets[b].x;
        int by = game->player_bullets[b].y;

        for (int row = 0; row < ALIEN_ROWS; row++) {
            for (int col = 0; col < ALIEN_COLS; col++) {
                alien_t* a = &game->aliens[row][col];
                if (!a->alive || a->exploding > 0) continue;

                int sw = alien_widths[a->type] * SPRITE_SCALE;
                int sh = alien_heights[a->type] * SPRITE_SCALE;

                if (bx + 3 >= a->x && bx <= a->x + sw &&
                    by + 12 >= a->y && by <= a->y + sh) {
                    a->alive = false;
                    a->exploding = EXPLOSION_FRAMES;
                    game->aliens_alive--;
                    game->score += alien_scores[a->type];
                    game->player_bullets[b].active = false;
                    audio_trigger(SOUND_ALIEN_EXPLODE);

                    if (!game->extra_life_given && game->score >= EXTRA_LIFE_SCORE) {
                        game->extra_life_given = true;
                        game->lives++;
                    }

                    if (game->aliens_alive == 0) {
                        game->state = STATE_LEVEL_CLEAR;
                        game->anim_timer = LEVEL_CLEAR_FRAMES;
                    }
                    goto next_bullet;
                }
            }
        }
        next_bullet:;
    }
}

static void check_bullet_vs_player(game_t* game) {
    int pw = SPRITE_CANNON_W * SPRITE_SCALE;
    int ph = SPRITE_CANNON_H * SPRITE_SCALE;

    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (!game->alien_bullets[i].active) continue;

        int bx = game->alien_bullets[i].x;
        int by = game->alien_bullets[i].y;

        if (bx + 3 >= game->player_x && bx <= game->player_x + pw &&
            by + 9 >= PLAYER_Y && by <= PLAYER_Y + ph) {
            // Player hit!
            game->alien_bullets[i].active = false;
            game->lives--;
            game->state = STATE_PLAYER_DEATH;
            game->anim_timer = DEATH_ANIM_FRAMES;
            audio_trigger(SOUND_PLAYER_DEATH);
            return;
        }
    }
}

static void check_bullet_vs_shields(game_t* game) {
    // Player bullets vs shields
    for (int b = 0; b < MAX_PLAYER_BULLETS; b++) {
        if (!game->player_bullets[b].active) continue;
        for (int s = 0; s < NUM_SHIELDS; s++) {
            for (int dy = 0; dy < 12; dy++) {
                for (int dx = 0; dx < 3; dx++) {
                    if (shield_pixel_test(&game->shields[s],
                            game->player_bullets[b].x + dx,
                            game->player_bullets[b].y + dy)) {
                        shield_damage(&game->shields[s],
                                      game->player_bullets[b].x + 1,
                                      game->player_bullets[b].y + dy, 4);
                        game->player_bullets[b].active = false;
                        goto next_player_bullet;
                    }
                }
            }
        }
        next_player_bullet:;
    }

    // Alien bullets vs shields
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (!game->alien_bullets[i].active) continue;
        for (int s = 0; s < NUM_SHIELDS; s++) {
            for (int dy = 0; dy < 9; dy++) {
                for (int dx = 0; dx < 3; dx++) {
                    if (shield_pixel_test(&game->shields[s],
                            game->alien_bullets[i].x + dx,
                            game->alien_bullets[i].y + dy)) {
                        shield_damage(&game->shields[s],
                                      game->alien_bullets[i].x + 1,
                                      game->alien_bullets[i].y + dy, 4);
                        game->alien_bullets[i].active = false;
                        goto next_alien_bullet;
                    }
                }
            }
        }
        next_alien_bullet:;
    }
}

static void check_bullet_vs_mystery(game_t* game) {
    if (!game->mystery.active) return;

    int mw = SPRITE_MYSTERY_W * SPRITE_SCALE;
    int mh = SPRITE_MYSTERY_H * SPRITE_SCALE;

    for (int b = 0; b < MAX_PLAYER_BULLETS; b++) {
        if (!game->player_bullets[b].active) continue;

        int bx = game->player_bullets[b].x;
        int by = game->player_bullets[b].y;

        if (bx + 3 >= game->mystery.x && bx <= game->mystery.x + mw &&
            by + 12 >= MYSTERY_Y && by <= MYSTERY_Y + mh) {
            game->score += game->mystery.points;
            game->mystery.active = false;
            game->player_bullets[b].active = false;
            audio_trigger(SOUND_MYSTERY_HIT);
            return;
        }
    }
}

static void check_aliens_vs_shields(game_t* game) {
    for (int row = 0; row < ALIEN_ROWS; row++) {
        for (int col = 0; col < ALIEN_COLS; col++) {
            alien_t* a = &game->aliens[row][col];
            if (!a->alive) continue;

            int sw = alien_widths[a->type] * SPRITE_SCALE;
            int sh = alien_heights[a->type] * SPRITE_SCALE;

            for (int s = 0; s < NUM_SHIELDS; s++) {
                shield_t* shield = &game->shields[s];
                // Check overlap
                if (a->x + sw > shield->x && a->x < shield->x + SHIELD_SCALED_W &&
                    a->y + sh > shield->y && a->y < shield->y + SHIELD_SCALED_H) {
                    // Erase overlapping shield pixels
                    int ox1 = (a->x > shield->x) ? a->x - shield->x : 0;
                    int oy1 = (a->y > shield->y) ? a->y - shield->y : 0;
                    int ox2 = ((a->x + sw) < (shield->x + SHIELD_SCALED_W)) ?
                              (a->x + sw - shield->x) : SHIELD_SCALED_W;
                    int oy2 = ((a->y + sh) < (shield->y + SHIELD_SCALED_H)) ?
                              (a->y + sh - shield->y) : SHIELD_SCALED_H;

                    for (int py = oy1; py < oy2; py++) {
                        for (int px = ox1; px < ox2; px++) {
                            int byte_idx = px / 8;
                            int bit_idx = 7 - (px % 8);
                            shield->bitmap[py][byte_idx] &= ~(1 << bit_idx);
                        }
                    }
                }
            }
        }
    }
}

static void check_aliens_reach_bottom(game_t* game) {
    for (int row = 0; row < ALIEN_ROWS; row++) {
        for (int col = 0; col < ALIEN_COLS; col++) {
            if (game->aliens[row][col].alive) {
                int sh = alien_heights[game->aliens[row][col].type] * SPRITE_SCALE;
                if (game->aliens[row][col].y + sh >= PLAYER_Y) {
                    game->lives = 0;
                    game->state = STATE_GAME_OVER;
                    if (game->score > game->highscore) {
                        game->highscore = game->score;
                        highscore_save(game->score);
                    }
                    audio_trigger(SOUND_PLAYER_DEATH);
                    return;
                }
            }
        }
    }
}

static void mystery_update(game_t* game) {
    if (game->mystery.active) {
        game->mystery.x += game->mystery.direction * MYSTERY_SPEED;
        int mw = SPRITE_MYSTERY_W * SPRITE_SCALE;
        if (game->mystery.x > SCREEN_W || game->mystery.x + mw < 0) {
            game->mystery.active = false;
        }
    } else {
        if (game->aliens_alive >= MYSTERY_MIN_ALIENS) {
            game->mystery_timer--;
            if (game->mystery_timer == 0) {
                game->mystery.active = true;
                static const uint16_t mystery_points[] = { 50, 100, 150, 300 };
                game->mystery.points = mystery_points[game_rand() % 4];
                if (game_rand() % 2) {
                    game->mystery.x = -SPRITE_MYSTERY_W * SPRITE_SCALE;
                    game->mystery.direction = 1;
                } else {
                    game->mystery.x = SCREEN_W;
                    game->mystery.direction = -1;
                }
                game->mystery_timer = 300 + (game_rand() % 300);
                audio_trigger(SOUND_MYSTERY_APPEAR);
            }
        }
    }
}

void game_update(game_t* game, bool left, bool right, bool fire) {
    game->frame++;
    rng_state ^= game->frame;

    // Player movement
    if (left) {
        game->player_x -= PLAYER_SPEED;
        if (game->player_x < 10) game->player_x = 10;
    }
    if (right) {
        game->player_x += PLAYER_SPEED;
        int max_x = SCREEN_W - SPRITE_CANNON_W * SPRITE_SCALE - 10;
        if (game->player_x > max_x) game->player_x = max_x;
    }

    // Player fire - find a free bullet slot
    if (fire) {
        for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
            if (!game->player_bullets[i].active) {
                game->player_bullets[i].active = true;
                game->player_bullets[i].x = game->player_x + (SPRITE_CANNON_W * SPRITE_SCALE) / 2 - 1;
                game->player_bullets[i].y = PLAYER_Y - 12;
                audio_trigger(SOUND_SHOOT);
                break;
            }
        }
    }

    // Move player bullets
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (game->player_bullets[i].active) {
            game->player_bullets[i].y -= PLAYER_BULLET_SPEED;
            if (game->player_bullets[i].y < 0) {
                game->player_bullets[i].active = false;
            }
        }
    }

    // Move alien bullets
    int bullet_speed = (game->aliens_alive <= 8) ? ALIEN_BULLET_FAST : ALIEN_BULLET_SPEED;
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (game->alien_bullets[i].active) {
            game->alien_bullets[i].y += bullet_speed;
            if (game->alien_bullets[i].y > SCREEN_H) {
                game->alien_bullets[i].active = false;
            }
        }
    }

    // Alien movement
    alien_movement(game);

    // Alien shooting
    alien_shooting(game);

    // Mystery ship
    mystery_update(game);

    // Update explosion animations
    update_explosions(game);

    // Collision detection
    check_bullet_vs_aliens(game);
    check_bullet_vs_mystery(game);
    check_bullet_vs_shields(game);
    check_bullet_vs_player(game);
    check_aliens_vs_shields(game);
    check_aliens_reach_bottom(game);
}

// --- NVS Highscore ---

uint32_t highscore_load(void) {
    nvs_handle_t handle;
    uint32_t score = 0;
    if (nvs_open("placeinvaders", NVS_READONLY, &handle) == ESP_OK) {
        nvs_get_u32(handle, "highscore", &score);
        nvs_close(handle);
    }
    return score;
}

void highscore_save(uint32_t score) {
    nvs_handle_t handle;
    if (nvs_open("placeinvaders", NVS_READWRITE, &handle) == ESP_OK) {
        nvs_set_u32(handle, "highscore", score);
        nvs_commit(handle);
        nvs_close(handle);
    }
}
