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
#define NUM_GHOSTS 2
#define SPACING 9
#define DEFAULT_SPEED 4
#define DEFAULT_ANIM_SPEED 1

// Pac-Man facing left: 2 frames
static const uint8_t pacman_left[NUM_FRAMES][SPRITE_H] = {
    // Frame 0 - mouth open
    {
        0x1C, // ..###..
        0x3E, // .#####.
        0x1F, // ..#####
        0x07, // ....###
        0x1F, // ..#####
        0x3E, // .#####.
        0x1C, // ..###..
    },
    // Frame 1 - mouth closing
    {
        0x1C, // ..###..
        0x3E, // .#####.
        0x3F, // .######
        0x7F, // #######
        0x3F, // .######
        0x3E, // .#####.
        0x1C, // ..###..
    },
};

// Pac-Man facing right: 2 frames
static const uint8_t pacman_right[NUM_FRAMES][SPRITE_H] = {
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

// Normal ghost: 2 frames (legs alternate)
static const uint8_t ghost_normal[NUM_FRAMES][SPRITE_H] = {
    {
        0x1C, // ..###..
        0x3E, // .#####.
        0x6B, // ##.##.#
        0x7F, // #######
        0x7F, // #######
        0x7F, // #######
        0x55, // #.#.#.#
    },
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

// Scared ghost (blue): 2 frames
static const uint8_t ghost_scared[NUM_FRAMES][SPRITE_H] = {
    {
        0x1C, // ..###..
        0x3E, // .#####.
        0x55, // #.#.#.#
        0x7F, // #######
        0x6B, // ##.#.##
        0x7F, // #######
        0x55, // #.#.#.#
    },
    {
        0x1C, // ..###..
        0x3E, // .#####.
        0x55, // #.#.#.#
        0x7F, // #######
        0x6B, // ##.#.##
        0x7F, // #######
        0x2A, // .#.#.#.
    },
};

// Phase 0: ghosts chase pac-man, all moving left
//   positions: pacman at x, ghost0 at x+SPACING, ghost1 at x+2*SPACING
//   start: x = grid->cols, end: x + 2*SPACING + SPRITE_W < 0
//
// Phase 1: pac-man chases scared ghosts, all moving right
//   positions: ghost0 at x, ghost1 at x+SPACING, pacman at x+2*SPACING
//   start: x = -total_width, end: x > grid->cols

typedef struct {
    int x;
    int phase;
    int tick;
    int speed;
    int anim_tick;
    int anim_speed;
    int frame;
    bool colorful;
    Color color;
    Color pacman_color;
    Color ghost_colors[NUM_GHOSTS];
    Color scared_color;
    int cols;
} PacmanMarchData;

static void PrintHelp() {
    printf("  -C               Enable colored mode\n");
    printf("  -S <ticks>       Scroll speed, higher = slower (default: %d)\n", DEFAULT_SPEED);
    printf("  -N <ticks>       Animation speed, higher = slower (default: %d)\n", DEFAULT_ANIM_SPEED);
    printf("  -P <color>       Pac-Man color (R,G,B, default: 255,255,85)\n");
    printf("  -G <color>       Ghost 1 color (R,G,B, default: 255,85,85)\n");
    printf("  -K <color>       Ghost 2 color (R,G,B, default: 255,85,255)\n");
    printf("  -E <color>       Scared ghost color (R,G,B, default: 85,85,255)\n");
}

static void StartPhase(PacmanMarchData *pd, int phase) {
    pd->phase = phase;
    int total = 2 * SPACING + SPRITE_W;
    if (phase == 0) {
        pd->x = pd->cols;
    } else {
        pd->x = -total;
    }
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    PacmanMarchData *pd = calloc(1, sizeof(PacmanMarchData));
    if (!pd) return NULL;

    pd->color = GREEN;
    pd->speed = DEFAULT_SPEED;
    pd->anim_speed = DEFAULT_ANIM_SPEED;
    pd->colorful = false;
    pd->pacman_color = (Color){255, 255, 85, 255};
    pd->ghost_colors[0] = (Color){255, 85, 85, 255};
    pd->ghost_colors[1] = (Color){255, 85, 255, 255};
    pd->scared_color = (Color){85, 85, 255, 255};
    pd->cols = grid->cols;

    int opt;
    optind = 1;
    while ((opt = getopt(argc, argv, ":d:CS:N:P:G:K:E:")) != -1) {
        switch (opt) {
            case 'C': pd->colorful = true; break;
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
            case 'E': pd->scared_color = ParseColor(optarg); break;
        }
    }

    StartPhase(pd, 0);
    return pd;
}

static void DrawSprite(Grid *grid, const uint8_t *sprite, int sx, int start_row, Color color) {
    for (int c = 0; c < SPRITE_W; c++) {
        int gx = sx + c;
        if (gx < 0 || gx >= grid->cols) continue;
        for (int r = 0; r < SPRITE_H; r++) {
            int gy = start_row + r;
            if (gy < 0 || gy >= grid->rows) continue;
            if ((sprite[r] >> (SPRITE_W - 1 - c)) & 1) {
                GridSetColor(grid, (Pos){gx, gy}, color);
            }
        }
    }
}

static void UpdateFrame(Grid *grid, void *data) {
    PacmanMarchData *pd = (PacmanMarchData *)data;
    Color bg = grid->conf.backgroundColor;

    GridFillColor(grid, bg);

    int start_row = (grid->rows - SPRITE_H) / 2;
    int f = pd->frame;

    if (pd->phase == 0) {
        // Ghosts chase Pac-Man, all moving left
        // Pac-Man leads (leftmost), ghosts follow
        Color pc = pd->colorful ? pd->pacman_color : pd->color;
        DrawSprite(grid, pacman_left[f], pd->x, start_row, pc);
        for (int i = 0; i < NUM_GHOSTS; i++) {
            Color gc = pd->colorful ? pd->ghost_colors[i] : pd->color;
            DrawSprite(grid, ghost_normal[f], pd->x + (i + 1) * SPACING, start_row, gc);
        }
    } else {
        // Pac-Man chases scared ghosts, all moving right
        // Ghosts lead (leftmost), Pac-Man follows
        for (int i = 0; i < NUM_GHOSTS; i++) {
            Color sc = pd->colorful ? pd->scared_color : pd->color;
            DrawSprite(grid, ghost_scared[f], pd->x + i * SPACING, start_row, sc);
        }
        Color pc = pd->colorful ? pd->pacman_color : pd->color;
        DrawSprite(grid, pacman_right[f], pd->x + NUM_GHOSTS * SPACING, start_row, pc);
    }

    // Scroll
    pd->tick++;
    if (pd->tick >= pd->speed) {
        pd->tick = 0;
        int total = 2 * SPACING + SPRITE_W;
        if (pd->phase == 0) {
            pd->x--;
            if (pd->x + total < 0) {
                StartPhase(pd, 1);
            }
        } else {
            pd->x++;
            if (pd->x > pd->cols) {
                StartPhase(pd, 0);
            }
        }
    }

    // Animation
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
