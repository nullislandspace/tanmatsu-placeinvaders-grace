#include "render.h"
#include "hershey_font.h"
#include "sprites.h"
#include <string.h>
#include <stdio.h>
#include <inttypes.h>

void render_clear(uint8_t* fb) {
    memset(fb, 0, FB_STRIDE * FB_HEIGHT * 3);
}

void render_sprite(uint8_t* fb, const uint16_t* sprite, int width, int height,
                   int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    for (int row = 0; row < height; row++) {
        uint16_t bits = sprite[row];
        for (int col = 0; col < width; col++) {
            if (bits & (1 << (width - 1 - col))) {
                // Draw a SPRITE_SCALE x SPRITE_SCALE block
                for (int sy = 0; sy < SPRITE_SCALE; sy++) {
                    for (int sx = 0; sx < SPRITE_SCALE; sx++) {
                        hershey_set_pixel(fb, FB_STRIDE, FB_HEIGHT,
                                          x + col * SPRITE_SCALE + sx,
                                          y + row * SPRITE_SCALE + sy,
                                          r, g, b);
                    }
                }
            }
        }
    }
}

void render_rect(uint8_t* fb, int x, int y, int w, int h,
                 uint8_t r, uint8_t g, uint8_t b) {
    for (int py = y; py < y + h; py++) {
        for (int px = x; px < x + w; px++) {
            hershey_set_pixel(fb, FB_STRIDE, FB_HEIGHT, px, py, r, g, b);
        }
    }
}

void render_shield(uint8_t* fb, const shield_t* shield) {
    for (int row = 0; row < SHIELD_SCALED_H; row++) {
        for (int col = 0; col < SHIELD_SCALED_W; col++) {
            int byte_idx = col / 8;
            int bit_idx = 7 - (col % 8);
            if (shield->bitmap[row][byte_idx] & (1 << bit_idx)) {
                hershey_set_pixel(fb, FB_STRIDE, FB_HEIGHT,
                                  shield->x + col, shield->y + row,
                                  0x00, 0xFF, 0x00);
            }
        }
    }
}

static void render_hud(uint8_t* fb, const game_t* game) {
    char buf[32];

    // Score (left)
    snprintf(buf, sizeof(buf), "SCORE %05" PRIu32, game->score);
    hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT, 10, 2, buf, 20, 0xFF, 0xFF, 0xFF);

    // Title (center)
    const char* title = "PLACE INVADERS";
    int tw = hershey_string_width(title, 18);
    hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT, (SCREEN_W - tw) / 2, 3, title, 18,
                        0x00, 0xFF, 0x00);

    // Hi-Score (right)
    snprintf(buf, sizeof(buf), "HI %05" PRIu32, game->highscore);
    int hw = hershey_string_width(buf, 20);
    hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT, SCREEN_W - hw - 10, 2, buf, 20,
                        0xFF, 0xFF, 0xFF);

    // Lives (bottom-left of HUD area)
    for (int i = 0; i < game->lives; i++) {
        render_sprite(fb, sprite_cannon, SPRITE_CANNON_W, SPRITE_CANNON_H,
                      10 + i * (SPRITE_CANNON_W * SPRITE_SCALE + 8), SCREEN_H - 28,
                      0x00, 0xFF, 0x00);
    }

    // Ground line
    render_rect(fb, 0, 455, SCREEN_W, 2, 0x00, 0xFF, 0x00);
}

void render_title_screen(uint8_t* fb, const game_t* game) {
    render_clear(fb);

    // Title
    const char* title = "PLACE INVADERS";
    int tw = hershey_string_width(title, 48);
    hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT, (SCREEN_W - tw) / 2, 80, title, 48,
                        0x00, 0xFF, 0x00);

    // Alien score table
    int table_y = 180;
    int table_x = SCREEN_W / 2 - 140;

    render_sprite(fb, sprite_squid_1, SPRITE_SQUID_W, SPRITE_SQUID_H,
                  table_x, table_y, 0xFF, 0xFF, 0xFF);
    hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT,
                        table_x + SPRITE_SQUID_W * SPRITE_SCALE + 20, table_y + 4,
                        "= 30 POINTS", 20, 0xFF, 0xFF, 0xFF);

    table_y += 40;
    render_sprite(fb, sprite_crab_1, SPRITE_CRAB_W, SPRITE_CRAB_H,
                  table_x, table_y, 0xFF, 0xFF, 0xFF);
    hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT,
                        table_x + SPRITE_CRAB_W * SPRITE_SCALE + 20, table_y + 4,
                        "= 20 POINTS", 20, 0xFF, 0xFF, 0xFF);

    table_y += 40;
    render_sprite(fb, sprite_octopus_1, SPRITE_OCTOPUS_W, SPRITE_OCTOPUS_H,
                  table_x, table_y, 0xFF, 0xFF, 0xFF);
    hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT,
                        table_x + SPRITE_OCTOPUS_W * SPRITE_SCALE + 20, table_y + 4,
                        "= 10 POINTS", 20, 0xFF, 0xFF, 0xFF);

    table_y += 40;
    render_sprite(fb, sprite_mystery, SPRITE_MYSTERY_W, SPRITE_MYSTERY_H,
                  table_x, table_y, 0xFF, 0x00, 0x00);
    hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT,
                        table_x + SPRITE_MYSTERY_W * SPRITE_SCALE + 20, table_y + 4,
                        "= ??? POINTS", 20, 0xFF, 0xFF, 0xFF);

    // High score
    char buf[32];
    snprintf(buf, sizeof(buf), "HIGH SCORE: %05" PRIu32, game->highscore);
    int hw = hershey_string_width(buf, 24);
    hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT, (SCREEN_W - hw) / 2, 380, buf, 24,
                        0xFF, 0xFF, 0x00);

    // Prompt
    const char* prompt = "PRESS SPACE TO START";
    int pw = hershey_string_width(prompt, 22);
    // Blink the prompt
    if ((game->frame / 20) % 2 == 0) {
        hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT, (SCREEN_W - pw) / 2, 420, prompt, 22,
                            0xFF, 0xFF, 0xFF);
    }
}

