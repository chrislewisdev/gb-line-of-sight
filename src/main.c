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

// Implementation of Bresenham's algorithm, mostly copied from Wikipedia's pseudo-code.
// This version is actually not used in favour of cast() below, but kept for reference.
// Returns true or false based on whether or not the target tile is blocked or not.
inline uint8_t bresenham(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    int8_t dx = abs(x1 - x0);
    int8_t sx = x0 < x1 ? 1 : -1;
    int8_t dy = -abs(y1 - y0);
    int8_t sy = y0 < y1 ? 1 : -1;
    int8_t error = dx + dy;
    int8_t e2;

    while (1) {
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

// A version of bresenham that keeps going until it hits a wall, revealing tiles along the way
// Once complete, the final co-ordinates are available in cast_x/cast_y.
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

// A cast implementation for going straight up/down/left/right.
// Could make some optimisations around reducing arguments here, and just provide dx/dy values directly.
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

// A naive approach to scan all unrevealed tiles in the map for visibility.
void reveal_visible_tiles() {
    for (uint8_t x = 0; x < MAP_WIDTH; x++) {
        for (uint8_t y = 0; y < MAP_HEIGHT; y++) {
            if (is_tile_visible(x, y)) continue;
            if (bresenham(player_x, player_y, x, y)) {
                reveal_tile(x, y);
            }
        }
    }
}

// A more optimised approach that tries to minimise the amount of casts required.
// We look in all cardinal directions for the nearest wall, and the further that wall is,
// the wider our search area is.
// I am not sure if this is fully accurate, but it seems to work well enough in my tests.
void reveal_visible_tiles_optimised() {
    // Up
    cast_cardinal(player_x, player_y, player_x, player_y - 1);
    uint8_t dy = player_y - cast_y;
    // Likely bug: I'm pretty sure these calculations can underflow or extend past the edge of the map, messing up our casts
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
    // Initialise the map with the 'fog' tile
    for (uint8_t x = 0; x < MAP_WIDTH; x++) {
        for (uint8_t y = 0; y < MAP_HEIGHT; y++) {
            set_bkg_tile_xy(x, y, 2);
        }
    }
    SHOW_BKG;
    reveal_visible_tiles_optimised();

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

        // Basic movement for our sprite
        if (input & J_RIGHT //&& !(previousInput & J_RIGHT)
                && get_map_tile_xy(player_x+1, player_y) != 1) {
            player_x += 1;
            update_player();
            reveal_visible_tiles_optimised();
        } else if (input & J_LEFT //&& !(previousInput & J_LEFT)
                && get_map_tile_xy(player_x-1, player_y) != 1) {
            player_x -= 1;
            update_player();
            reveal_visible_tiles_optimised();
        } else if (input & J_UP //&& !(previousInput & J_UP)
                && get_map_tile_xy(player_x, player_y-1) != 1) {
            player_y -= 1;
            update_player();
            reveal_visible_tiles_optimised();
        } else if (input & J_DOWN //&& !(previousInput & J_DOWN)
                && get_map_tile_xy(player_x, player_y+1) != 1) {
            player_y += 1;
            update_player();
            reveal_visible_tiles_optimised();
        }
    }
}

