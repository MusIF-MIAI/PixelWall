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

// Add Pixel struct definition
typedef struct {
    Color color;
    bool isWorm;
    bool isFruit;
} Pixel;

// Game objects
typedef struct {
    Vector2 position;
    Color color;
} Segment;

typedef struct {
    Vector2 position;
    Color color;
} Fruit;

typedef struct {
    Segment *body;      // Pointer to dynamically allocated array
    int length;         // Current length of the worm
    int max_length;     // Maximum allocated length
    Vector2 direction;  // Current movement direction
} Worm;

// Main game state structure
typedef struct {
    Worm worm;           // The snake/worm object
    Fruit fruit;         // The fruit object
    float moveTimer;     // Timer for movement updates
    Vector2 currentDir;  // Current movement direction
    bool gameOver;       // Game over flag
} GameState;

// Update configuration variables
extern int WORM_LENGTH;
extern int MAX_WORM_LENGTH;
extern float MOVE_INTERVAL;
extern Color BACKGROUND_COLOR;
extern Color WORM_COLOR;

typedef struct {
    int rows;
    int cols;
    Pixel **grid;
} Grid;

void DesignInit(Grid *grid, GameState *state);
void DesignUpdateFrame(Grid *grid, GameState *state);
void DesignCleanup(GameState *state);

void InitializeGrid(Grid *grid);
Color GetRandomColor();
Vector2 GetRandomDirection();
