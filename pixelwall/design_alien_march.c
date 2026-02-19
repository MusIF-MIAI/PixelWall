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
#define NUM_SPRITES 3
#define SPRITE_GAP 3
#define PATTERN_W (SPRITE_W + SPRITE_GAP)
#define TOTAL_PATTERN_W (NUM_SPRITES * PATTERN_W)
#define NUM_FRAMES 2
#define DEFAULT_SPEED 1

// 7-wide x 7-tall alien sprites, 2 animation frames each
// bit 6 = leftmost pixel
static const uint8_t sprites[NUM_SPRITES][NUM_FRAMES][SPRITE_H] = {
    // Alien 1 - crab
    {
        // Frame 0
        {
            0x1C, // ..###..
            0x3E, // .#####.
            0x7F, // #######
            0x6B, // ##.#.##
            0x7F, // #######
            0x14, // ..#.#..
            0x22, // .#...#.
        },
        // Frame 1
        {
            0x1C, // ..###..
            0x3E, // .#####.
            0x7F, // #######
            0x6B, // ##.#.##
            0x7F, // #######
            0x22, // .#...#.
            0x41, // #.....#
        },
    },
    // Alien 2 - squid
    {
        // Frame 0
        {
            0x08, // ...#...
            0x1C, // ..###..
            0x3E, // .#####.
            0x6B, // ##.#.##
            0x7F, // #######
            0x14, // ..#.#..
            0x22, // .#...#.
        },
        // Frame 1
        {
            0x08, // ...#...
            0x1C, // ..###..
            0x3E, // .#####.
            0x6B, // ##.#.##
            0x7F, // #######
            0x2A, // .#.#.#.
            0x49, // #..#..#
        },
    },
    // Alien 3 - jellyfish
    {
        // Frame 0
        {
            0x14, // ..#.#..
            0x3E, // .#####.
            0x6B, // ##.#.##
            0x7F, // #######
            0x3E, // .#####.
            0x14, // ..#.#..
            0x41, // #.....#
        },
        // Frame 1
        {
            0x14, // ..#.#..
            0x3E, // .#####.
            0x6B, // ##.#.##
            0x7F, // #######
            0x3E, // .#####.
            0x22, // .#...#.
            0x14, // ..#.#..
        },
    },
};

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
    Color alien_colors[NUM_SPRITES];
} AlienMarchData;

static void PrintHelp() {
    printf("  -C               Enable colored mode\n");
    printf("  -R               Reverse direction (scroll right)\n");
    printf("  -S <ticks>       Scroll speed, higher = slower (default: %d)\n", DEFAULT_SPEED);
    printf("  -N <ticks>       Animation speed, higher = slower (default: same as -S)\n");
    printf("  -A <color>       Alien 1 color (R,G,B, default: 255,85,85)\n");
    printf("  -G <color>       Alien 2 color (R,G,B, default: 85,255,255)\n");
    printf("  -K <color>       Alien 3 color (R,G,B, default: 255,255,85)\n");
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    (void)grid;
    AlienMarchData *ad = calloc(1, sizeof(AlienMarchData));
    if (!ad) return NULL;

    ad->color = GREEN;
    ad->dir = 1;
    ad->speed = DEFAULT_SPEED;
    ad->anim_speed = -1;
    ad->colorful = false;
    ad->alien_colors[0] = (Color){255, 85, 85, 255};
    ad->alien_colors[1] = (Color){85, 255, 255, 255};
    ad->alien_colors[2] = (Color){255, 255, 85, 255};

    int opt;
    optind = 1;
    while ((opt = getopt(argc, argv, ":d:CRS:N:A:G:K:")) != -1) {
        switch (opt) {
            case 'C': ad->colorful = true; break;
            case 'R': ad->dir = -1; break;
            case 'S':
                ad->speed = atoi(optarg);
                if (ad->speed < 1) ad->speed = 1;
                break;
            case 'N':
                ad->anim_speed = atoi(optarg);
                if (ad->anim_speed < 1) ad->anim_speed = 1;
                break;
            case 'A': ad->alien_colors[0] = ParseColor(optarg); break;
            case 'G': ad->alien_colors[1] = ParseColor(optarg); break;
            case 'K': ad->alien_colors[2] = ParseColor(optarg); break;
        }
    }

    if (ad->anim_speed < 0) ad->anim_speed = ad->speed;

    return ad;
}

static void UpdateFrame(Grid *grid, void *data) {
    AlienMarchData *ad = (AlienMarchData *)data;
    Color bg = grid->conf.backgroundColor;

    GridFillColor(grid, bg);

    int start_row = (grid->rows - SPRITE_H) / 2;

    for (int x = 0; x < grid->cols; x++) {
        int pcol = ((x + ad->offset) % TOTAL_PATTERN_W + TOTAL_PATTERN_W) % TOTAL_PATTERN_W;
        int sprite_idx = pcol / PATTERN_W;
        int sprite_col = pcol % PATTERN_W;

        if (sprite_col >= SPRITE_W) continue;

        Color col = ad->colorful ? ad->alien_colors[sprite_idx] : ad->color;

        for (int r = 0; r < SPRITE_H; r++) {
            int gy = start_row + r;
            if (gy < 0 || gy >= grid->rows) continue;
            if ((sprites[sprite_idx][ad->frame][r] >> (SPRITE_W - 1 - sprite_col)) & 1) {
                GridSetColor(grid, (Pos){x, gy}, col);
            }
        }
    }

    ad->tick++;
    if (ad->tick >= ad->speed) {
        ad->tick = 0;
        ad->offset += ad->dir;
    }

    ad->anim_tick++;
    if (ad->anim_tick >= ad->anim_speed) {
        ad->anim_tick = 0;
        ad->frame = (ad->frame + 1) % NUM_FRAMES;
    }
}

static void Destroy(void *data) {
    free(data);
}

Design alienMarchDesign = {
    .name = "alien_march",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy,
};
