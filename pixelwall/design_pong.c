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
    int paddle_len;
    int margin;
    Color paddle_color;
    Color ball_color;
} PongConf;

static PongConf defaultConf = {
    .paddle_len = 1,
    .margin = 1,
    .paddle_color = WHITE,
    .ball_color = RED,
};

typedef struct {
    PongConf conf;
    int rows;
    int cols;
    Pos ball;
    Pos direction;
    int paddle1_y; // Position of the left paddle (y-coordinate)
    int paddle2_y; // Position of the right paddle (y-coordinate)
    int prev_paddle1_y; // Previous position of the left paddle
    int prev_paddle2_y; // Previous position of the right paddle
} PongData;

static void PongPrintHelp() {
    printf("  -M <margin>      Margin around playfield (default: %d)\n", defaultConf.margin);
    printf("  -P <length>      Paddle length (default: %d)\n", defaultConf.paddle_len);
    printf("  -p <paddle>      Paddle color (R,G,B, default: %d,%d,%d)\n",
           defaultConf.paddle_color.r, defaultConf.paddle_color.g, defaultConf.paddle_color.b);
    printf("  -o <ball>        Ball color (R,G,B, default: %d,%d,%d)\n",
           defaultConf.ball_color.r, defaultConf.ball_color.g, defaultConf.ball_color.b);
}

static void ParseOptions(PongConf *conf, int argc, char *argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, ":d:M:P:p:o:")) != -1) {
        switch (opt) {
            case 'M': conf->margin = atoi(optarg); break;
            case 'P': conf->paddle_len = atoi(optarg); break;
            case 'p': conf->paddle_color = ParseColor(optarg); break;
            case 'o': conf->ball_color = ParseColor(optarg); break;
        }
    }
}

static void* PongCreate(Grid *grid, int argc, char *argv[]) {
    PongData *pd = malloc(sizeof(PongData));
    if (!pd) return NULL;
    pd->conf = defaultConf;
    ParseOptions(&pd->conf, argc, argv);

    pd->rows = grid->conf.rows - 2 * pd->conf.margin;
    pd->cols = grid->conf.cols - 2 * pd->conf.margin;

    // Initialize paddles and ball
    pd->paddle1_y = pd->rows / 2;
    pd->paddle2_y = pd->rows / 2;
    pd->ball = (Pos) { pd->cols / 2, pd->rows / 2 };
    pd->direction = (Pos) {1, 1};

    // Initialize previous paddle positions
    pd->prev_paddle1_y = pd->paddle1_y;
    pd->prev_paddle2_y = pd->paddle2_y;

    srand(time(NULL));
    return pd;
}

static void PongUpdateFrame(Grid *grid, void *data) {
    PongData *pd = (PongData*)data;

    // Clear the previous ball position
    Pos prev_ball = {pd->conf.margin + pd->ball.x, pd->conf.margin + pd->ball.y};
    GridSetColor(grid, prev_ball, grid->conf.backgroundColor);

    // Clear the previous paddle positions
    for (int i = -pd->conf.paddle_len; i <= pd->conf.paddle_len; i++) {
        Pos paddle1_pos = {pd->conf.margin, pd->conf.margin + pd->prev_paddle1_y + i};
        Pos paddle2_pos = {pd->conf.margin + pd->cols - 1, pd->conf.margin + pd->prev_paddle2_y + i};
        GridSetColor(grid, paddle1_pos, grid->conf.backgroundColor);
        GridSetColor(grid, paddle2_pos, grid->conf.backgroundColor);
    }

    // Update ball position
    pd->ball.x += pd->direction.x;
    pd->ball.y += pd->direction.y;

    // Ball collision with top and bottom walls
    if (pd->ball.y < 0 || pd->ball.y >= pd->rows) {
        pd->direction.y *= -1; // Reverse y-direction
        pd->ball.y += pd->direction.y; // Move ball back inside the grid
    }

    // Ball collision with paddles
    if (pd->ball.x == 1 && pd->ball.y >= pd->paddle1_y - pd->conf.paddle_len && pd->ball.y <= pd->paddle1_y + pd->conf.paddle_len) {
        pd->direction.x *= -1; // Reverse x-direction (left paddle)
    }
    if (pd->ball.x == pd->cols - 2 && pd->ball.y >= pd->paddle2_y - pd->conf.paddle_len && pd->ball.y <= pd->paddle2_y + pd->conf.paddle_len) {
        pd->direction.x *= -1; // Reverse x-direction (right paddle)
    }

    // Ball out of bounds (left or right)
    if (pd->ball.x < 0 || pd->ball.x >= pd->cols) {
        // Reset ball position
        pd->ball.x = pd->cols / 2;
        pd->ball.y = pd->rows / 2;
        pd->direction.x = (pd->ball.x < 0) ? 1 : -1; // Set direction based on which side it went out
    }

    // randomly increment or decrement paddles
    static int changes[] = { -1, 0, 1 };
    pd->paddle1_y += changes[rand() % 3];
    pd->paddle2_y += changes[rand() % 3];
    
    // Move paddles (simple AI or user input can be added here)
    if (pd->ball.y > pd->paddle1_y) pd->paddle1_y++;
    if (pd->ball.y < pd->paddle1_y) pd->paddle1_y--;
    if (pd->ball.y > pd->paddle2_y) pd->paddle2_y++;
    if (pd->ball.y < pd->paddle2_y) pd->paddle2_y--;

    // Ensure paddles stay within the grid
    if (pd->paddle1_y < pd->conf.paddle_len) pd->paddle1_y = pd->conf.paddle_len;
    if (pd->paddle1_y >= pd->rows - pd->conf.paddle_len) pd->paddle1_y = pd->rows - pd->conf.paddle_len - 1;
    if (pd->paddle2_y < pd->conf.paddle_len) pd->paddle2_y = pd->conf.paddle_len;
    if (pd->paddle2_y >= pd->rows - pd->conf.paddle_len) pd->paddle2_y = pd->rows - pd->conf.paddle_len - 1;

    // Draw paddles
    for (int i = -pd->conf.paddle_len; i <= pd->conf.paddle_len; i++) {
        Pos paddle1_pos = {pd->conf.margin, pd->conf.margin + pd->paddle1_y + i};
        Pos paddle2_pos = {pd->conf.margin + pd->cols - 1, pd->conf.margin + pd->paddle2_y + i};
        GridSetColor(grid, paddle1_pos, pd->conf.paddle_color);
        GridSetColor(grid, paddle2_pos, pd->conf.paddle_color);
    }

    // Draw ball
    Pos ball_pos = {pd->conf.margin + pd->ball.x, pd->conf.margin + pd->ball.y};
    GridSetColor(grid, ball_pos, pd->conf.ball_color);

    // Update previous paddle positions
    pd->prev_paddle1_y = pd->paddle1_y;
    pd->prev_paddle2_y = pd->paddle2_y;
}

static void PongDestroy(void *data) {
    free(data);
}

Design pongDesign = {
    .name = "pong",
    .PrintHelp = PongPrintHelp,
    .Create = PongCreate,
    .UpdateFrame = PongUpdateFrame,
    .Destroy = PongDestroy
};