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

inline uint8_t bresenham(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    int8_t dx = abs(x1 - x0);
    int8_t sx = x0 < x1 ? 1 : -1;
    int8_t dy = -abs(y1 - y0);
    int8_t sy = y0 < y1 ? 1 : -1;
    int8_t error = dx + dy;
    int8_t e2;

    while (1) {
        //set_bkg_tile_xy(x0, y0, 1);
        reveal_tile(x0, y0);
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

uint8_t cast_x = 0, cast_y = 0;
inline void cast(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    int8_t dx = abs(x1 - x0);
    int8_t sx = x0 < x1 ? 1 : -1;
    int8_t dy = -abs(y1 - y0);
    int8_t sy = y0 < y1 ? 1 : -1;
    int8_t error = dx + dy;
    int8_t e2;

    cast_x = x0;
    cast_y = y0;

    while (1) {
        //set_bkg_tile_xy(x0, y0, 1);
        reveal_tile(cast_x, cast_y);
        //if (x0 == x1 && y0 == y1) break;
        if (get_map_tile_xy(cast_x, cast_y) == 1) {
            return;
        }

        e2 = 2 * error;
        if (e2 >= dy) {
            error += dy;
            cast_x += sx;
        }
        if (e2 <= dx) {
            error += dx;
            cast_y += sy;
        }
    }
}

inline void cast_cardinal(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    int8_t dx = x0 == x1
        ? 0
        : x0 < x1 ? 1 : -1;
    int8_t dy = y0 == y1
        ? 0
        : y0 < y1 ? 1 : -1;

    cast_x = x0;
    cast_y = y0;

    while (1) {
        reveal_tile(cast_x, cast_y);
        if (get_map_tile_xy(cast_x, cast_y) == 1) {
            return;
        }
        cast_x += dx;
        cast_y += dy;
    }
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
    
    if ((visibilityMask[byteIndex] &= (1 << bitIndex)) == 0) {
        set_bkg_tile_xy(x, y, map_map[y * MAP_WIDTH + x]);
    }

    visibilityMask[byteIndex] |= (1 << bitIndex);

    // Reveal in the tilemap
    //set_bkg_tile_xy(x, y, map_map[y * MAP_WIDTH + x]);
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

void reveal_tiles_along_axes() {
    // Up until we reach a wall
    uint8_t d = 0;
    do {
        d++;
        reveal_tile(player_x - 1, player_y + d);
        reveal_tile(player_x, player_y + d);
        reveal_tile(player_x + 1, player_y + d);
    } while (get_map_tile_xy(player_x, player_y + d) != 1);
    // Down until we reach a wall
    d = 0;
    do {
        d++;
        reveal_tile(player_x - 1, player_y - d);
        reveal_tile(player_x, player_y - d);
        reveal_tile(player_x + 1, player_y - d);
    } while (get_map_tile_xy(player_x, player_y - d) != 1);
    // Left until we reach a wall
    d = 0;
    do {
        d++;
        reveal_tile(player_x - d, player_y - 1);
        reveal_tile(player_x - d, player_y);
        reveal_tile(player_x - d, player_y + 1);
    } while (get_map_tile_xy(player_x - d, player_y) != 1);
    // Right until we reach a wall
    d = 0;
    do {
        d++;
        reveal_tile(player_x + d, player_y - 1);
        reveal_tile(player_x + d, player_y);
        reveal_tile(player_x + d, player_y + 1);
    } while (get_map_tile_xy(player_x + d, player_y) != 1);
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

void reveal_visible_tiles2() {
    // Possible bug: I think it is possible for `player_x - dy/2-1` to underflow, causing some tiles to go unrevealed
    // Up
    cast_cardinal(player_x, player_y, player_x, player_y - 1);
    uint8_t dy = player_y - cast_y;
    for (uint8_t x = player_x - dy/2-1; x <= player_x + dy/2+1; x++) {
        cast(player_x, player_y, x, player_y - dy);
    }
    // Down
    cast_cardinal(player_x, player_y, player_x, player_y + 1);
    dy = cast_y - player_y;
    for (uint8_t x = player_x - dy/2-1; x <= player_x + dy/2+1; x++) {
        cast(player_x, player_y, x, player_y + dy);
    }
    // Left
    cast_cardinal(player_x, player_y, player_x - 1, player_y);
    uint8_t dx = player_x - cast_x;
    for (uint8_t y = player_y - dx/2-1; y <= player_y + dx/2+1; y++) {
        cast(player_x, player_y, player_x - dx, y);
    }
    // Right
    cast_cardinal(player_x, player_y, player_x + 1, player_y);
    dx = cast_x - player_x;
    for (uint8_t y = player_y - dx/2-1; y <= player_y + dx/2+1; y++) {
        cast(player_x, player_y, player_x + dx, y);
    }
}

void main() {
    // load up map graphics and display them
    set_bkg_data(0, map_TILE_COUNT, map_tiles);
    //set_bkg_tiles(0, 0, 20, 18, map_map);
    for (uint8_t x = 0; x < MAP_WIDTH; x++) {
        for (uint8_t y = 0; y < MAP_HEIGHT; y++) {
            //if (bresenham(player_x, player_y, x, y)) {
                //set_bkg_tile_xy(x, y, map_map[y * MAP_WIDTH + x]);
                //reveal_tile(x, y);
            //} else {
                set_bkg_tile_xy(x, y, 2);
            //}
        }
    }
    SHOW_BKG;
    reveal_visible_tiles2();

    // load up sprite data and display that
    set_sprite_data(0, sprite_TILE_COUNT, sprite_tiles);
    set_sprite_tile(SPRITE_ID, 0);
    update_player();
    SHOW_SPRITES;

    while (1) {
        wait_vbl_done();

        previousInput = input;
        input = joypad();

        // Testing code: run 10 bresenhams to check CPU usage
        //for (uint8_t i = 0; i < 10; i++) { bresenham(0, 0, 19, 17); }
        //bresenham(0, 0, 19, 17);
        //bresenham(0, 0, 19, 17);

        // Basic movement for our sprite
        if (input & J_RIGHT //&& !(previousInput & J_RIGHT)
                && get_map_tile_xy(player_x+1, player_y) != 1) {
            player_x += 1;
            update_player();
            //reveal_tiles_around_player();
            //reveal_tiles_along_axes();
            reveal_visible_tiles2();
        } else if (input & J_LEFT //&& !(previousInput & J_LEFT)
                && get_map_tile_xy(player_x-1, player_y) != 1) {
            player_x -= 1;
            update_player();
            //reveal_tiles_around_player();
            //reveal_tiles_along_axes();
            reveal_visible_tiles2();
        } else if (input & J_UP //&& !(previousInput & J_UP)
                && get_map_tile_xy(player_x, player_y-1) != 1) {
            player_y -= 1;
            update_player();
            //reveal_tiles_around_player();
            //reveal_tiles_along_axes();
            reveal_visible_tiles2();
        } else if (input & J_DOWN //&& !(previousInput & J_DOWN)
                && get_map_tile_xy(player_x, player_y+1) != 1) {
            player_y += 1;
            update_player();
            //reveal_tiles_around_player();
            //reveal_tiles_along_axes();
            reveal_visible_tiles2();
        }
    }
}

