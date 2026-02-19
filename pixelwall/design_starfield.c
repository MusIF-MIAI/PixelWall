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
#include <math.h>

#define MAX_STARS 30
#define ACCEL 1.05f
#define SPAWN_PER_FRAME 2

typedef struct {
    float x, y;
    float vx, vy;
    bool active;
} Star;

typedef struct {
    Star stars[MAX_STARS];
    bool colorful;
    Color color;
    float cx, cy;
} StarfieldData;

static void SpawnStar(StarfieldData *sd) {
    for (int i = 0; i < MAX_STARS; i++) {
        if (sd->stars[i].active) continue;
        sd->stars[i].active = true;
        sd->stars[i].x = sd->cx;
        sd->stars[i].y = sd->cy;
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;
        float speed = 0.1f + (float)(rand() % 10) * 0.02f;
        sd->stars[i].vx = cosf(angle) * speed;
        sd->stars[i].vy = sinf(angle) * speed;
        return;
    }
}

static void PrintHelp() {
    printf("  -C               Enable colored mode (brightness varies by distance)\n");
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    StarfieldData *sd = calloc(1, sizeof(StarfieldData));
    if (!sd) return NULL;

    sd->color = GREEN;
    sd->colorful = false;
    sd->cx = grid->cols / 2.0f;
    sd->cy = grid->rows / 2.0f;

    int opt;
    optind = 1;
    while ((opt = getopt(argc, argv, ":d:C")) != -1) {
        switch (opt) {
            case 'C': sd->colorful = true; break;
        }
    }

    srand(time(NULL));
    return sd;
}

static void UpdateFrame(Grid *grid, void *data) {
    StarfieldData *sd = (StarfieldData *)data;
    Color bg = grid->conf.backgroundColor;

    GridFillColor(grid, bg);

    float max_dist = sqrtf(sd->cx * sd->cx + sd->cy * sd->cy);

    for (int i = 0; i < MAX_STARS; i++) {
        if (!sd->stars[i].active) continue;
        Star *s = &sd->stars[i];

        s->x += s->vx;
        s->y += s->vy;
        s->vx *= ACCEL;
        s->vy *= ACCEL;

        int gx = (int)(s->x + 0.5f);
        int gy = (int)(s->y + 0.5f);

        if (gx < 0 || gx >= grid->cols || gy < 0 || gy >= grid->rows) {
            s->active = false;
            continue;
        }

        Color col;
        if (sd->colorful) {
            float dx = s->x - sd->cx;
            float dy = s->y - sd->cy;
            float dist = sqrtf(dx * dx + dy * dy);
            float bright = dist / max_dist;
            if (bright > 1.0f) bright = 1.0f;
            unsigned char v = (unsigned char)(80 + bright * 175);
            col = (Color){v, v, v, 255};
        } else {
            col = sd->color;
        }

        GridSetColor(grid, (Pos){gx, gy}, col);
    }

    for (int i = 0; i < SPAWN_PER_FRAME; i++) {
        SpawnStar(sd);
    }
}

static void Destroy(void *data) {
    free(data);
}

Design starfieldDesign = {
    .name = "starfield",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy,
};
