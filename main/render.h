#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>
#include "game.h"

// Clear framebuffer to black
void render_clear(uint8_t* fb);

// Draw a scaled 1-bit sprite at screen position (x, y) with given color
void render_sprite(uint8_t* fb, const uint16_t* sprite, int width, int height,
                   int x, int y, uint8_t r, uint8_t g, uint8_t b);

// Draw a filled rectangle at screen position
void render_rect(uint8_t* fb, int x, int y, int w, int h,
                 uint8_t r, uint8_t g, uint8_t b);

// Draw a shield from its bitmap
void render_shield(uint8_t* fb, const shield_t* shield);

// Draw the title screen
void render_title_screen(uint8_t* fb, const game_t* game);

// Draw the gameplay screen (HUD + game objects)
void render_game(uint8_t* fb, const game_t* game);

// Draw the game-over overlay
void render_game_over(uint8_t* fb, const game_t* game);

#endif // RENDER_H
