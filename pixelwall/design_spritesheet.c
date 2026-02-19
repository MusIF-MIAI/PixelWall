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

typedef struct {
    Image image;
    int grid_cols, grid_rows;
    int frame_w, frame_h;
    int *frames;
    int num_frames;
    int current_frame;
    int tick, anim_speed;
    int offset_x, offset_y;
    int spacing_x, spacing_y;
    float zoom;
    bool use_key_color;
    Color key_color;
    int key_tolerance;
} SpritesheetData;

static void PrintHelp() {
    printf("  -I <path>        Sprite sheet image file (required)\n");
    printf("  -G <CxR>         Grid layout: columns x rows of sprites (default: 1x1)\n");
    printf("  -W <WxH>         Sprite frame size in pixels (default: auto)\n");
    printf("  -A <list>        Frame indices to animate, comma-separated (default: 0)\n");
    printf("  -N <ticks>       Animation speed, higher = slower (default: 5)\n");
    printf("  -P <XxY>         Offset from top-left corner in pixels (default: 0x0)\n");
    printf("  -S <XxY>         Spacing between frames in pixels (default: 0x0)\n");
    printf("  -Z <factor>      Zoom factor (default: 1.0)\n");
    printf("  -K               Use top-left pixel of sprite as transparent key color\n");
    printf("  -E <tolerance>   Key color tolerance for JPEG artifacts (default: 30)\n");
}

static int *ParseFrameList(const char *str, int *count) {
    int capacity = 8;
    int *frames = malloc(capacity * sizeof(int));
    int n = 0;

    const char *p = str;
    while (*p) {
        if (n >= capacity) {
            capacity *= 2;
            frames = realloc(frames, capacity * sizeof(int));
        }
        frames[n++] = atoi(p);
        while (*p && *p != ',') p++;
        if (*p == ',') p++;
    }

    *count = n;
    return frames;
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    SpritesheetData *sd = calloc(1, sizeof(SpritesheetData));
    if (!sd) return NULL;

    sd->grid_cols = 1;
    sd->grid_rows = 1;
    sd->frame_w = 0;
    sd->frame_h = 0;
    sd->anim_speed = 5;
    sd->zoom = 1.0f;
    sd->use_key_color = false;
    sd->key_tolerance = 30;

    char *image_path = NULL;
    char *grid_spec = NULL;
    char *size_spec = NULL;
    char *anim_spec = NULL;
    char *offset_spec = NULL;
    char *spacing_spec = NULL;

    int opt;
    optind = 1;
    while ((opt = getopt(argc, argv, ":d:I:G:W:A:N:KE:P:S:Z:")) != -1) {
        switch (opt) {
            case 'I': image_path = optarg; break;
            case 'G': grid_spec = optarg; break;
            case 'W': size_spec = optarg; break;
            case 'A': anim_spec = optarg; break;
            case 'N': sd->anim_speed = atoi(optarg); break;
            case 'K': sd->use_key_color = true; break;
            case 'E': sd->key_tolerance = atoi(optarg); break;
            case 'P': offset_spec = optarg; break;
            case 'S': spacing_spec = optarg; break;
            case 'Z': sd->zoom = atof(optarg); break;
        }
    }

    if (!image_path) {
        fprintf(stderr, "spritesheet: -I <path> is required\n");
        exit(1);
    }

    sd->image = LoadImage(image_path);
    if (sd->image.data == NULL) {
        fprintf(stderr, "spritesheet: failed to load image '%s'\n", image_path);
        exit(1);
    }

    if (grid_spec) {
        sscanf(grid_spec, "%dx%d", &sd->grid_cols, &sd->grid_rows);
    }
    if (sd->grid_cols < 1 || sd->grid_rows < 1) {
        fprintf(stderr, "spritesheet: invalid grid %dx%d\n", sd->grid_cols, sd->grid_rows);
        exit(1);
    }

    if (offset_spec) {
        sscanf(offset_spec, "%dx%d", &sd->offset_x, &sd->offset_y);
    }
    if (spacing_spec) {
        sscanf(spacing_spec, "%dx%d", &sd->spacing_x, &sd->spacing_y);
    }

    if (size_spec) {
        sscanf(size_spec, "%dx%d", &sd->frame_w, &sd->frame_h);
    } else {
        sd->frame_w = (sd->image.width - sd->offset_x - sd->spacing_x * (sd->grid_cols - 1)) / sd->grid_cols;
        sd->frame_h = (sd->image.height - sd->offset_y - sd->spacing_y * (sd->grid_rows - 1)) / sd->grid_rows;
    }
    if (sd->frame_w < 1 || sd->frame_h < 1) {
        fprintf(stderr, "spritesheet: invalid frame size %dx%d\n", sd->frame_w, sd->frame_h);
        exit(1);
    }


    int max_frames = sd->grid_cols * sd->grid_rows;
    int total_w = sd->offset_x + sd->grid_cols * sd->frame_w + (sd->grid_cols - 1) * sd->spacing_x;
    int total_h = sd->offset_y + sd->grid_rows * sd->frame_h + (sd->grid_rows - 1) * sd->spacing_y;
    if (total_w > sd->image.width || total_h > sd->image.height) {
        fprintf(stderr, "spritesheet: sprite grid (%dx%d frames of %dx%d, offset %dx%d, spacing %dx%d) exceeds image size %dx%d\n",
                sd->grid_cols, sd->grid_rows, sd->frame_w, sd->frame_h,
                sd->offset_x, sd->offset_y, sd->spacing_x, sd->spacing_y,
                sd->image.width, sd->image.height);
        exit(1);
    }

    if (anim_spec) {
        sd->frames = ParseFrameList(anim_spec, &sd->num_frames);
    } else {
        sd->frames = malloc(sizeof(int));
        sd->frames[0] = 0;
        sd->num_frames = 1;
    }

    for (int i = 0; i < sd->num_frames; i++) {
        if (sd->frames[i] < 0 || sd->frames[i] >= max_frames) {
            fprintf(stderr, "spritesheet: frame index %d out of range (0-%d)\n",
                    sd->frames[i], max_frames - 1);
            exit(1);
        }
    }

    if (sd->use_key_color) {
        int col = sd->frames[0] % sd->grid_cols;
        int row = sd->frames[0] / sd->grid_cols;
        int fx = sd->offset_x + col * (sd->frame_w + sd->spacing_x);
        int fy = sd->offset_y + row * (sd->frame_h + sd->spacing_y);
        sd->key_color = GetImageColor(sd->image, fx, fy);
    }

    return sd;
}