void render_game(uint8_t* fb, const game_t* game) {
    render_clear(fb);
    render_hud(fb, game);

    // Draw shields
    for (int i = 0; i < NUM_SHIELDS; i++) {
        render_shield(fb, &game->shields[i]);
    }

    // Draw aliens
    for (int row = 0; row < ALIEN_ROWS; row++) {
        for (int col = 0; col < ALIEN_COLS; col++) {
            const alien_t* a = &game->aliens[row][col];
            if (a->exploding > 0) {
                render_sprite(fb, sprite_explosion, SPRITE_EXPLOSION_W, SPRITE_EXPLOSION_H,
                              a->x, a->y, 0xFF, 0xFF, 0xFF);
            } else if (a->alive) {
                const uint16_t* spr = alien_sprites[a->type][game->alien_anim];
                uint8_t w = alien_widths[a->type];
                uint8_t h = alien_heights[a->type];
                // Color by type
                uint8_t r, g, b;
                switch (a->type) {
                    case ALIEN_SQUID:   r = 0xFF; g = 0x00; b = 0xFF; break;
                    case ALIEN_CRAB:    r = 0x00; g = 0xFF; b = 0xFF; break;
                    case ALIEN_OCTOPUS: r = 0x00; g = 0xFF; b = 0x00; break;
                    default:            r = 0xFF; g = 0xFF; b = 0xFF; break;
                }
                render_sprite(fb, spr, w, h, a->x, a->y, r, g, b);
            }
        }
    }

    // Draw mystery ship
    if (game->mystery.active) {
        render_sprite(fb, sprite_mystery, SPRITE_MYSTERY_W, SPRITE_MYSTERY_H,
                      game->mystery.x, MYSTERY_Y, 0xFF, 0x00, 0x00);
    }

    // Draw player (not during death animation)
    if (game->state != STATE_PLAYER_DEATH || (game->anim_timer % 8 < 4)) {
        render_sprite(fb, sprite_cannon, SPRITE_CANNON_W, SPRITE_CANNON_H,
                      game->player_x, PLAYER_Y, 0x00, 0xFF, 0x00);
    }

    // Draw player bullets
    for (int i = 0; i < MAX_PLAYER_BULLETS; i++) {
        if (game->player_bullets[i].active) {
            render_rect(fb, game->player_bullets[i].x, game->player_bullets[i].y,
                        3, 12, 0xFF, 0xFF, 0xFF);
        }
    }

    // Draw alien bullets
    for (int i = 0; i < MAX_ALIEN_BULLETS; i++) {
        if (game->alien_bullets[i].active) {
            render_rect(fb, game->alien_bullets[i].x, game->alien_bullets[i].y,
                        3, 9, 0xFF, 0xFF, 0x00);
        }
    }
}

void render_game_over(uint8_t* fb, const game_t* game) {
    render_clear(fb);

    const char* go = "GAME OVER";
    int gw = hershey_string_width(go, 48);
    hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT, (SCREEN_W - gw) / 2, 120, go, 48,
                        0xFF, 0x00, 0x00);

    char buf[32];
    snprintf(buf, sizeof(buf), "SCORE: %05" PRIu32, game->score);
    int sw = hershey_string_width(buf, 32);
    hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT, (SCREEN_W - sw) / 2, 220, buf, 32,
                        0xFF, 0xFF, 0xFF);

    snprintf(buf, sizeof(buf), "HIGH SCORE: %05" PRIu32, game->highscore);
    int hw = hershey_string_width(buf, 28);
    hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT, (SCREEN_W - hw) / 2, 280, buf, 28,
                        0xFF, 0xFF, 0x00);

    if (game->score >= game->highscore && game->score > 0) {
        const char* nr = "NEW RECORD!";
        int nw = hershey_string_width(nr, 28);
        hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT, (SCREEN_W - nw) / 2, 330, nr, 28,
                            0x00, 0xFF, 0x00);
    }

    const char* prompt = "PRESS SPACE TO CONTINUE";
    int pw = hershey_string_width(prompt, 22);
    if ((game->frame / 20) % 2 == 0) {
        hershey_draw_string(fb, FB_STRIDE, FB_HEIGHT, (SCREEN_W - pw) / 2, 400, prompt, 22,
                            0xFF, 0xFF, 0xFF);
    }
}
