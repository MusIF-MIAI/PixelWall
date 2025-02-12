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

typedef struct {
    int stripe;
    int chance;
    Direction direction;
    Color onColor;
    Color offColor;
} CM5Conf;

static CM5Conf defaultConf = {
    .stripe = 4,
    .chance = 5,
    .direction = HORIZONTAL,
    .offColor = BLACK,
    .onColor = (Color) { 200, 10, 5, 255 },
};

static void PrintHelp() {
    printf("  -s <stripes>     Number of row in a single stripe (default: %d)\n", defaultConf.stripe);
    printf("  -C <chance>      Chance of an on pixel (default: %d)\n", defaultConf.chance);
    printf("  -d <direction>   Set initial direction (0: horizontal, 1: vertical default: %d)\n", defaultConf.direction);    printf("  -O <color>       On color (R,G,B, default: %d,%d,%d)\n",
           defaultConf.onColor.r, defaultConf.onColor.g, defaultConf.onColor.b);
    printf("  -I <color>       Off color (R,G,B, default: %d,%d,%d)\n",
           defaultConf.offColor.r, defaultConf.offColor.g, defaultConf.offColor.b);
}

static void ParseOptions(CM5Conf *conf, int argc, char *argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, ":s:C:d:O:I:")) != -1) {
        switch (opt) {
            case 's': conf->stripe = atoi(optarg); break;
            case 'C': conf->chance = atoi(optarg); break;
            case 'd': conf->direction = ParseDirection(optarg); break;
            case 'O': conf->offColor = ParseColor(optarg); break;
            case 'I': conf->onColor = ParseColor(optarg); break;
        }
    }
}

static Color RandomColor(CM5Conf *conf) {
    return ((rand() % conf->chance) == 0) ? conf->onColor : conf->offColor;
}

static void RotateRight(Grid *grid, int y, Color color) {
    for (int x = 0; x < grid->cols - 1; x++) {
        GridSetColor(grid, (Pos){x, y}, GridGetColor(grid, (Pos){x + 1, y}));
    }

    GridSetColor(grid, (Pos){grid->cols - 1, y}, color);
}

static void RotateLeft(Grid *grid, int y, Color color) {
    for (int x = grid->cols - 1; x > 0; x--) {
        GridSetColor(grid, (Pos){x, y}, GridGetColor(grid, (Pos){x - 1, y}));
    }

    GridSetColor(grid, (Pos){0, y}, color);
}

static void RotateDown(Grid *grid, int x, Color color) {
    for (int y = 0; y < grid->rows - 1; y++) {
        GridSetColor(grid, (Pos){x, y}, GridGetColor(grid, (Pos){x, y + 1}));
    }

    GridSetColor(grid, (Pos){x, grid->rows - 1}, color);
}

static void RotateUp(Grid *grid, int x, Color color) {
    for (int y = grid->rows - 1; y > 0; y--) {
        GridSetColor(grid, (Pos){x, y}, GridGetColor(grid, (Pos){x, y - 1}));
    }

    GridSetColor(grid, (Pos){x, 0}, color);
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    CM5Conf *conf = (CM5Conf *)malloc(sizeof(CM5Conf));
    if (!conf) return NULL;
    *conf = defaultConf;
    ParseOptions(conf, argc, argv);

    for (int x = 0; x < grid->cols; x++) {
        for (int y = 0; y < grid->rows; y++) {
            GridSetColor(grid, (Pos){x, y}, RandomColor(conf));
        }
    }

    return conf;
}

static void UpdateFrame(Grid *grid, void *data) {
    CM5Conf *conf = (CM5Conf *)data;

    if (IsKeyPressed(KEY_D)) {
        conf->direction = !conf->direction;
    }

    static void (*rotations[2][2])(Grid *, int, Color) = {
         { RotateRight, RotateLeft },
         { RotateUp, RotateDown },
    };

    int limit = conf->direction == HORIZONTAL ? grid->rows : grid->cols;
    for (int i = 0; i < limit; i++) {
        bool stripe = ((i / conf->stripe) % 2) == 0;
        rotations[conf->direction][stripe](grid, i, RandomColor(conf));
    }
}

static void Destroy(void *data) {
    free(data);
}

Design cm5Design = {
    .name = "cm5",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy
};
