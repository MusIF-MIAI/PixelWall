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

// 3x6 seven-segment digit bitmaps (3 bits per row, MSB = col 0)
// Row layout: top bar, upper verticals, middle bar,
//             lower verticals, lower verticals, bottom bar
static const uint8_t digitFont[10][6] = {
    {0x7, 0x5, 0x5, 0x5, 0x5, 0x7}, // 0
    {0x2, 0x6, 0x2, 0x2, 0x2, 0x7}, // 1
    {0x7, 0x1, 0x7, 0x4, 0x4, 0x7}, // 2
    {0x7, 0x1, 0x7, 0x1, 0x1, 0x7}, // 3
    {0x5, 0x5, 0x7, 0x1, 0x1, 0x1}, // 4
    {0x7, 0x4, 0x7, 0x1, 0x1, 0x7}, // 5
    {0x7, 0x4, 0x4, 0x7, 0x5, 0x7}, // 6
    {0x7, 0x1, 0x1, 0x1, 0x1, 0x1}, // 7
    {0x7, 0x5, 0x7, 0x5, 0x5, 0x7}, // 8
    {0x7, 0x5, 0x7, 0x1, 0x1, 0x7}, // 9
};

#define DIGIT_W 3
#define DIGIT_H 6
#define DIGIT_TOP_ROW 5  // (16 - 6) / 2, vertically centered

// Horizontal column positions for HH:MM layout
// margin(2) + H1(3) + gap(1) + H0(3) + gap(1) + colon(2) + gap(1) + M1(3) + gap(1) + M0(3) + margin(2) = 22
#define COL_H1    2
#define COL_H0    6
#define COL_COLON 10
#define COL_M1    13
#define COL_M0    17

typedef struct {
    Color digitColor;
    Color markerColor;
} ClockConf;

static ClockConf defaultClockConf = {
    .digitColor = GREEN,
    .markerColor = GREEN,
};

static void ParseOptions(ClockConf *conf, int argc, char *argv[]) {
    int opt;
    optind = 1;
    while ((opt = getopt(argc, argv, ":d:T:S:")) != -1) {
        switch (opt) {
            case 'T': conf->digitColor = ParseColor(optarg); break;
            case 'S': conf->markerColor = ParseColor(optarg); break;
        }
    }
}

static void PrintHelp() {
    printf("  -T <color>       Digit color (default: green)\n");
    printf("  -S <color>       Seconds marker color (default: green)\n");
}

static void DrawDigit(Grid *grid, int digit, int startCol, Color color) {
    for (int r = 0; r < DIGIT_H; r++) {
        uint8_t row = digitFont[digit][r];
        for (int c = 0; c < DIGIT_W; c++) {
            if (row & (0x4 >> c)) {
                Pos pos = {startCol + c, DIGIT_TOP_ROW + r};
                GridSetColor(grid, pos, color);
            }
        }
    }
}

static void DrawColon(Grid *grid, int startCol, Color color, int sec) {
    // Animated 2x6 colon: two dots that alternate diagonal position
    // Even seconds: top-left (#.) and bottom-right (.#)
    // Odd seconds:  top-right (.#) and bottom-left (#.)
    if (sec % 2 == 0) {
        GridSetColor(grid, (Pos){startCol, DIGIT_TOP_ROW}, color);
        GridSetColor(grid, (Pos){startCol + 1, DIGIT_TOP_ROW + 5}, color);
    } else {
        GridSetColor(grid, (Pos){startCol + 1, DIGIT_TOP_ROW}, color);
        GridSetColor(grid, (Pos){startCol, DIGIT_TOP_ROW + 5}, color);
    }
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    if (grid->cols != 22 || grid->rows != 16) {
        fprintf(stderr, "clock: requires a 22x16 grid (got %dx%d)\n",
                grid->cols, grid->rows);
        return NULL;
    }
    ClockConf *conf = (ClockConf *)malloc(sizeof(ClockConf));
    if (!conf) return NULL;
    *conf = defaultClockConf;
    ParseOptions(conf, argc, argv);
    return conf;
}

static void UpdateFrame(Grid *grid, void *data) {
    ClockConf *conf = (ClockConf *)data;

    GridFillColor(grid, grid->conf.backgroundColor);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    int hour = t->tm_hour;
    int min = t->tm_min;
    int sec = t->tm_sec;

    // Draw digits
    DrawDigit(grid, hour / 10, COL_H1, conf->digitColor);
    DrawDigit(grid, hour % 10, COL_H0, conf->digitColor);
    DrawColon(grid, COL_COLON, conf->digitColor, sec);
    DrawDigit(grid, min / 10, COL_M1, conf->digitColor);
    DrawDigit(grid, min % 10, COL_M0, conf->digitColor);

    // Draw seconds marker: 12 evenly spaced positions around the perimeter
    // Corners use L-shaped 3 pixels, edges use 2 pixels
    int slot = sec / 5;
    Color mc = conf->markerColor;
    switch (slot) {
        case 0:  // :00 top center
            GridSetColor(grid, (Pos){10, 0}, mc);
            GridSetColor(grid, (Pos){11, 0}, mc);
            break;
        case 1:  // :05 top right
            GridSetColor(grid, (Pos){15, 0}, mc);
            GridSetColor(grid, (Pos){16, 0}, mc);
            break;
        case 2:  // :10 top-right corner
            GridSetColor(grid, (Pos){20, 0}, mc);
            GridSetColor(grid, (Pos){21, 0}, mc);
            GridSetColor(grid, (Pos){21, 1}, mc);
            break;
        case 3:  // :15 right middle
            GridSetColor(grid, (Pos){21, 6}, mc);
            GridSetColor(grid, (Pos){21, 7}, mc);
            break;
        case 4:  // :20 bottom-right corner
            GridSetColor(grid, (Pos){21, 14}, mc);
            GridSetColor(grid, (Pos){21, 15}, mc);
            GridSetColor(grid, (Pos){20, 15}, mc);
            break;
        case 5:  // :25 bottom right
            GridSetColor(grid, (Pos){15, 15}, mc);
            GridSetColor(grid, (Pos){16, 15}, mc);
            break;
        case 6:  // :30 bottom center
            GridSetColor(grid, (Pos){10, 15}, mc);
            GridSetColor(grid, (Pos){11, 15}, mc);
            break;
        case 7:  // :35 bottom left
            GridSetColor(grid, (Pos){5, 15}, mc);
            GridSetColor(grid, (Pos){6, 15}, mc);
            break;
        case 8:  // :40 bottom-left corner
            GridSetColor(grid, (Pos){1, 15}, mc);
            GridSetColor(grid, (Pos){0, 15}, mc);
            GridSetColor(grid, (Pos){0, 14}, mc);
            break;
        case 9:  // :45 left middle
            GridSetColor(grid, (Pos){0, 7}, mc);
            GridSetColor(grid, (Pos){0, 6}, mc);
            break;
        case 10: // :50 top-left corner
            GridSetColor(grid, (Pos){0, 1}, mc);
            GridSetColor(grid, (Pos){0, 0}, mc);
            GridSetColor(grid, (Pos){1, 0}, mc);
            break;
        case 11: // :55 top left
            GridSetColor(grid, (Pos){5, 0}, mc);
            GridSetColor(grid, (Pos){6, 0}, mc);
            break;
    }
}

static void Destroy(void *data) {
    free(data);
}

Design clockDesign = {
    .name = "clock",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy,
};
