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

#define FIELD_W 10
#define FIELD_H 16
#define FIELD_X 6
#define NUM_PIECES 7
#define DROP_TICKS 4

// Each piece: 4 rotations, each a 4x4 bitmap stored as 4 rows of 4 bits
static const uint16_t pieces[NUM_PIECES][4] = {
    // I
    { 0x0F00, 0x2222, 0x00F0, 0x4444 },
    // O
    { 0x6600, 0x6600, 0x6600, 0x6600 },
    // T
    { 0x0E40, 0x4C40, 0x4E00, 0x4640 },
    // S
    { 0x06C0, 0x8C40, 0x6C00, 0x4620 },
    // Z
    { 0x0C60, 0x4C80, 0xC600, 0x2640 },
    // L
    { 0x0E80, 0xC440, 0x2E00, 0x44C0 },
    // J
    { 0x0E20, 0x44C0, 0x8E00, 0xC880 },
};

static const Color piece_colors[NUM_PIECES] = {
    {85, 255, 255, 255},   // I - Cyan
    {255, 255, 85, 255},   // O - Yellow
    {255, 85, 255, 255},   // T - Magenta
    {85, 255, 85, 255},    // S - Green
    {255, 85, 85, 255},    // Z - Red
    {255, 170, 0, 255},    // L - Orange
    {85, 85, 255, 255},    // J - Blue
};

static bool PieceCell(int type, int rot, int r, int c) {
    uint16_t bits = pieces[type][rot];
    int idx = r * 4 + c;
    return (bits >> (15 - idx)) & 1;
}

typedef struct {
    int field[FIELD_H][FIELD_W];
    int field_color[FIELD_H][FIELD_W];
    int piece, rotation;
    int piece_x, piece_y;
    int tick;
    int target_x, target_rot;
    bool colorful;
    Color color;
} TetrisData;

static bool Collides(TetrisData *td, int type, int rot, int px, int py) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (!PieceCell(type, rot, r, c)) continue;
            int fx = px + c;
            int fy = py + r;
            if (fx < 0 || fx >= FIELD_W || fy >= FIELD_H) return true;
            if (fy >= 0 && td->field[fy][fx]) return true;
        }
    }
    return false;
}

static int FieldHeight(TetrisData *td) {
    int h = 0;
    for (int c = 0; c < FIELD_W; c++) {
        for (int r = 0; r < FIELD_H; r++) {
            if (td->field[r][c]) {
                int col_h = FIELD_H - r;
                if (col_h > h) h = col_h;
                break;
            }
        }
    }
    return h;
}

static int CountHoles(TetrisData *td) {
    int holes = 0;
    for (int c = 0; c < FIELD_W; c++) {
        bool found_block = false;
        for (int r = 0; r < FIELD_H; r++) {
            if (td->field[r][c]) found_block = true;
            else if (found_block) holes++;
        }
    }
    return holes;
}

static int CountCompleteRows(TetrisData *td) {
    int count = 0;
    for (int r = 0; r < FIELD_H; r++) {
        bool full = true;
        for (int c = 0; c < FIELD_W; c++) {
            if (!td->field[r][c]) { full = false; break; }
        }
        if (full) count++;
    }
    return count;
}

static void AIChoose(TetrisData *td) {
    int best_score = -100000;
    int best_x = td->piece_x;
    int best_rot = td->rotation;

    for (int rot = 0; rot < 4; rot++) {
        for (int x = -2; x < FIELD_W; x++) {
            if (Collides(td, td->piece, rot, x, td->piece_y)) continue;
            // Simulate drop
            int y = td->piece_y;
            while (!Collides(td, td->piece, rot, x, y + 1)) y++;
            // Temporarily place piece
            for (int r = 0; r < 4; r++)
                for (int c = 0; c < 4; c++)
                    if (PieceCell(td->piece, rot, r, c) && y + r >= 0 && y + r < FIELD_H && x + c >= 0 && x + c < FIELD_W)
                        td->field[y + r][x + c] = 1;
            int complete = CountCompleteRows(td);
            int height = FieldHeight(td);
            int holes = CountHoles(td);
            int score = complete * 100 - height * 10 - holes * 30;
            // Remove piece
            for (int r = 0; r < 4; r++)
                for (int c = 0; c < 4; c++)
                    if (PieceCell(td->piece, rot, r, c) && y + r >= 0 && y + r < FIELD_H && x + c >= 0 && x + c < FIELD_W)
                        td->field[y + r][x + c] = 0;
            if (score > best_score) {
                best_score = score;
                best_x = x;
                best_rot = rot;
            }
        }
    }
    td->target_x = best_x;
    td->target_rot = best_rot;
}

