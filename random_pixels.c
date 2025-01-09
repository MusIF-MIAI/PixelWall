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
#include <unistd.h>  // POSIX API for getopt
#include <stdlib.h>  // Standard library functions
#include "raylib.h"  // Raylib graphics library

// Default configuration values
#define DEFAULT_ROWS 16
#define DEFAULT_COLS 22
#define DEFAULT_BORDER_SIZE 2
#define DEFAULT_TICK_INTERVAL 0.5f
#define DEFAULT_INITIAL_RANDOM_SQUARES 10
#define DEFAULT_MAX_RANDOM_TICK 100
#define DEFAULT_GRID_COLOR BLUE
#define DEFAULT_DEACTIVE_COLOR BLACK
#define MAX_COLOR_VALUE 255  // Maximum value for RGB color components
#define DEFAULT_WINDOW_WIDTH 1280  // 720p width
#define DEFAULT_WINDOW_HEIGHT 720  // 720p height

// Global configuration variables
int ROWS = DEFAULT_ROWS;
int COLS = DEFAULT_COLS;
int BORDER_SIZE = DEFAULT_BORDER_SIZE;
float TICK_INTERVAL = DEFAULT_TICK_INTERVAL;
int INITIAL_RANDOM_SQUARES = DEFAULT_INITIAL_RANDOM_SQUARES;
int MAX_RANDOM_TICK = DEFAULT_MAX_RANDOM_TICK;
Color GRID_COLOR = DEFAULT_GRID_COLOR;
Color DEACTIVE_COLOR = DEFAULT_DEACTIVE_COLOR;
int WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH;
int WINDOW_HEIGHT = DEFAULT_WINDOW_HEIGHT;

