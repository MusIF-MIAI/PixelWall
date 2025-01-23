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
#include <signal.h>  // Signal handling
#include <unistd.h>  // POSIX API for getopt
#include "raylib.h"  // Raylib graphics library

// Default configuration values
#define DEFAULT_ROWS 16
#define DEFAULT_COLS 22
#define DEFAULT_BORDER_SIZE 2
#define DEFAULT_GRID_COLOR BLUE
#define DEFAULT_DEACTIVE_COLOR BLACK
#define MAX_COLOR_VALUE 255  // Maximum value for RGB color components
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

// Function to parse command line arguments
void ParseCommandLine(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "hr:c:b:g:d:w:H:")) != -1) {
        switch (opt) {
            case 'h':  // Help option - display usage information and exit
                printf("Contrasting Grid - Visual Art Generator\n\n");
                printf("Usage: %s [options]\n\n", argv[0]);
                printf("Options:\n");
                printf("  -h            Show this help message\n");
                printf("  -r ROWS       Number of rows (default: %d)\n", DEFAULT_ROWS);
                printf("  -c COLS       Number of columns (default: %d)\n", DEFAULT_COLS);
                printf("  -b BORDER     Border size in pixels (default: %d)\n", DEFAULT_BORDER_SIZE);
                printf("  -g R,G,B      Grid color (default: %d,%d,%d)\n", 
                       DEFAULT_GRID_COLOR.r, DEFAULT_GRID_COLOR.g, DEFAULT_GRID_COLOR.b);
                printf("  -d R,G,B      Deactive color (default: %d,%d,%d)\n\n",
                       DEFAULT_DEACTIVE_COLOR.r, DEFAULT_DEACTIVE_COLOR.g, DEFAULT_DEACTIVE_COLOR.b);
                printf("  -w WIDTH      Window width (default: %d)\n", DEFAULT_WINDOW_WIDTH);
                printf("  -H HEIGHT     Window height (default: %d)\n", DEFAULT_WINDOW_HEIGHT);
                printf("Example:\n");
                printf("  %s -r 20 -c 30 -w 1280 -H 720 -b 3 -g 255,0,0 -d 0,0,0\n", argv[0]);
                exit(EXIT_SUCCESS);
            
            case 'r':  // Set number of rows
                ROWS = atoi(optarg);
                if (ROWS <= 0) {
                    fprintf(stderr, "Number of rows must be positive\n");
                    exit(EXIT_FAILURE);
                }
                break;
            
            case 'c':  // Set number of columns
                COLS = atoi(optarg); 
                if (COLS <= 0) {
                    fprintf(stderr, "Number of columns must be positive\n");
                    exit(EXIT_FAILURE);
                }
                break;
            
            case 'b':  // Set border size
                BORDER_SIZE = atoi(optarg); 
                if (BORDER_SIZE < 0) {
                    fprintf(stderr, "Border size cannot be negative\n");
                    exit(EXIT_FAILURE);
                }
                break;
            
            case 'g': {  // Set grid color
                int r, g, b;
                if (sscanf(optarg, "%d,%d,%d", &r, &g, &b) == 3) {
                    if (r < 0 || r > MAX_COLOR_VALUE || 
                        g < 0 || g > MAX_COLOR_VALUE || 
                        b < 0 || b > MAX_COLOR_VALUE) {
                        fprintf(stderr, "Color values must be between 0 and %d\n", MAX_COLOR_VALUE);
                        exit(EXIT_FAILURE);
                    }
                    GRID_COLOR = (Color){r, g, b, 255};
                } else {
                    fprintf(stderr, "Invalid color format for -g. Use R,G,B (e.g., 255,0,0)\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            
            case 'd': {  // Set deactive color
                int r, g, b;
                if (sscanf(optarg, "%d,%d,%d", &r, &g, &b) == 3) {
                    if (r < 0 || r > MAX_COLOR_VALUE || 
                        g < 0 || g > MAX_COLOR_VALUE || 
                        b < 0 || b > MAX_COLOR_VALUE) {
                        fprintf(stderr, "Color values must be between 0 and %d\n", MAX_COLOR_VALUE);
                        exit(EXIT_FAILURE);
                    }
                    DEACTIVE_COLOR = (Color){r, g, b, 255};
                } else {
                    fprintf(stderr, "Invalid color format for -d. Use R,G,B (e.g., 0,0,0)\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            
            case 'w':  // Set window width
                WINDOW_WIDTH = atoi(optarg);
                if (WINDOW_WIDTH <= 0) {
                    fprintf(stderr, "Window width must be positive\n");
                    exit(EXIT_FAILURE);
                }
                break;
                
            case 'H':  // Set window height
                WINDOW_HEIGHT = atoi(optarg);
                if (WINDOW_HEIGHT <= 0) {
                    fprintf(stderr, "Window height must be positive\n");
                    exit(EXIT_FAILURE);
                }
                break;
            
            default:  // Handle unknown options
                fprintf(stderr, "Usage: %s [-r rows] [-c cols] [-b border_size] "
                        "[-g grid_color] [-d deactive_color] [-w width] [-H height]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

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

    // Parse command line arguments
    ParseCommandLine(argc, argv);
 
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