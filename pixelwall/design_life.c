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

static void* LifeCreate(Grid *grid, int argc, char *argv[]) {
    for (int i = 0; i < grid->rows; i++) {
        for (int j = 0; j < grid->cols; j++) {
            Pos pos = {j, i};
            Color color = (rand() % 2) ? WHITE : BLACK;
            GridSetColor(grid, pos, color);
        }
    }

    srand(time(NULL));    
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

static void LifeUpdateFrame(Grid *grid, void *data) {
    for (int i = 0; i < grid->rows; i++) {
        for (int j = 0; j < grid->cols; j++) {
            Pos pos = {j, i};

            int live = CountLiveNeighbors(grid, i, j);
            GridSetData(grid, pos, live);
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
