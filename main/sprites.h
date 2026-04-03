#ifndef SPRITES_H
#define SPRITES_H

#include <stdint.h>

// Sprite data: 1-bit bitmaps, each row is a uint16_t.
// MSB = leftmost pixel within the sprite width.

// --- Squid (30 pts) - 8x8 ---
#define SPRITE_SQUID_W 8
#define SPRITE_SQUID_H 8

static const uint16_t sprite_squid_1[8] = {
    0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x5A, 0x81, 0x42,
};
static const uint16_t sprite_squid_2[8] = {
    0x18, 0x3C, 0x7E, 0xDB, 0xFF, 0x24, 0x5A, 0xA5,
};

// --- Crab (20 pts) - 11x8 ---
#define SPRITE_CRAB_W 11
#define SPRITE_CRAB_H 8

static const uint16_t sprite_crab_1[8] = {
    0x104, 0x088, 0x1FC, 0x376, 0x7FF, 0x5FD, 0x505, 0x0D8,
};
static const uint16_t sprite_crab_2[8] = {
    0x104, 0x489, 0x5FD, 0x777, 0x7FF, 0x3FE, 0x104, 0x202,
};

// --- Octopus (10 pts) - 12x8 ---
#define SPRITE_OCTOPUS_W 12
#define SPRITE_OCTOPUS_H 8

static const uint16_t sprite_octopus_1[8] = {
    0x0F0, 0x7FE, 0xFFF, 0xE67, 0xFFF, 0x39C, 0x6F6, 0xC03,
};
static const uint16_t sprite_octopus_2[8] = {
    0x0F0, 0x7FE, 0xFFF, 0xE67, 0xFFF, 0x59A, 0xA05, 0x50A,
};

// --- Player cannon - 15x8 ---
#define SPRITE_CANNON_W 15
#define SPRITE_CANNON_H 8

static const uint16_t sprite_cannon[8] = {
    0x0080, 0x01C0, 0x01C0, 0x3FFE, 0x7FFF, 0x7FFF, 0x7FFF, 0x7FFF,
};

// --- Explosion - 13x8 ---
#define SPRITE_EXPLOSION_W 13
#define SPRITE_EXPLOSION_H 8

static const uint16_t sprite_explosion[8] = {
    0x0888, 0x0490, 0x01F0, 0x0BFA, 0x01F0, 0x0490, 0x0888, 0x0000,
};

// --- Mystery ship - 16x7 ---
#define SPRITE_MYSTERY_W 16
#define SPRITE_MYSTERY_H 7

static const uint16_t sprite_mystery[7] = {
    0x07E0, 0x1FF8, 0x3FFC, 0x6DB6, 0xFFFF, 0x39CE, 0x1008,
};

// --- Shield template - 22x16 (1 bit per pixel, stored in uint32_t) ---
// Bit 21 = leftmost pixel, bit 0 = rightmost pixel
#define SHIELD_TEMPLATE_W 22
#define SHIELD_TEMPLATE_H 16

static const uint32_t shield_template[16] = {
    0x07FFF8,
    0x0FFFFC,
    0x1FFFFE,
    0x3FFFFF,
    0x3FFFFF,
    0x3FFFFF,
    0x3FFFFF,
    0x3FFFFF,
    0x3FFFFF,
    0x3FFFFF,
    0x3FFFFF,
    0x3F807F,
    0x3F003F,
    0x3E001F,
    0x3C000F,
    0x380007,
};

// Lookup tables for alien sprites by type and frame
static const uint16_t* alien_sprites[3][2] = {
    { sprite_squid_1,   sprite_squid_2 },
    { sprite_crab_1,    sprite_crab_2 },
    { sprite_octopus_1, sprite_octopus_2 },
};

static const uint8_t alien_widths[3]  = { SPRITE_SQUID_W, SPRITE_CRAB_W, SPRITE_OCTOPUS_W };
static const uint8_t alien_heights[3] = { SPRITE_SQUID_H, SPRITE_CRAB_H, SPRITE_OCTOPUS_H };
static const uint8_t alien_scores[3]  = { 30, 20, 10 };

#endif // SPRITES_H
