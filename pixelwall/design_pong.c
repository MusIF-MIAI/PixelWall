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
    int rows;
    int cols;
    int margin;
    Pos ball;
    Pos direction;
    int paddle1_y; // Position of the left paddle (y-coordinate)
    int paddle2_y; // Position of the right paddle (y-coordinate)
    int prev_paddle1_y; // Previous position of the left paddle
    int prev_paddle2_y; // Previous position of the right paddle
} PongData;

static void PongPrintHelp() {
    printf("Simple Pong animation. Config options: rows, cols, frameRate.\n");
}

static void* PongCreate(Grid *grid, int argc, char *argv[]) {
    PongData *pd = malloc(sizeof(PongData));
    pd->margin = 1;

    pd->rows = grid->conf.rows - 2 * pd->margin;
    pd->cols = grid->conf.cols - 2 * pd->margin;

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
    Pos prev_ball = {pd->margin + pd->ball.x, pd->margin + pd->ball.y};
    GridSetColor(grid, prev_ball, (Color){0, 0, 0, 255}); // Black with full opacity

    // Clear the previous paddle positions
    for (int i = -1; i <= 1; i++) {
        Pos paddle1_pos = {pd->margin, pd->margin + pd->prev_paddle1_y + i};
        Pos paddle2_pos = {pd->margin + pd->cols - 1, pd->margin + pd->prev_paddle2_y + i};
        GridSetColor(grid, paddle1_pos, (Color){0, 0, 0, 255}); // Clear left paddle
        GridSetColor(grid, paddle2_pos, (Color){0, 0, 0, 255}); // Clear right paddle
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
    if (pd->ball.x == 1 && pd->ball.y >= pd->paddle1_y - 1 && pd->ball.y <= pd->paddle1_y + 1) {
        pd->direction.x *= -1; // Reverse x-direction (left paddle)
    }
    if (pd->ball.x == pd->cols - 2 && pd->ball.y >= pd->paddle2_y - 1 && pd->ball.y <= pd->paddle2_y + 1) {
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
    if (pd->paddle1_y < 1) pd->paddle1_y = 1;
    if (pd->paddle1_y >= pd->rows - 1) pd->paddle1_y = pd->rows - 2;
    if (pd->paddle2_y < 1) pd->paddle2_y = 1;
    if (pd->paddle2_y >= pd->rows - 1) pd->paddle2_y = pd->rows - 2;

    // Draw paddles
    for (int i = -1; i <= 1; i++) {
        Pos paddle1_pos = {pd->margin, pd->margin + pd->paddle1_y + i};
        Pos paddle2_pos = {pd->margin + pd->cols - 1, pd->margin + pd->paddle2_y + i};
        GridSetColor(grid, paddle1_pos, (Color){255, 255, 255, 255}); // White paddles with full opacity
        GridSetColor(grid, paddle2_pos, (Color){255, 255, 255, 255});
    }

    // Draw ball
    Pos ball_pos = {pd->margin + pd->ball.x, pd->margin + pd->ball.y};
    GridSetColor(grid, ball_pos, (Color){255, 0, 0, 255}); // Red ball with full opacity

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