static void UpdateFrame(Grid *grid, void *data) {
    SpritesheetData *sd = (SpritesheetData *)data;

    GridFillColor(grid, grid->conf.backgroundColor);

    int idx = sd->frames[sd->current_frame];
    int col = idx % sd->grid_cols;
    int row = idx / sd->grid_cols;
    int fx = sd->offset_x + col * (sd->frame_w + sd->spacing_x);
    int fy = sd->offset_y + row * (sd->frame_h + sd->spacing_y);

    int draw_w = (int)(sd->frame_w * sd->zoom);
    int draw_h = (int)(sd->frame_h * sd->zoom);
    int ox = (grid->cols - draw_w) / 2;
    int oy = (grid->rows - draw_h) / 2;
    if (ox < 0) ox = 0;
    if (oy < 0) oy = 0;

    for (int gy = 0; gy < draw_h && gy + oy < grid->rows; gy++) {
        for (int gx = 0; gx < draw_w && gx + ox < grid->cols; gx++) {
            int px = (int)(gx / sd->zoom);
            int py = (int)(gy / sd->zoom);
            if (px >= sd->frame_w) px = sd->frame_w - 1;
            if (py >= sd->frame_h) py = sd->frame_h - 1;

            Color col = GetImageColor(sd->image, fx + px, fy + py);

            if (col.a == 0) continue;
            if (sd->use_key_color &&
                abs(col.r - sd->key_color.r) <= sd->key_tolerance &&
                abs(col.g - sd->key_color.g) <= sd->key_tolerance &&
                abs(col.b - sd->key_color.b) <= sd->key_tolerance) continue;

            GridSetColor(grid, (Pos){gx + ox, gy + oy}, col);
        }
    }

    sd->tick++;
    if (sd->tick >= sd->anim_speed) {
        sd->tick = 0;
        sd->current_frame = (sd->current_frame + 1) % sd->num_frames;
    }
}

static void Destroy(void *data) {
    SpritesheetData *sd = (SpritesheetData *)data;
    UnloadImage(sd->image);
    free(sd->frames);
    free(sd);
}

Design spritesheetDesign = {
    .name = "spritesheet",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy,
};
