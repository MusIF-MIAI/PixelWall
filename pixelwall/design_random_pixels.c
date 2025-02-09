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
    int initialSquares;
    int maxRandomTick;
} RandomConf;

static RandomConf defaultRandomConf = {
    .initialSquares = 10,
    .maxRandomTick = 100,
};

static void ParseOptions(RandomConf *conf, int argc, char *argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, ":I:m:")) != -1) {
        switch (opt) {
            case 'I':
                conf->initialSquares = atoi(optarg);
                break;

            case 'm':
                conf->maxRandomTick = atoi(optarg);
                break;
        }
    }
}

static void PrintHelp() {
    printf("  -I <squares>     Initial active squares (default: %d)\n", defaultRandomConf.initialSquares);
    printf("  -m <ticks>       Maximum random ticks (default: %d)\n", defaultRandomConf.maxRandomTick);
}

static void ActivateRandomPixel(Grid *grid, RandomConf *conf) {
    Pos pos;
    do { 
        pos = GetRandomPositionIn(grid->rows, grid->cols);
    } while (GridGetData(grid, pos) > 0);

    GridSetColor(grid, pos, GetRandomColor());
    GridSetData(grid, pos, GetRandomValue(1, conf->maxRandomTick));
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    RandomConf *conf = (RandomConf *)malloc(sizeof(RandomConf));
    *conf = defaultRandomConf;
    if (!conf) return NULL;
    ParseOptions(conf, argc, argv);

    // Activate random initial pixels
    for (int i = 0; i < conf->initialSquares; i++) {
        ActivateRandomPixel(grid, conf);
    }

    return conf;
}

static void UpdateFrame(Grid *grid, void *data) {
    RandomConf *conf = (RandomConf *)data;
    
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            Pos pos = (Pos){col, row};
            uintptr_t tick = GridGetData(grid, pos);
            if (tick > 0) {
                tick--;
                GridSetData(grid, pos, tick);

                if (tick == 0) {
                    GridSetColor(grid, pos, grid->conf.backgroundColor);
                    ActivateRandomPixel(grid, conf);
                }
            }
        }
    }
}

static void Destroy(void *data) {
    free(data);
}

Design randomPixelsDesign = {
    .name = "random-pixels",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy,
};