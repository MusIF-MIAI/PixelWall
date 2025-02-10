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

static void PrintHelp() {
}

static Color RandomColor() {
    Color OffColor = BLACK;
    Color OnColor = (Color) { 200, 10, 5, 255 };
    return ((rand() % 5) == 0) ? OnColor : OffColor;
}

static void RotateRight(Grid *grid, int y) {
    for (int x = 0; x < grid->cols - 1; x++) {
        GridSetColor(grid, (Pos){x, y}, GridGetColor(grid, (Pos){x + 1, y}));
    }

    GridSetColor(grid, (Pos){grid->cols - 1, y}, RandomColor());
}

static void RotateLeft(Grid *grid, int y) {
    for (int x = grid->cols - 1; x > 0; x--) {
        GridSetColor(grid, (Pos){x, y}, GridGetColor(grid, (Pos){x - 1, y}));
    }

    GridSetColor(grid, (Pos){0, y}, RandomColor());
}

static void RotateDown(Grid *grid, int x) {
    for (int y = 0; y < grid->rows - 1; y++) {
        GridSetColor(grid, (Pos){x, y}, GridGetColor(grid, (Pos){x, y + 1}));
    }

    GridSetColor(grid, (Pos){x, grid->rows - 1}, RandomColor());
}

static void RotateUp(Grid *grid, int x) {
    for (int y = grid->rows - 1; y > 0; y--) {
        GridSetColor(grid, (Pos){x, y}, GridGetColor(grid, (Pos){x, y - 1}));
    }

    GridSetColor(grid, (Pos){x, 0}, RandomColor());
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    for (int x = 0; x < grid->cols; x++) {
        for (int y = 0; y < grid->rows; y++) {
            GridSetColor(grid, (Pos){x, y}, RandomColor());
        }
    }

    return NULL;
}

static void UpdateFrame(Grid *grid, void *data) {
    static void (*rotations[2][2])(Grid *, int) = {
         {RotateUp, RotateDown },
         {RotateRight, RotateLeft},
    };

    static bool horizontal = true;
    int limit = horizontal ? grid->rows : grid->cols;

    for (int i = 0; i < limit; i++) {
        bool stripe = ((i / 4) % 2) == 0;
        rotations[horizontal][stripe](grid, i);
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
