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
    Direction direction;
    int fontSize;
    int velocity;
    const char* text;
    Color color;
} TextConf;

static TextConf defaultConf = {
    .direction = HORIZONTAL,    
    .fontSize = 14,
    .text = "TEXT",
    .color = WHITE,
    .velocity = 1,
};

typedef struct {
    TextConf conf;
    int font;
    int position;
} TextState;

static void ParseOptions(TextConf *conf, int argc, char *argv[]) {
    int opt;
    
    while ((opt = getopt(argc, argv, ":t:f:T:d:")) != -1) {
        switch (opt) {
            case 't': conf->text = optarg; break;
            case 'f': conf->fontSize = atoi(optarg); break;
            case 'T': conf->color = ParseColor(optarg); break;
            case 'd': conf->direction = ParseDirection(optarg); break;
        }
    }
}

static void PrintHelp() {
    printf("  -t <text>        Set text (default: %s)\n", defaultConf.text);
    printf("  -f <font size>   Set font size (default: %d)\n", defaultConf.fontSize);
    printf("  -T <color>       Set text color (R,G,B default: %d,%d,%d)\n", defaultConf.color.r, defaultConf.color.g, defaultConf.color.b); 
    printf("  -d <direction>   Set initial direction (0: horizontal, 1: vertical default: %d)\n", defaultConf.direction);
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    TextState *state = (TextState *)malloc(sizeof(TextState));
    if (!state) return NULL;
    
    memset(state, 0, sizeof(TextState));
    state->conf = defaultConf;
    ParseOptions(&state->conf, argc, argv);
    state->font = 44;
    return state;
}

static void draw_glyph_offset(Grid *grid, char current_char, int font, Pos pos, Pos offset) {
    for (int y = offset.y; y < fonts.size; y++) {
        for (int x = offset.x; x < fonts.size; x++) {
            Pos src =  { (current_char - 32) * 8, font * fonts.size };
            Color c = GetImageColor(fonts.image, src.x + x, src.y + y);
            Pos dst = { pos.x + x - offset.x, pos.y + y - offset.y };

            if (dst.x < grid->cols && dst.y < grid->rows) {
                GridSetColor(grid, dst, c);
            }
        }
    }
}

static void UpdateFrame(Grid *grid, void *data) {
    TextState *state = (TextState *)data;

    bool horizontal = state->conf.direction == HORIZONTAL;
    int char_pos = state->position / fonts.size;
    int char_offset = state->position % fonts.size;
    char text_len = strlen(state->conf.text);

    Pos pos = {
        horizontal ? 0 : (grid->cols - fonts.size) / 2,
        horizontal ? (grid->rows - fonts.size) / 2 : 0,
    };

    Pos offset = { 
        horizontal ? char_offset : 0, 
        horizontal ? 0 : char_offset,
    };

    int wrap_size = state->conf.direction == HORIZONTAL ? grid->rows : grid->cols;
    int offset_for_wrap = (wrap_size / fonts.size) + 1;
    char_pos %= text_len + offset_for_wrap;
    char_pos -= offset_for_wrap;

    GridFillColor(grid, grid->conf.backgroundColor);

    if (char_pos == -offset_for_wrap && char_offset == 0) {
        // change font every time the text wraps around
        state->font = rand() % fonts.count;
    }

    while ((pos.x < grid->cols) && (pos.y < grid->rows) && (char_pos < text_len)) {
        char current_char = (char_pos < 0) ? ' ' : state->conf.text[char_pos];
        draw_glyph_offset(grid, current_char, state->font, pos, offset);

        pos.x += horizontal ? fonts.size : 0;
        pos.y += horizontal ? 0 : fonts.size;
        char_pos++;

        pos.x -= offset.x;
        pos.y -= offset.y;
        offset = (Pos){ 0, 0 };
    }
    
    state->position += state->conf.velocity;
}

static void Destroy(void *data) {
    free(data);
}

Design arcadeDesign = {
    .name = "text-arcade",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy,
};