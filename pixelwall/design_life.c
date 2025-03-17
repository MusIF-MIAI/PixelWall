/*
DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

Copyright (C) 2004 Sam Hocevar <sam@hocevar.net>

Everyone is permitted to copy and distribute verbatim or modified
copies of this license document, and changing it is allowed as long
as the name is changed.

DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

0. You just DO WHAT THE FUCK YOU WANT TO.
*/

#include "pixelwall.h"


static void LifePrintHelp() {
}

static void LifeReset(Grid *grid) {
    for (int i = 0; i < grid->rows; i++) {
        for (int j = 0; j < grid->cols; j++) {
            Pos pos = {j, i};
            Color color = (rand() % 2) ? WHITE : BLACK;
            GridSetColor(grid, pos, color);
        }
    }
}

static void* LifeCreate(Grid *grid, int argc, char *argv[]) {
    srand(time(NULL));
    LifeReset(grid);
    return NULL;
}

static int CountLiveNeighbors(Grid *grid, int x, int y) {
    int count = 0;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0)
                continue;
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && nx < grid->cols && ny >= 0 && ny < grid->rows) {
                Pos pos = {nx, ny};
                if (GridGetColor(grid, pos).r > 0)
                    count++;
            }
        }
    }
    return count;
}

static uint32_t hash(int a, int b, int c) {
    return (a * 2654435761U) ^ (b * 2246822519U) ^ (c * 3266489917U);
}

static uint32_t hash_combine(uint32_t prev, uint32_t next) {
    return prev ^ (next + 0x9e3779b9 + (prev << 6) + (prev >> 2));
}

static void LifeUpdateFrame(Grid *grid, void *data) {
    for (int i = 0; i < grid->rows; i++) {
        for (int j = 0; j < grid->cols; j++) {
            Pos pos = {j, i};
            int live = CountLiveNeighbors(grid, j, i);
            GridSetData(grid, pos, live);
        }
    }
    
    uint32_t current_hash = 0;
    for (int i = 0; i < grid->rows; i++) {
        for (int j = 0; j < grid->cols; j++) {
            Pos pos = {j, i};
            int liveNeighbors = GridGetData(grid, pos);
            bool wasAlive = GridGetColor(grid, pos).r > 0;
            bool newAlive = wasAlive;
            if (wasAlive && (liveNeighbors < 2 || liveNeighbors > 3))
                newAlive = false;
            else if (!wasAlive && liveNeighbors == 3)
                newAlive = true;
            
            GridSetColor(grid, pos, newAlive ? WHITE : BLACK);
            
            current_hash = hash_combine(current_hash, hash(j, i, newAlive ? 1 : 0));
        }
    }
    
    static int frame_counter = 0;
    static uint32_t hash_prev = 0;
    static uint32_t hash_prev2 = 0;
    
    if (frame_counter % 10 == 0) {
        if (current_hash == hash_prev || current_hash == hash_prev2) {
            LifeReset(grid);
            hash_prev = 0;
            hash_prev2 = 0;
            frame_counter = 0;
            return;
        }
        hash_prev2 = hash_prev;
        hash_prev = current_hash;
    }
    frame_counter++;
}

static void LifeDestroy(void *data) {
}

Design lifeDesign = {
    .name = "life",
    .PrintHelp = LifePrintHelp,
    .Create = LifeCreate,
    .UpdateFrame = LifeUpdateFrame,
    .Destroy = LifeDestroy
};
