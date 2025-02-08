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

#define HORIZONTAL 0
#define VERTICAL 1

typedef struct {
    int direction;
    int fontSize;
    const char* text;
    Color color;
} TextConf;

static TextConf defaultConf = {
    .direction = HORIZONTAL,    
    .fontSize = 14,
    .text = "text",
    .color = WHITE,
};

typedef struct {
    TextConf conf;
    Vector2 position;
    Vector2 velocity;
} TextState;

static void ParseOptions(TextConf *conf, int argc, char *argv[]) {
    int opt;
    
    // Dump argv
    printf("Command line arguments:\n");
    for (int i = 0; i < argc; i++) {
        printf("argv[%d]: %s\n", i, argv[i]);
    }

    while ((opt = getopt(argc, argv, ":t:f:T:d:")) != -1) {
        printf(">> %c\n", opt);
        switch (opt) {
            case 't': conf->text = optarg; break;
            case 'f': conf->fontSize = atoi(optarg); break;
            case 'T': conf->color = ParseColor(optarg); break;
            case 'd': 
                conf->direction = atoi(optarg);
                if (conf->direction != HORIZONTAL && conf->direction != VERTICAL) {
                    fprintf(stderr, "Invalid direction. Use 0 for horizontal, 1 for vertical\n");
                    exit(EXIT_FAILURE);
                }
                break;

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

    state->conf = defaultConf;
    ParseOptions(&state->conf, argc, argv);

    return state;
}

static void RenderTextToGrid(Grid *grid, TextState *state) {
    // Create an image from text using the new API
    Image textImage = GenImageColor(grid->cols, grid->rows, BLANK);
    
    if (state->conf.direction == HORIZONTAL) {
        // Draw horizontal text
        ImageDrawText(
            &textImage, 
            state->conf.text, 
            state->position.x,
            state->position.y, 
            state->conf.fontSize,
            state->conf.color
        );
    } else {
        // Draw vertical text
        int yOffset = 0;
        for (int i = 0; i < strlen(state->conf.text); i++) {
            char singleChar[2] = { state->conf.text[i], '\0'};
            ImageDrawText(
                &textImage,
                singleChar,
                state->position.x,
                state->position.y + yOffset,
                state->conf.fontSize,
                state->conf.color
            );
            yOffset += state->conf.fontSize;
        }
    }
    

    // Convert image to grid
    for (int y = 0; y < grid->rows; y++) {
        for (int x = 0; x < grid->cols; x++) {
            Color textColor = GetImageColor(textImage, x, y);
            Color gridColor = textColor.a > 0 ? state->conf.color : grid->conf.backgroundColor;
            GridSetColor(grid, (Pos){x, y}, gridColor);
        }
    }

    UnloadImage(textImage);
}

static void UpdateTextPosition(Grid *grid, TextState* state) {
    if (state->conf.direction == HORIZONTAL) {
        state->position.x -= 1;
        int textWidth = MeasureText(state->conf.text, state->conf.fontSize);
        if (state->position.x + textWidth < 0) {
            state->position.x = grid->cols;
        }
    } else {
        state->position.y -= 1;
        int textHeight = strlen(state->conf.text) * state->conf.fontSize;
        if (state->position.y + textHeight < 0) {
            state->position.y = grid->rows;
        }
    }
}

static void UpdateFrame(Grid *grid, void *data) {
    TextState *state = (TextState *)data;
    UpdateTextPosition(grid, state);
    GridFillColor(grid, grid->conf.backgroundColor);
    RenderTextToGrid(grid, state);
}

static void Destroy(void *data) {
    free(data);
}

Design textDesign = {
    .name = "text",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy,
};