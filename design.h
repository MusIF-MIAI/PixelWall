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

#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <string.h>  
#include <unistd.h>

#include "raylib.h"

typedef struct {
    float moveInterval;
    Color backgroundColor;
} Config;

typedef struct {
    Color color;
    uintptr_t data;
} Pixel;

typedef struct {
    int x;
    int y;
} Pos;

typedef struct {
    Config conf;
    int rows;
    int cols;
    Pixel **pixels;
} Grid;

void GridInitialize(Grid *grid, int rows, int cols);
void GridFillColor(Grid *grid, Color color);
void GridFillData(Grid *grid, uintptr_t data);

Color GridGetColor(const Grid *grid, Pos pos);
void GridSetColor(Grid *grid, Pos pos, Color color);
uintptr_t GridGetData(const Grid *grid, Pos pos);
void GridSetData(Grid *grid, Pos pos, uintptr_t data);
void GridCleanup(Grid *grid);

void *DesignCreate(Grid *grid, int argc, char *argv[]);
void DesignPrintHelp();
void DesignUpdateFrame(Grid *grid, void *design);
void DesignDestroy(void *design);

Color GetRandomColor();
Pos GetRandomDirection();
Color ParseColor(const char *string);
