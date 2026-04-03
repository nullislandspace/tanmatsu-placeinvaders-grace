#ifndef GAME_H
#define GAME_H

#include <stdbool.h>
#include <stdint.h>

// Screen dimensions (as user sees them, after 270-degree rotation)
#define SCREEN_W 800
#define SCREEN_H 480

// Framebuffer dimensions (physical memory layout)
#define FB_STRIDE 480
#define FB_HEIGHT 800

// Sprite scaling factor
#define SPRITE_SCALE 3

// Alien grid
#define ALIEN_COLS  11
#define ALIEN_ROWS  5
#define ALIEN_COUNT (ALIEN_COLS * ALIEN_ROWS)

// Alien spacing (in screen pixels)
#define ALIEN_SPACING_X 55
#define ALIEN_SPACING_Y 45
#define ALIEN_START_X   100
#define ALIEN_START_Y   80

// Alien movement
#define ALIEN_STEP      2
#define ALIEN_DROP      16
#define ALIEN_EDGE_LEFT  20
#define ALIEN_EDGE_RIGHT (SCREEN_W - 20)

// Player constants
#define PLAYER_Y          430
#define PLAYER_SPEED      5
#define PLAYER_START_X    (SCREEN_W / 2 - 22)
#define PLAYER_LIVES      3
#define EXTRA_LIFE_SCORE  1500

// Bullet constants
#define MAX_PLAYER_BULLETS  7
#define PLAYER_BULLET_SPEED 8
#define ALIEN_BULLET_SPEED  4
#define MAX_ALIEN_BULLETS   3
#define ALIEN_BULLET_FAST   6

// Shield constants
#define NUM_SHIELDS    4
#define SHIELD_W       22
#define SHIELD_H       16
#define SHIELD_Y       370
#define SHIELD_SCALED_W (SHIELD_W * SPRITE_SCALE)
#define SHIELD_SCALED_H (SHIELD_H * SPRITE_SCALE)
#define SHIELD_ROW_BYTES ((SHIELD_SCALED_W + 7) / 8)

// Mystery ship
#define MYSTERY_Y          40
#define MYSTERY_SPEED      3
#define MYSTERY_MIN_ALIENS 8

// Scoring
#define SCORE_SQUID   30
#define SCORE_CRAB    20
#define SCORE_OCTOPUS 10

// Animation
#define DEATH_ANIM_FRAMES   40
#define LEVEL_CLEAR_FRAMES  60
#define EXPLOSION_FRAMES    4

// Game states
typedef enum {
    STATE_TITLE,
    STATE_PLAYING,
    STATE_PLAYER_DEATH,
    STATE_LEVEL_CLEAR,
    STATE_GAME_OVER
} game_state_t;

// Alien types
typedef enum {
    ALIEN_SQUID   = 0,
    ALIEN_CRAB    = 1,
    ALIEN_OCTOPUS = 2,
    ALIEN_TYPE_COUNT = 3
} alien_type_t;

// Sound effect indices
typedef enum {
    SOUND_SHOOT         = 0,
    SOUND_ALIEN_EXPLODE = 1,
    SOUND_PLAYER_DEATH  = 2,
    SOUND_MYSTERY_APPEAR = 3,
    SOUND_MYSTERY_HIT   = 4,
    SOUND_COUNT         = 5
} sound_id_t;

// Alien structure
typedef struct {
    int16_t x, y;
    bool alive;
    alien_type_t type;
    uint8_t exploding;  // Countdown frames for explosion anim (0 = not exploding)
} alien_t;

// Bullet structure
typedef struct {
    int16_t x, y;
    bool active;
} bullet_t;

// Shield structure
typedef struct {
    int16_t x, y;
    uint8_t bitmap[SHIELD_SCALED_H][SHIELD_ROW_BYTES];
} shield_t;

// Mystery ship structure
typedef struct {
    int16_t x;
    bool active;
    int8_t direction;
    uint16_t points;
} mystery_ship_t;

// Main game structure
typedef struct {
    game_state_t state;
    uint32_t score;
    uint32_t highscore;
    uint8_t lives;
    uint8_t level;
    bool extra_life_given;

    // Player
    int16_t player_x;
    bullet_t player_bullets[MAX_PLAYER_BULLETS];

    // Aliens
    alien_t aliens[ALIEN_ROWS][ALIEN_COLS];
    uint8_t aliens_alive;
    int8_t alien_dir;
    bool alien_drop;
    uint8_t alien_anim;
    int8_t sweep_row;
    uint8_t sweep_col;

    // Alien bullets
    bullet_t alien_bullets[MAX_ALIEN_BULLETS];
    uint8_t alien_fire_timer;

    // Shields
    shield_t shields[NUM_SHIELDS];

    // Mystery ship
    mystery_ship_t mystery;
    uint16_t mystery_timer;

    // Animation timer
    uint16_t anim_timer;

    // Frame counter
    uint32_t frame;

    // Sound triggers (set by game logic, cleared by audio task)
    volatile bool sound_trigger[SOUND_COUNT];
} game_t;

// Game functions
void game_init(game_t* game, uint32_t highscore);
void game_new_wave(game_t* game);
void game_update(game_t* game, bool left, bool right, bool fire);
void game_init_shields(game_t* game);

// Highscore functions
uint32_t highscore_load(void);
void highscore_save(uint32_t score);

#endif // GAME_H
