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
#include <stdbool.h>

#define SPRITE_W 7
#define SPRITE_H 7
#define NUM_FRAMES 2
#define DEFAULT_SPEED 4
#define DEFAULT_ANIM_SPEED 1

// Pac-Man: 2 frames (mouth open / mouth closed)
static const uint8_t pacman[NUM_FRAMES][SPRITE_H] = {
    // Frame 0 - mouth open
    {
        0x1C, // ..###..
        0x3E, // .#####.
        0x7C, // #####..
        0x70, // ###....
        0x7C, // #####..
        0x3E, // .#####.
        0x1C, // ..###..
    },
    // Frame 1 - mouth closing
    {
        0x1C, // ..###..
        0x3E, // .#####.
        0x7E, // ######.
        0x7F, // #######
        0x7E, // ######.
        0x3E, // .#####.
        0x1C, // ..###..
    },
};

// Ghost: 2 frames (legs alternate)
static const uint8_t ghost[NUM_FRAMES][SPRITE_H] = {
    // Frame 0
    {
        0x1C, // ..###..
        0x3E, // .#####.
        0x6B, // ##.##.#
        0x7F, // #######
        0x7F, // #######
        0x7F, // #######
        0x55, // #.#.#.#
    },
    // Frame 1
    {
        0x1C, // ..###..
        0x3E, // .#####.
        0x6B, // ##.##.#
        0x7F, // #######
        0x7F, // #######
        0x7F, // #######
        0x2A, // .#.#.#.
    },
};

// Dot: small pellet centered in a 3-wide space (only middle row has a pixel)
#define DOT_W 3
#define DOT_H 7

// Pattern layout:
// [pacman 7] [gap 2] [dot 3] [dot 3] [dot 3] [ghost 7] [gap 2] [ghost 7] [gap 2]
// Total: 7 + 2 + 3 + 3 + 3 + 7 + 2 + 7 + 2 = 36

#define NUM_ELEMENTS 9
static const int element_widths[NUM_ELEMENTS] = { 7, 2, 3, 3, 3, 7, 2, 7, 2 };
// Element types: 0=pacman, 1=gap, 2=dot, 3=ghost
static const int element_types[NUM_ELEMENTS] = { 0, 1, 2, 2, 2, 3, 1, 3, 1 };
#define TOTAL_PATTERN_W 36

typedef struct {
    int offset;
    int dir;
    int tick;
    int speed;
    int anim_tick;
    int anim_speed;
    int frame;
    bool colorful;
    Color color;
    Color pacman_color;
    Color ghost_colors[2];
    Color dot_color;
} PacmanMarchData;

static void PrintHelp() {
    printf("  -C               Enable colored mode\n");
    printf("  -R               Reverse direction (scroll right)\n");
    printf("  -S <ticks>       Scroll speed, higher = slower (default: %d)\n", DEFAULT_SPEED);
    printf("  -N <ticks>       Animation speed, higher = slower (default: %d)\n", DEFAULT_ANIM_SPEED);
    printf("  -P <color>       Pac-Man color (R,G,B, default: 255,255,85)\n");
    printf("  -G <color>       Ghost 1 color (R,G,B, default: 255,85,85)\n");
    printf("  -K <color>       Ghost 2 color (R,G,B, default: 85,255,255)\n");
    printf("  -E <color>       Dot color (R,G,B, default: 255,185,80)\n");
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    (void)grid;
    PacmanMarchData *pd = calloc(1, sizeof(PacmanMarchData));
    if (!pd) return NULL;

    pd->color = GREEN;
    pd->dir = 1;
    pd->speed = DEFAULT_SPEED;
    pd->anim_speed = DEFAULT_ANIM_SPEED;
    pd->colorful = false;
    pd->pacman_color = (Color){255, 255, 85, 255};
    pd->ghost_colors[0] = (Color){255, 85, 85, 255};
    pd->ghost_colors[1] = (Color){85, 255, 255, 255};
    pd->dot_color = (Color){255, 185, 80, 255};

    int opt;
    optind = 1;
    while ((opt = getopt(argc, argv, ":d:CRS:N:P:G:K:E:")) != -1) {
        switch (opt) {
            case 'C': pd->colorful = true; break;
            case 'R': pd->dir = -1; break;
            case 'S':
                pd->speed = atoi(optarg);
                if (pd->speed < 1) pd->speed = 1;
                break;
            case 'N':
                pd->anim_speed = atoi(optarg);
                if (pd->anim_speed < 1) pd->anim_speed = 1;
                break;
            case 'P': pd->pacman_color = ParseColor(optarg); break;
            case 'G': pd->ghost_colors[0] = ParseColor(optarg); break;
            case 'K': pd->ghost_colors[1] = ParseColor(optarg); break;
            case 'E': pd->dot_color = ParseColor(optarg); break;
        }
    }

    return pd;
}

static void DrawColumn(PacmanMarchData *pd, Grid *grid, int screen_x, int pattern_col,
                        int start_row, int frame) {
    // Find which element this column belongs to
    int col = pattern_col;
    int elem = 0;
    while (elem < NUM_ELEMENTS && col >= element_widths[elem]) {
        col -= element_widths[elem];
        elem++;
    }
    if (elem >= NUM_ELEMENTS) return;

    int type = element_types[elem];

    if (type == 1) return; // gap

    if (type == 2) {
        // Dot: single pixel in the center (col 1, row 3)
        if (col == 1) {
            int gy = start_row + SPRITE_H / 2;
            if (gy >= 0 && gy < grid->rows) {
                Color c = pd->colorful ? pd->dot_color : pd->color;
                GridSetColor(grid, (Pos){screen_x, gy}, c);
            }
        }
        return;
    }

    const uint8_t *sprite;
    Color c;

    if (type == 0) {
        // Pac-Man
        sprite = pacman[frame];
        c = pd->colorful ? pd->pacman_color : pd->color;
    } else {
        // Ghost - figure out which one (first or second ghost in pattern)
        int ghost_idx = (elem == 5) ? 0 : 1;
        sprite = ghost[frame];
        c = pd->colorful ? pd->ghost_colors[ghost_idx] : pd->color;
    }

    for (int r = 0; r < SPRITE_H; r++) {
        int gy = start_row + r;
        if (gy < 0 || gy >= grid->rows) continue;
        if ((sprite[r] >> (SPRITE_W - 1 - col)) & 1) {
            GridSetColor(grid, (Pos){screen_x, gy}, c);
        }
    }
}

static void UpdateFrame(Grid *grid, void *data) {
    PacmanMarchData *pd = (PacmanMarchData *)data;
    Color bg = grid->conf.backgroundColor;

    GridFillColor(grid, bg);

    int start_row = (grid->rows - SPRITE_H) / 2;

    for (int x = 0; x < grid->cols; x++) {
        int pcol = ((x + pd->offset) % TOTAL_PATTERN_W + TOTAL_PATTERN_W) % TOTAL_PATTERN_W;
        DrawColumn(pd, grid, x, pcol, start_row, pd->frame);
    }

    pd->tick++;
    if (pd->tick >= pd->speed) {
        pd->tick = 0;
        pd->offset += pd->dir;
    }

    pd->anim_tick++;
    if (pd->anim_tick >= pd->anim_speed) {
        pd->anim_tick = 0;
        pd->frame = (pd->frame + 1) % NUM_FRAMES;
    }
}

static void Destroy(void *data) {
    free(data);
}

Design pacmanMarchDesign = {
    .name = "pacman_march",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy,
};
