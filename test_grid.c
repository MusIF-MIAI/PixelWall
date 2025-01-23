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

#include <stdio.h>   // Standard I/O functions
#include <stdlib.h>  // Standard library functions
#include <signal.h>  // Add this for signal handling
#include "raylib.h"  // Raylib graphics library

// Default configuration values
#define DEFAULT_ROWS 16
#define DEFAULT_COLS 22
#define DEFAULT_BORDER_SIZE 2
#define DEFAULT_GRID_COLOR BLUE
#define DEFAULT_DEACTIVE_COLOR BLACK
#define DEFAULT_WINDOW_WIDTH 1280  // 720p width
#define DEFAULT_WINDOW_HEIGHT 720  // 720p height

// Global configuration variables
int ROWS = DEFAULT_ROWS;
int COLS = DEFAULT_COLS;
int BORDER_SIZE = DEFAULT_BORDER_SIZE;
Color GRID_COLOR = DEFAULT_GRID_COLOR;
Color DEACTIVE_COLOR = DEFAULT_DEACTIVE_COLOR;
int WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH;
int WINDOW_HEIGHT = DEFAULT_WINDOW_HEIGHT;

// Predefined color palette
Color colorPalette[] = {
    (Color){255, 0, 0, 255},     // Red
    (Color){0, 255, 0, 255},     // Green
    (Color){0, 0, 255, 255},     // Blue
    (Color){255, 255, 0, 255},   // Yellow
    (Color){255, 0, 255, 255},   // Magenta
    (Color){0, 255, 255, 255}    // Cyan
};
int paletteSize = 6;

// Structure representing a single grid pixel
typedef struct {
    Color color;  // Current color of the pixel
} Pixel;

// 2D array representing the pixel grid
Pixel **grid = NULL;

// Function to get a color that contrasts with neighbors
Color GetContrastingColor(int row, int col) {
    Color neighbors[4];  // Array to store colors of neighboring pixels
    int neighborCount = 0;

    // Check the colors of the 4 neighboring pixels (top, bottom, left, right)
    if (row > 0) neighbors[neighborCount++] = grid[row - 1][col].color;
    if (row < ROWS - 1) neighbors[neighborCount++] = grid[row + 1][col].color;
    if (col > 0) neighbors[neighborCount++] = grid[row][col - 1].color;
    if (col < COLS - 1) neighbors[neighborCount++] = grid[row][col + 1].color;

    // Find a color that is not used by any neighbor
    for (int i = 0; i < paletteSize; i++) {
        bool colorAvailable = true;
        for (int j = 0; j < neighborCount; j++) {
            if (colorPalette[i].r == neighbors[j].r &&
                colorPalette[i].g == neighbors[j].g &&
                colorPalette[i].b == neighbors[j].b) {
                colorAvailable = false;
                break;
            }
        }
        if (colorAvailable) {
            return colorPalette[i];
        }
    }

    // If all colors are used by neighbors, default to the first color
    return colorPalette[0];
}

// Initialize the grid with colors from the palette
void InitGrid() {
    // Allocate memory for rows
    grid = (Pixel **)malloc(ROWS * sizeof(Pixel *));
    if (!grid) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for columns
    for (int i = 0; i < ROWS; i++) {
        grid[i] = (Pixel *)malloc(COLS * sizeof(Pixel));
        if (!grid[i]) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
    }

    // Fill the grid with colors, ensuring no two neighbors share the same color
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            grid[row][col].color = GetContrastingColor(row, col);
        }
    }
}

// Calculate the dimensions of each grid cell based on window size
void CalculateCellDimensions(int *cellWidth, int *cellHeight) {
    *cellWidth = WINDOW_WIDTH / COLS;
    *cellHeight = WINDOW_HEIGHT / ROWS;
}

// Draw the grid of rectangles
void DrawArtRectangles() {
    int cellWidth, cellHeight;
    CalculateCellDimensions(&cellWidth, &cellHeight);
    
    // Draw each cell in the grid
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            // Draw the cell border
            DrawRectangle(col * cellWidth, row * cellHeight, 
                         cellWidth, cellHeight, GRID_COLOR);
            
            // Draw the inner colored rectangle
            DrawRectangle(col * cellWidth + BORDER_SIZE, 
                         row * cellHeight + BORDER_SIZE, 
                         cellWidth - 2*BORDER_SIZE, 
                         cellHeight - 2*BORDER_SIZE, 
                         grid[row][col].color);
        }
    }
}

// Add cleanup function
void CleanupGrid() {
    if (grid) {
        for (int i = 0; i < ROWS; i++) {
            free(grid[i]);
        }
        free(grid);
    }
}

// Function to handle SIGINT (Ctrl+C)
void HandleSIGINT(int signal) {
    if (signal == SIGINT) {
        CleanupGrid();
        CloseWindow();
        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char *argv[]) {
    // Set up signal handler for Ctrl+C
    signal(SIGINT, HandleSIGINT);
 
    // Initialize the window and grid
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Contrasting Grid");
    InitGrid();
    SetTargetFPS(60);

    // Main program loop
    while (!WindowShouldClose()) {
        // Begin drawing
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawArtRectangles();
        EndDrawing();
    }

    // Clean up and exit
    CleanupGrid();
    CloseWindow();
    return 0;
}