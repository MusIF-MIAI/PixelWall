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

#include "design.h"

// Configuration
#define DEFAULT_ROWS 16
#define DEFAULT_COLS 22
#define DEFAULT_WINDOW_WIDTH 1280
#define DEFAULT_WINDOW_HEIGHT 720
#define MAX_COLOR_VALUE 255
#define DEFAULT_WORM_LENGTH 5
#define DEFAULT_FRAME_RATE 10
#define DEFAULT_MOVE_INTERVAL 0.2f
#define DEFAULT_MAX_WORM_LENGTH 100
#define DEFAULT_BORDER_SIZE 2  // Default border size in pixels
#define DEFAULT_BACKGROUND_COLOR BLACK
#define DEFAULT_WORM_COLOR GREEN
#define DEFAULT_BORDER_COLOR GRAY

// Update configuration variables
int WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH;
int WINDOW_HEIGHT = DEFAULT_WINDOW_HEIGHT;
int BORDER_SIZE = DEFAULT_BORDER_SIZE;  // Configurable border size
int WORM_LENGTH = DEFAULT_WORM_LENGTH;
int FRAME_RATE = DEFAULT_FRAME_RATE;
int MAX_WORM_LENGTH = DEFAULT_MAX_WORM_LENGTH;
float MOVE_INTERVAL = DEFAULT_MOVE_INTERVAL;
Color BACKGROUND_COLOR = DEFAULT_BACKGROUND_COLOR;
Color WORM_COLOR = DEFAULT_WORM_COLOR;
Color BORDER_COLOR = DEFAULT_BORDER_COLOR;

int cellWidth;
int cellHeight;

// Initialize the game grid with default values
void InitializeGrid(Grid *grid) {
    // Loop through all rows and columns
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            grid->grid[row][col].color = BACKGROUND_COLOR;  // Set background color
            grid->grid[row][col].isWorm = false;            // No worm initially
            grid->grid[row][col].isFruit = false;           // No fruit initially
        }
    }
}

// Add helper function for random colors
Color GetRandomColor() {
    return (Color){
        GetRandomValue(50, 255),  // R
        GetRandomValue(50, 255),  // G
        GetRandomValue(50, 255),  // B
        255                       // A (fully opaque)
    };
}

// Add movement functions
Vector2 GetRandomDirection() {
    Vector2 dirs[] = {
        {1, 0},   // Right
        {-1, 0},  // Left
        {0, 1},   // Down
        {0, -1}   // Up
    };
    return dirs[GetRandomValue(0, 3)];
}

Grid theGrid;
GameState theState;

void DrawPixelGrid(const Grid *grid) {
    // Calculate cell dimensions excluding borders
    int cellWidthNoBorder = cellWidth - BORDER_SIZE;
    int cellHeightNoBorder = cellHeight - BORDER_SIZE;
    
    // Draw grid cells with borders
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            // Calculate cell position with border spacing
            int x = col * cellWidth + BORDER_SIZE;
            int y = row * cellHeight + BORDER_SIZE;
            
            // Draw cell content
            Color cellColor = grid->grid[row][col].color;
            DrawRectangle(x, y, cellWidthNoBorder, cellHeightNoBorder, cellColor);
        }
    }
    
    // Draw grid borders between cells
    for (int i = 0; i <= grid->cols; i++) {
        int x = i * cellWidth;
        DrawRectangle(x, 0, BORDER_SIZE, WINDOW_HEIGHT, BORDER_COLOR);
    }

    for (int i = 0; i <= grid->rows; i++) {
        int y = i * cellHeight;
        DrawRectangle(0, y, WINDOW_WIDTH, BORDER_SIZE, BORDER_COLOR);
    }
}

void InitializeGridMemory(Grid *grid) {
    // Allocate memory for rows
    grid->grid = (Pixel **)malloc(grid->rows * sizeof(Pixel *));
    if (!grid->grid) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for columns
    for (int i = 0; i < grid->rows; i++) {
        grid->grid[i] = (Pixel *)malloc(grid->cols * sizeof(Pixel));
        if (!grid->grid[i]) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
    }
}

void CleanupGridMemory(Grid *grid) {
    if (grid->grid) {
        for (int i = 0; i < grid->rows; i++) {
            free(grid->grid[i]);
        }
        free(grid->grid);
    }
}

