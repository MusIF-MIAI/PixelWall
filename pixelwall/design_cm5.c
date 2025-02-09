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

static Color OffColor = BLACK;
static Color OnColor = (Color) { 200, 10, 5, 255 };

static void PrintHelp() {
}

static void* Create(Grid *grid, int argc, char *argv[]) {
    for (int i = 0; i < grid->rows; i++) {
        for (int j = 0; j < grid->cols; j++) {
            Pos pos = {j, i};
            GridSetData(grid, pos, (rand() % 3) == 0);
        }
    }

    srand(time(NULL));    
    return NULL;
}

static void UpdateFrame1(Grid *grid, void *data) {
    for (int y = 0; y < grid->rows; y++) {
        if ((y / 4) % 2) {
            int first = GridGetData(grid, (Pos){0, y});
            for (int x = 0; x < grid->cols - 1; x++) {
                GridSetData(grid, (Pos){x, y}, GridGetData(grid, (Pos){x + 1, y}));
            }

            GridSetData(grid, (Pos){grid->cols - 1, y}, first);
        } else {
            int last = GridGetData(grid, (Pos){grid->cols - 1, y});
            for (int x = grid->cols - 1; x > 0; x--) {
                GridSetData(grid, (Pos){x, y}, GridGetData(grid, (Pos){x - 1, y}));
            }

            GridSetData(grid, (Pos){0, y}, last);
        }
    }

    for (int y = 0; y < grid->rows; y++) {
        for (int x = 0; x < grid->cols; x++) {
            Pos pos = (Pos) { x, y };
            bool isOn = GridGetData(grid, pos);
            GridSetColor(grid, pos, isOn ? OnColor : OffColor);
        }
    }
}

static void UpdateFrame(Grid *grid, void *data) {
    for (int x = 0; x < grid->cols; x++) {
        if ((x / 4) % 2) {
            int first = GridGetData(grid, (Pos){x, 0});
            for (int y = 0; y < grid->rows - 1; y++) {
                GridSetData(grid, (Pos){x, y}, GridGetData(grid, (Pos){x, y + 1}));
            }

            GridSetData(grid, (Pos){x, grid->rows - 1}, first);
        } else {
            int last = GridGetData(grid, (Pos){x, grid->rows - 1});
            for (int y = grid->rows - 1; y > 0; y--) {
                GridSetData(grid, (Pos){x, y}, GridGetData(grid, (Pos){x, y - 1}));
            }

            GridSetData(grid, (Pos){x, 0}, last);
        }
    }

    for (int y = 0; y < grid->rows; y++) {
        for (int x = 0; x < grid->cols; x++) {
            Pos pos = (Pos) { x, y };
            bool isOn = GridGetData(grid, pos);
            GridSetColor(grid, pos, isOn ? OnColor : OffColor);
        }
    }
}

static void Destroy(void *data) {
}

Design cm5Design = {
    .name = "cm5",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy
};
