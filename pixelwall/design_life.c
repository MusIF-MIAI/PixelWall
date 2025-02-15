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
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx, ny = y + dy;
            if (nx >= 0 && nx < grid->conf.rows && ny >= 0 && ny < grid->conf.cols) {
                Pos pos = {ny, nx};
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
    static uint32_t hashes[4] = { 1, 2, 3, 4 };
    static uint32_t hashes_head = 0;

    hashes[hashes_head] = 0;

    for (int i = 0; i < grid->rows; i++) {
        for (int j = 0; j < grid->cols; j++) {
            Pos pos = {j, i};

            int live = CountLiveNeighbors(grid, i, j);
            GridSetData(grid, pos, live);

            uint32_t this_hash = hash(i, j, live);
            hashes[hashes_head] = hash_combine(hashes[hashes_head], this_hash);
        }
    }

    for (int i = 0; i < grid->rows; i++) {
        for (int j = 0; j < grid->cols; j++) {
            Pos pos = {j, i};
            int liveNeighbors = GridGetData(grid, pos);
            bool wasAlive = GridGetColor(grid, pos).r > 0;

            if (wasAlive && (liveNeighbors < 2 || liveNeighbors > 3)) {
                GridSetColor(grid, pos, BLACK);
            } else if (!wasAlive && liveNeighbors == 3) {
                GridSetColor(grid, pos, WHITE);
            } 
        }
    }

    bool completely_frozen = (
        (hashes[0] == hashes[1]) &&
        (hashes[1] == hashes[2]) &&
        (hashes[2] == hashes[3]) &&
        (hashes[3] == hashes[0])
    );

    bool oscillating = (
        (hashes[0] == hashes[2]) &&
        (hashes[1] == hashes[3])
    );

    if (completely_frozen || oscillating) {
        LifeReset(grid);
    }

    hashes_head = (hashes_head + 1) % 4;
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