void PrintHelp() {
    printf("Snake Animation - Command Line Options\n");
    printf("Usage: snake_animation [options]\n");
    printf("Options:\n");
    printf("  -B <color>       Set background color (R,G,B, default: %d,%d,%d)\n",
           DEFAULT_BACKGROUND_COLOR.r, DEFAULT_BACKGROUND_COLOR.g, DEFAULT_BACKGROUND_COLOR.b);
    printf("  -W <color>       Set worm color (R,G,B, default: %d,%d,%d)\n",
           DEFAULT_WORM_COLOR.r, DEFAULT_WORM_COLOR.g, DEFAULT_WORM_COLOR.b);
    printf("  -O <color>       Set border color (R,G,B, default: %d,%d,%d)\n",
           DEFAULT_BORDER_COLOR.r, DEFAULT_BORDER_COLOR.g, DEFAULT_BORDER_COLOR.b);
    printf("  -r <rows>        Set number of rows (default: %d)\n", DEFAULT_ROWS);
    printf("  -c <cols>        Set number of columns (default: %d)\n", DEFAULT_COLS);
    printf("  -w <width>       Set window width (default: %d)\n", DEFAULT_WINDOW_WIDTH);
    printf("  -H <height>      Set window height (default: %d)\n", DEFAULT_WINDOW_HEIGHT);
    printf("  -l <length>      Set initial worm length (default: %d)\n", DEFAULT_WORM_LENGTH);
    printf("  -f <rate>        Set frame rate (default: %d)\n", DEFAULT_FRAME_RATE);
    printf("  -i <interval>    Set move interval in seconds (default: %.1f)\n", DEFAULT_MOVE_INTERVAL);
    printf("  -b <size>        Set border size (default: %d)\n", DEFAULT_BORDER_SIZE);
    printf("  -m <length>      Set maximum worm length (default: %d)\n", DEFAULT_MAX_WORM_LENGTH);
    printf("  -h               Show this help message\n");
}

void ParseCommandLine(int argc, char *argv[], Grid *grid) {
    int opt;
    while ((opt = getopt(argc, argv, "r:c:w:H:l:f:i:b:m:B:W:O:h")) != -1) {
        switch (opt) {
            case 'r':
                grid->rows = atoi(optarg);
                if (grid->rows < 5) grid->rows = 5;
                break;
            case 'c':
                grid->cols = atoi(optarg);
                if (grid->cols < 5) grid->cols = 5;
                break;
            case 'w':
                WINDOW_WIDTH = atoi(optarg);
                if (WINDOW_WIDTH < 100) WINDOW_WIDTH = 100;
                break;
            case 'H':
                WINDOW_HEIGHT = atoi(optarg);
                if (WINDOW_HEIGHT < 100) WINDOW_HEIGHT = 100;
                break;
            case 'l':
                WORM_LENGTH = atoi(optarg);
                if (WORM_LENGTH < 1) WORM_LENGTH = 1;
                break;
            case 'f':
                FRAME_RATE = atoi(optarg);
                if (FRAME_RATE < 1) FRAME_RATE = 1;
                break;
            case 'i':
                MOVE_INTERVAL = atof(optarg);
                if (MOVE_INTERVAL < 0.1f) {
                    fprintf(stderr, "Move interval must be at least 0.1 seconds\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'b':
                BORDER_SIZE = atoi(optarg);
                if (BORDER_SIZE < 0) BORDER_SIZE = 0;
                break;
            case 'm':
                MAX_WORM_LENGTH = atoi(optarg);
                if (MAX_WORM_LENGTH < WORM_LENGTH) {
                    fprintf(stderr, "Maximum worm length cannot be less than initial length (%d)\n", WORM_LENGTH);
                    exit(EXIT_FAILURE);
                }
                break;
            case 'B':  // Background color
            case 'W':  // Worm color
            case 'O': {  // Border color
                int r, g, b;
                if (sscanf(optarg, "%d,%d,%d", &r, &g, &b) == 3) {
                    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
                        fprintf(stderr, "Color values must be between 0 and 255\n");
                        exit(EXIT_FAILURE);
                    }
                    Color newColor = {r, g, b, 255};
                    switch (opt) {
                        case 'B': BACKGROUND_COLOR = newColor; break;
                        case 'W': WORM_COLOR = newColor; break;
                        case 'O': BORDER_COLOR = newColor; break;
                    }
                } else {
                    fprintf(stderr, "Invalid color format. Use R,G,B (e.g., 255,0,0)\n");
                    exit(EXIT_FAILURE);
                }
                break;
            } 
            case 'h':
            default:
                PrintHelp();
                exit(EXIT_SUCCESS);
        }
    }
}

void SetDefaults(Grid *grid) {
    grid->rows = DEFAULT_ROWS;
    grid->cols = DEFAULT_COLS;
}

// Main game loop
int main(int argc, char *argv[]) {
    Grid *grid = &theGrid;
    GameState *state = &theState;

    SetDefaults(grid);
    ParseCommandLine(argc, argv, grid);
    
    // Initialize window and set frame rate
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Snake Animation");
    SetTargetFPS(FRAME_RATE);

    // Calculate cell dimensions based on window size
    cellWidth = WINDOW_WIDTH / grid->cols;
    cellHeight = WINDOW_HEIGHT / grid->rows;

    InitializeGridMemory(grid);
    InitializeGrid(grid);

    DesignInit(grid, state);

    // Main game loop
    while (!WindowShouldClose()) {
        // Update game state
        DesignUpdateFrame(grid, state);
        
        // Draw game state
        BeginDrawing();
            ClearBackground(BACKGROUND_COLOR);
            DrawPixelGrid(grid);
        EndDrawing();
    }

    DesignCleanup(state);
    CleanupGridMemory(grid);
    CloseWindow();
    return 0;
}