static void SpawnPiece(TetrisData *td) {
    td->piece = rand() % NUM_PIECES;
    td->rotation = 0;
    td->piece_x = FIELD_W / 2 - 2;
    td->piece_y = -1;
    AIChoose(td);
}

static void ClearRows(TetrisData *td) {
    for (int r = FIELD_H - 1; r >= 0; r--) {
        bool full = true;
        for (int c = 0; c < FIELD_W; c++) {
            if (!td->field[r][c]) { full = false; break; }
        }
        if (full) {
            for (int rr = r; rr > 0; rr--) {
                for (int c = 0; c < FIELD_W; c++) {
                    td->field[rr][c] = td->field[rr - 1][c];
                    td->field_color[rr][c] = td->field_color[rr - 1][c];
                }
            }
            for (int c = 0; c < FIELD_W; c++) {
                td->field[0][c] = 0;
                td->field_color[0][c] = 0;
            }
            r++; // re-check this row
        }
    }
}

static void PrintHelp() {
    printf("  -C               Enable colored mode\n");
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    (void)grid;
    TetrisData *td = calloc(1, sizeof(TetrisData));
    if (!td) return NULL;

    td->color = GREEN;
    td->colorful = false;

    int opt;
    optind = 1;
    while ((opt = getopt(argc, argv, ":d:C")) != -1) {
        switch (opt) {
            case 'C': td->colorful = true; break;
        }
    }

    srand(time(NULL));
    SpawnPiece(td);
    return td;
}

static void UpdateFrame(Grid *grid, void *data) {
    TetrisData *td = (TetrisData *)data;
    Color bg = grid->conf.backgroundColor;

    td->tick++;
    if (td->tick < DROP_TICKS) return;
    td->tick = 0;

    // Move piece toward target rotation and x
    if (td->rotation != td->target_rot) {
        int next_rot = (td->rotation + 1) % 4;
        if (!Collides(td, td->piece, next_rot, td->piece_x, td->piece_y))
            td->rotation = next_rot;
    }
    if (td->piece_x < td->target_x) {
        if (!Collides(td, td->piece, td->rotation, td->piece_x + 1, td->piece_y))
            td->piece_x++;
    } else if (td->piece_x > td->target_x) {
        if (!Collides(td, td->piece, td->rotation, td->piece_x - 1, td->piece_y))
            td->piece_x--;
    }

    // Drop
    if (!Collides(td, td->piece, td->rotation, td->piece_x, td->piece_y + 1)) {
        td->piece_y++;
    } else {
        // Lock piece
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; c++) {
                if (!PieceCell(td->piece, td->rotation, r, c)) continue;
                int fy = td->piece_y + r;
                int fx = td->piece_x + c;
                if (fy >= 0 && fy < FIELD_H && fx >= 0 && fx < FIELD_W) {
                    td->field[fy][fx] = 1;
                    td->field_color[fy][fx] = td->piece;
                }
            }
        }
        ClearRows(td);
        SpawnPiece(td);
        // If new piece immediately collides, reset
        if (Collides(td, td->piece, td->rotation, td->piece_x, td->piece_y)) {
            memset(td->field, 0, sizeof(td->field));
            memset(td->field_color, 0, sizeof(td->field_color));
            SpawnPiece(td);
        }
    }

    // Draw
    GridFillColor(grid, bg);

    // Draw field
    for (int r = 0; r < FIELD_H; r++) {
        for (int c = 0; c < FIELD_W; c++) {
            if (td->field[r][c]) {
                Color col = td->colorful ? piece_colors[td->field_color[r][c]] : td->color;
                GridSetColor(grid, (Pos){FIELD_X + c, r}, col);
            }
        }
    }

    // Draw current piece
    Color pc = td->colorful ? piece_colors[td->piece] : td->color;
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (!PieceCell(td->piece, td->rotation, r, c)) continue;
            int gy = td->piece_y + r;
            int gx = td->piece_x + c;
            if (gy >= 0 && gy < FIELD_H && gx >= 0 && gx < FIELD_W)
                GridSetColor(grid, (Pos){FIELD_X + gx, gy}, pc);
        }
    }

    // Draw field borders
    Color border = td->colorful ? (Color){170, 170, 170, 255} : td->color;
    for (int r = 0; r < FIELD_H; r++) {
        GridSetColor(grid, (Pos){FIELD_X - 1, r}, border);
        GridSetColor(grid, (Pos){FIELD_X + FIELD_W, r}, border);
    }
}

static void Destroy(void *data) {
    free(data);
}

Design tetrisDesign = {
    .name = "tetris",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy,
};