// Function to parse command line arguments
void ParseCommandLine(int argc, char *argv[]) {
    int opt;
    // Loop through all command line options using getopt
    // The colon after each letter indicates the option requires an argument
    while ((opt = getopt(argc, argv, "hr:c:b:t:i:m:g:d:w:H:")) != -1) {
        switch (opt) {
            case 'h':  // Help option - display usage information and exit
                printf("Art Grid - Interactive Visual Art Generator\n\n");
                printf("Usage: %s [options]\n\n", argv[0]);
                printf("Options:\n");
                printf("  -h            Show this help message\n");
                printf("  -w WIDTH      Window width (default: %d)\n", DEFAULT_WINDOW_WIDTH);
                printf("  -H HEIGHT     Window height (default: %d)\n", DEFAULT_WINDOW_HEIGHT);
                printf("  -r ROWS       Number of rows (default: %d)\n", DEFAULT_ROWS);
                printf("  -c COLS       Number of columns (default: %d)\n", DEFAULT_COLS);
                printf("  -b BORDER     Border size in pixels (default: %d)\n", DEFAULT_BORDER_SIZE);
                printf("  -t INTERVAL   Tick interval in seconds (default: %.1f)\n", DEFAULT_TICK_INTERVAL);
                printf("  -i SQUARES    Initial active squares (default: %d)\n", DEFAULT_INITIAL_RANDOM_SQUARES);
                printf("  -m TICKS      Maximum random ticks (default: %d)\n", DEFAULT_MAX_RANDOM_TICK);
                printf("  -g R,G,B      Grid color (default: %d,%d,%d)\n", 
                       DEFAULT_GRID_COLOR.r, DEFAULT_GRID_COLOR.g, DEFAULT_GRID_COLOR.b);
                printf("  -d R,G,B      Deactive color (default: %d,%d,%d)\n\n",
                       DEFAULT_DEACTIVE_COLOR.r, DEFAULT_DEACTIVE_COLOR.g, DEFAULT_DEACTIVE_COLOR.b);
                printf("Example:\n");
                printf("  %s -r 20 -c 30 -w 1280 -H 720 -b 3 -t 5.0 -i 15 -m 200 -g 255,0,0 -d 0,0,0\n", argv[0]);
                exit(EXIT_SUCCESS);
            
            // Handle row count option
            case 'r': 
                ROWS = atoi(optarg);  // Convert string argument to integer
                if (ROWS <= 0) {
                    fprintf(stderr, "Number of rows must be positive\n");
                    exit(EXIT_FAILURE);
                }
                break;
            
            // Handle column count option
            case 'c': 
                COLS = atoi(optarg); 
                if (COLS <= 0) {
                    fprintf(stderr, "Number of columns must be positive\n");
                    exit(EXIT_FAILURE);
                }
                break;
            
            // Handle border size option
            case 'b': 
                BORDER_SIZE = atoi(optarg); 
                if (BORDER_SIZE < 0) {
                    fprintf(stderr, "Border size cannot be negative\n");
                    exit(EXIT_FAILURE);
                }
                break;
            
            // Handle tick interval option
            case 't': 
                TICK_INTERVAL = atof(optarg);  // Convert string argument to float
                if (TICK_INTERVAL <= 0.0f) {
                    fprintf(stderr, "Tick interval must be positive\n");
                    exit(EXIT_FAILURE);
                }
                break;
            
            // Handle initial active squares option
            case 'i': 
                INITIAL_RANDOM_SQUARES = atoi(optarg); 
                if (INITIAL_RANDOM_SQUARES < 0) {
                    fprintf(stderr, "Initial squares cannot be negative\n");
                    exit(EXIT_FAILURE);
                }
                break;
            
            // Handle maximum random ticks option
            case 'm': 
                MAX_RANDOM_TICK = atoi(optarg); 
                if (MAX_RANDOM_TICK <= 0) {
                    fprintf(stderr, "Maximum ticks must be positive\n");
                    exit(EXIT_FAILURE);
                }
                break;
            
            // Handle grid color option
            case 'g': {
                int r, g, b;
                // Parse RGB values from comma-separated string
                if (sscanf(optarg, "%d,%d,%d", &r, &g, &b) == 3) {
                    // Validate color values are within range
                    if (r < 0 || r > MAX_COLOR_VALUE || 
                        g < 0 || g > MAX_COLOR_VALUE || 
                        b < 0 || b > MAX_COLOR_VALUE) {
                        fprintf(stderr, "Color values must be between 0 and %d\n", MAX_COLOR_VALUE);
                        exit(EXIT_FAILURE);
                    }
                    GRID_COLOR = (Color){r, g, b, 255};  // Set color with full opacity
                } else {
                    fprintf(stderr, "Invalid color format for -g. Use R,G,B (e.g., 255,0,0)\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            
            // Handle deactive color option
            case 'd': {
                int r, g, b;
                // Parse RGB values from comma-separated string
                if (sscanf(optarg, "%d,%d,%d", &r, &g, &b) == 3) {
                    // Validate color values are within range
                    if (r < 0 || r > MAX_COLOR_VALUE || 
                        g < 0 || g > MAX_COLOR_VALUE || 
                        b < 0 || b > MAX_COLOR_VALUE) {
                        fprintf(stderr, "Color values must be between 0 and %d\n", MAX_COLOR_VALUE);
                        exit(EXIT_FAILURE);
                    }
                    DEACTIVE_COLOR = (Color){r, g, b, 255};  // Set color with full opacity
                } else {
                    fprintf(stderr, "Invalid color format for -d. Use R,G,B (e.g., 0,0,0)\n");
                    exit(EXIT_FAILURE);
                }
                break;
            }
            
            // Handle window width option
            case 'w':
                WINDOW_WIDTH = atoi(optarg);
                if (WINDOW_WIDTH <= 0) {
                    fprintf(stderr, "Window width must be positive\n");
                    exit(EXIT_FAILURE);
                }
                break;
                
            // Handle window height option
            case 'H':
                WINDOW_HEIGHT = atoi(optarg);
                if (WINDOW_HEIGHT <= 0) {
                    fprintf(stderr, "Window height must be positive\n");
                    exit(EXIT_FAILURE);
                }
                break;
            
            // Handle unknown options
            default:
                fprintf(stderr, "Usage: %s [-r rows] [-c cols] [-b border_size] "
                        "[-t tick_interval] [-i initial_squares] [-m max_tick] "
                        "[-g grid_color] [-d deactive_color]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

// Structure representing a single grid pixel
typedef struct {
    Color color;  // Current color of the pixel
    int tick;     // Timer controlling when the pixel changes
} Pixel;

// 2D array representing the pixel grid
Pixel **grid = NULL;

// Global timer for grid updates
float timer = 0.0f;

// Function to generate a random color
Color GetRandomColor() {
    return (Color){
        GetRandomValue(0, MAX_COLOR_VALUE),  // Random red component
        GetRandomValue(0, MAX_COLOR_VALUE),  // Random green component
        GetRandomValue(0, MAX_COLOR_VALUE),  // Random blue component
        MAX_COLOR_VALUE                      // Fully opaque
    };
}

// Initialize the grid with default values
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

    // Set all pixels to the deactive color
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            grid[row][col].color = DEACTIVE_COLOR;
            grid[row][col].tick = 0;
        }
    }

    // Activate random initial pixels
    for (int i = 0; i < INITIAL_RANDOM_SQUARES; i++) {
        int row = GetRandomValue(0, ROWS - 1);
        int col = GetRandomValue(0, COLS - 1);
        
        // Ensure we select an inactive pixel
        while (grid[row][col].tick > 0) {
            row = GetRandomValue(0, ROWS - 1);
            col = GetRandomValue(0, COLS - 1);
        }
        
        grid[row][col].color = GetRandomColor();
        grid[row][col].tick = GetRandomValue(1, MAX_RANDOM_TICK);
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

// Set color of a specific grid square
void SetSquareColor(int row, int col, Color color) {
    // Check bounds before setting color
    if (row >= 0 && row < DEFAULT_ROWS && col >= 0 && col < DEFAULT_COLS) {
        grid[row][col].color = color;
    }
}

// Set tick for a specific grid square
void SetSquareTick(int row, int col, int tick) {
    // Check bounds before setting tick
    if (row >= 0 && row < DEFAULT_ROWS && col >= 0 && col < DEFAULT_COLS) {
        grid[row][col].tick = tick;
    }
}

// Update grid state
void UpdateGrid() {
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            // Process active pixels
            if (grid[row][col].tick > 0) {
                grid[row][col].tick--;
                
                // When tick reaches 0, return to deactivated state
                if (grid[row][col].tick == 0) {
                    grid[row][col].color = DEACTIVE_COLOR;
                    
                    // Choose a new random square to activate
                    int newRow, newCol;
                    do {
                        newRow = GetRandomValue(0, ROWS - 1);
                        newCol = GetRandomValue(0, COLS - 1);
                    } while (grid[newRow][newCol].tick > 0);
                    
                    // Activate the new pixel
                    grid[newRow][newCol].color = GetRandomColor();
                    grid[newRow][newCol].tick = GetRandomValue(1, MAX_RANDOM_TICK);
                }
            }
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

int main(int argc, char *argv[]) {
    ParseCommandLine(argc, argv);
    
    // Initialize the window and grid
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Random Pixels");
    InitGrid();
    SetTargetFPS(60);

    // Main program loop
    while (!WindowShouldClose()) {
        // Update the timer
        timer += GetFrameTime();
        
        // Update the grid at the specified interval
        if (timer >= TICK_INTERVAL) {
            UpdateGrid();
            timer = 0.0f;
        }

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

