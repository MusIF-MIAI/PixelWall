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

#define BRICK_ROWS 3
#define BRICK_COLS 11
#define PADDLE_ROW 14
#define DEFAULT_PADDLE_HALF 1

typedef struct {
    Color color;
    bool colorful;
    Color row_colors[BRICK_ROWS];
    Color ball_color;
    Color paddle_color;
    int paddle_half;
    Pos ball;
    Pos direction;
    int paddle_x;
    bool bricks[BRICK_ROWS][BRICK_COLS];
    int bricks_remaining;
} BreakoutData;

static void ResetBricks(BreakoutData *bd) {
    bd->bricks_remaining = BRICK_ROWS * BRICK_COLS;
    for (int r = 0; r < BRICK_ROWS; r++)
        for (int b = 0; b < BRICK_COLS; b++)
            bd->bricks[r][b] = true;
}

static void ResetBall(BreakoutData *bd) {
    bd->ball = (Pos){11, 8};
    bd->direction = (Pos){(rand() % 2) ? 1 : -1, -1};
}

// Get the left column of brick b (no gaps, all rows identical)
static int BrickCol(int r, int b) {
    (void)r;
    return b * 2;  // 0,2,4,6,8,10,12,14,16,18,20
}

// Check if column col hits a brick in row r, return brick index or -1
static int ColToBrick(int r, int col) {
    for (int b = 0; b < BRICK_COLS; b++) {
        int left = BrickCol(r, b);
        if (col == left || col == left + 1)
            return b;
    }
    return -1;
}

static void PrintHelp() {
    printf("  -C               Enable colored mode\n");
    printf("  -R <color>       Brick row 1 color (default: 255,85,85)\n");
    printf("  -G <color>       Brick row 2 color (default: 255,255,85)\n");
    printf("  -K <color>       Brick row 3 color (default: 85,255,255)\n");
    printf("  -L <color>       Ball color (default: 255,255,85)\n");
    printf("  -P <color>       Paddle color (default: 255,255,255)\n");
    printf("  -S <size>        Paddle size in pixels (default: %d)\n", DEFAULT_PADDLE_HALF * 2 + 1);
}

static void ParseOptions(BreakoutData *bd, int argc, char *argv[]) {
    int opt;
    optind = 1;
    while ((opt = getopt(argc, argv, ":d:CR:G:K:L:P:S:")) != -1) {
        switch (opt) {
            case 'C': bd->colorful = true; break;
            case 'R': bd->row_colors[0] = ParseColor(optarg); break;
            case 'G': bd->row_colors[1] = ParseColor(optarg); break;
            case 'K': bd->row_colors[2] = ParseColor(optarg); break;
            case 'L': bd->ball_color = ParseColor(optarg); break;
            case 'P': bd->paddle_color = ParseColor(optarg); break;
            case 'S': {
                int size = atoi(optarg);
                if (size < 1) size = 1;
                bd->paddle_half = (size - 1) / 2;
                break;
            }
        }
    }
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    BreakoutData *bd = malloc(sizeof(BreakoutData));
    if (!bd) return NULL;

    bd->color = GREEN;
    bd->colorful = false;
    // CGA palette colors (used when -c is enabled)
    bd->row_colors[0] = (Color){255, 85, 85, 255};    // CGA Light Red
    bd->row_colors[1] = (Color){255, 255, 85, 255};   // CGA Yellow
    bd->row_colors[2] = (Color){85, 255, 255, 255};   // CGA Light Cyan
    bd->ball_color = (Color){255, 255, 85, 255};       // CGA Yellow
    bd->paddle_color = (Color){255, 255, 255, 255};    // White
    bd->paddle_half = DEFAULT_PADDLE_HALF;

    ParseOptions(bd, argc, argv);
    ResetBricks(bd);

    srand(time(NULL));
    ResetBall(bd);
    bd->paddle_x = 11;

    return bd;
}

static void UpdateFrame(Grid *grid, void *data) {
    BreakoutData *bd = (BreakoutData *)data;
    Color bg = grid->conf.backgroundColor;

    GridFillColor(grid, bg);

    // Move ball
    int nx = bd->ball.x + bd->direction.x;
    int ny = bd->ball.y + bd->direction.y;

    // Wall bounce left/right
    if (nx < 0 || nx > 21) {
        bd->direction.x *= -1;
        nx = bd->ball.x + bd->direction.x;
    }

    // Wall bounce top
    if (ny < 0) {
        bd->direction.y *= -1;
        ny = bd->ball.y + bd->direction.y;
    }

    // Brick collision: check if new position hits a brick
    if (ny >= 0 && ny < BRICK_ROWS) {
        int b = ColToBrick(ny, nx);
        if (b >= 0 && bd->bricks[ny][b]) {
            bd->bricks[ny][b] = false;
            bd->bricks_remaining--;
            bd->direction.y *= -1;
            ny = bd->ball.y + bd->direction.y;
        }
    }

    // Paddle collision
    if (ny == PADDLE_ROW) {
        if (nx >= bd->paddle_x - bd->paddle_half && nx <= bd->paddle_x + bd->paddle_half) {
            bd->direction.y = -1;
            ny = bd->ball.y + bd->direction.y;
        }
    }

    // Ball lost below paddle
    if (ny > PADDLE_ROW) {
        ResetBall(bd);
        nx = bd->ball.x;
        ny = bd->ball.y;
    } else {
        bd->ball.x = nx;
        bd->ball.y = ny;
    }

    // Safety clamp
    if (bd->ball.x < 0) bd->ball.x = 0;
    if (bd->ball.x > 21) bd->ball.x = 21;
    if (bd->ball.y < 0) bd->ball.y = 0;
    if (bd->ball.y > PADDLE_ROW) bd->ball.y = PADDLE_ROW;

    // AI paddle: move toward ball with slight randomness
    int jitter = (rand() % 3) - 1;
    int target = bd->ball.x + jitter;
    if (bd->paddle_x < target) bd->paddle_x++;
    if (bd->paddle_x > target) bd->paddle_x--;
    if (bd->paddle_x < bd->paddle_half) bd->paddle_x = bd->paddle_half;
    if (bd->paddle_x > 21 - bd->paddle_half) bd->paddle_x = 21 - bd->paddle_half;

    // Draw bricks
    for (int r = 0; r < BRICK_ROWS; r++) {
        Color bc = bd->colorful ? bd->row_colors[r] : bd->color;
        for (int b = 0; b < BRICK_COLS; b++) {
            if (bd->bricks[r][b]) {
                int left = BrickCol(r, b);
                GridSetColor(grid, (Pos){left, r}, bc);
                GridSetColor(grid, (Pos){left + 1, r}, bc);
            }
        }
    }

    // Draw paddle
    Color pc = bd->colorful ? bd->paddle_color : bd->color;
    for (int i = -bd->paddle_half; i <= bd->paddle_half; i++) {
        GridSetColor(grid, (Pos){bd->paddle_x + i, PADDLE_ROW}, pc);
    }

    // Draw ball
    Color blc = bd->colorful ? bd->ball_color : bd->color;
    GridSetColor(grid, (Pos){bd->ball.x, bd->ball.y}, blc);

    // Reset if all bricks destroyed
    if (bd->bricks_remaining == 0) {
        ResetBricks(bd);
    }
}

static void Destroy(void *data) {
    free(data);
}

Design breakoutDesign = {
    .name = "breakout",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy,
};
