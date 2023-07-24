#include <gb/gb.h>
#include <stdlib.h>
// Graphics data
#include "gen/map.h"
#include "gen/sprite.h"

#define SPRITE_ID   0
#define TILE_SIZE   8
#define MAP_WIDTH   20
#define MAP_HEIGHT  18

// Global variables
uint8_t player_x = 8, player_y = 8;
uint8_t input = 0, previousInput = 0;
uint8_t visibilityMask[MAP_WIDTH * MAP_HEIGHT / 8] = {0};

inline void reveal_tile(uint8_t x, uint8_t y);

inline uint8_t get_map_tile_xy(uint8_t x, uint8_t y) {
    return map_map[y * MAP_WIDTH + x];
}

uint8_t bresenham(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    int16_t dx = abs(x1 - x0);
    int16_t sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs(y1 - y0);
    int16_t sy = y0 < y1 ? 1 : -1;
    int16_t error = dx + dy;
    int16_t e2;

    while (1) {
        //set_bkg_tile_xy(x0, y0, 1);
        //reveal_tile(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        if (get_map_tile_xy(x0, y0) == 1) {
            return FALSE;
        }

        e2 = 2 * error;
        if (e2 >= dy) {
            if (x0 == x1) break;
            error += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            if (y0 == y1) break;
            error += dx;
            y0 += sy;
        }
    }

    return TRUE;
}

inline void update_player() {
    move_sprite(SPRITE_ID, player_x * TILE_SIZE + 8, player_y * TILE_SIZE + 16);
}

inline uint8_t is_tile_visible(uint8_t x, uint8_t y) {
    uint16_t tileIndex = y * MAP_WIDTH + x;
    uint8_t byteIndex = tileIndex / 8;
    uint8_t bitIndex = tileIndex % 8;
    // tileIndex = 20
    // byteIndex = 20 / 8 = 2
    // bitIndex = 20 % 8 = 2
    // 0        1       2
    // 00000000 0001000 01000100
    return visibilityMask[byteIndex] & (1 << bitIndex);
}

inline void reveal_tile(uint8_t x, uint8_t y) {
    uint16_t tileIndex = y * MAP_WIDTH + x;
    uint8_t byteIndex = tileIndex / 8;
    uint8_t bitIndex = tileIndex % 8;
    visibilityMask[byteIndex] |= (1 << bitIndex);

    // Reveal in the tilemap
    set_bkg_tile_xy(x, y, map_map[y * MAP_WIDTH + x]);
}

void reveal_tiles_around_player() {
    uint8_t xStart = player_x - 1;
    uint8_t yStart = player_y - 1;

    for (uint8_t x = 0; x <= 2; x++) {
        for (uint8_t y = 0; y <= 2; y++) {
            reveal_tile(xStart + x, yStart + y);
        }
    }
}

void reveal_visible_tiles() {
    for (uint8_t x = 0; x < MAP_WIDTH; x++) {
        for (uint8_t y = 0; y < MAP_HEIGHT; y++) {
            if (is_tile_visible(x, y)) continue;
            if (bresenham(player_x, player_y, x, y)) {
                reveal_tile(x, y);
            }
            //bresenham(player_x, player_y, x, y);
        }
    }
}

void main() {
    // load up map graphics and display them
    set_bkg_data(0, map_TILE_COUNT, map_tiles);
    //set_bkg_tiles(0, 0, 20, 18, map_map);
    for (uint8_t x = 0; x < MAP_WIDTH; x++) {
        for (uint8_t y = 0; y < MAP_HEIGHT; y++) {
            if (bresenham(player_x, player_y, x, y)) {
                //set_bkg_tile_xy(x, y, map_map[y * MAP_WIDTH + x]);
                reveal_tile(x, y);
            } else {
                set_bkg_tile_xy(x, y, 2);
            }
        }
    }
    SHOW_BKG;

    // load up sprite data and display that
    set_sprite_data(0, sprite_TILE_COUNT, sprite_tiles);
    set_sprite_tile(SPRITE_ID, 0);
    update_player();
    SHOW_SPRITES;

    while (1) {
        wait_vbl_done();

        previousInput = input;
        input = joypad();

        // Basic movement for our sprite
        if (input & J_RIGHT && !(previousInput & J_RIGHT)
                && get_bkg_tile_xy(player_x+1, player_y) != 1) {
            player_x += 1;
            update_player();
            //reveal_tiles_around_player();
            reveal_visible_tiles();
        } else if (input & J_LEFT && !(previousInput & J_LEFT)
                && get_bkg_tile_xy(player_x-1, player_y) != 1) {
            player_x -= 1;
            update_player();
            //reveal_tiles_around_player();
            reveal_visible_tiles();
        } else if (input & J_UP && !(previousInput & J_UP)
                && get_bkg_tile_xy(player_x, player_y-1) != 1) {
            player_y -= 1;
            update_player();
            //reveal_tiles_around_player();
            reveal_visible_tiles();
        } else if (input & J_DOWN && !(previousInput & J_DOWN)
                && get_bkg_tile_xy(player_x, player_y+1) != 1) {
            player_y += 1;
            update_player();
            //reveal_tiles_around_player();
            reveal_visible_tiles();
        }
    }
}

