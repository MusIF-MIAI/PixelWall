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
int ROWS = DEFAULT_ROWS;
int COLS = DEFAULT_COLS;
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


// Function prototypes
void InitializeGrid(GameState *state);
void InitializeWorm(GameState *state);
void InitializeFruit(GameState *state);
void DrawPixelGrid(const GameState *state);
void InitializeGridMemory(GameState *state);
void CleanupGridMemory(GameState *state);
bool CheckGameOver(const GameState *state, Vector2 newHead);
void ParseCommandLine(int argc, char *argv[]);
void CleanupWorm(GameState *state);

int cellWidth;
int cellHeight;

// Initialize the game grid with default values
void InitializeGrid(GameState *state) {
    // Loop through all rows and columns
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            state->grid[row][col].color = BACKGROUND_COLOR;  // Set background color
            state->grid[row][col].isWorm = false;            // No worm initially
            state->grid[row][col].isFruit = false;           // No fruit initially
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

// Main game loop
int main(int argc, char *argv[]) {  // Change main signature to standard form
    ParseCommandLine(argc, argv);   // Use standard argc/argv instead of __argc/__argv
    // Initialize window and set frame rate
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Snake Animation");
    SetTargetFPS(FRAME_RATE);

    // Calculate cell dimensions based on window size
    cellWidth = WINDOW_WIDTH / COLS;
    cellHeight = WINDOW_HEIGHT / ROWS;

    // Initialize game state
    GameState state;
    
    InitializeGridMemory(&state);
    InitializeGrid(&state);

    DesignInit(&state);

    // Main game loop
    while (!WindowShouldClose()) {
        // Update game state
        DesignUpdateFrame(&state);
        
        // Draw game state
        BeginDrawing();
            ClearBackground(BACKGROUND_COLOR);
            DrawPixelGrid(&state);
        EndDrawing();
    }

    CleanupWorm(&state);
    // Cleanup resources
    CleanupGridMemory(&state);
    CloseWindow();
    return 0;
}

void DrawPixelGrid(const GameState *state) {
    // Calculate cell dimensions excluding borders
    int cellWidthNoBorder = cellWidth - BORDER_SIZE;
    int cellHeightNoBorder = cellHeight - BORDER_SIZE;
    
    // Draw grid cells with borders
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            // Calculate cell position with border spacing
            int x = col * cellWidth + BORDER_SIZE;
            int y = row * cellHeight + BORDER_SIZE;
            
            // Draw cell content
            Color cellColor = state->grid[row][col].color;
            DrawRectangle(x, y, cellWidthNoBorder, cellHeightNoBorder, cellColor);
        }
    }
    
    // Draw grid borders between cells
    for (int i = 0; i <= COLS; i++) {
        int x = i * cellWidth;
        DrawRectangle(x, 0, BORDER_SIZE, WINDOW_HEIGHT, BORDER_COLOR);
    }
    for (int i = 0; i <= ROWS; i++) {
        int y = i * cellHeight;
        DrawRectangle(0, y, WINDOW_WIDTH, BORDER_SIZE, BORDER_COLOR);
    }
}

void InitializeGridMemory(GameState *state) {
    // Allocate memory for rows
    state->grid = (Pixel **)malloc(ROWS * sizeof(Pixel *));
    if (!state->grid) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for columns
    for (int i = 0; i < ROWS; i++) {
        state->grid[i] = (Pixel *)malloc(COLS * sizeof(Pixel));
        if (!state->grid[i]) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
    }
}

void CleanupGridMemory(GameState *state) {
    if (state->grid) {
        for (int i = 0; i < ROWS; i++) {
            free(state->grid[i]);
        }
        free(state->grid);
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

void ParseCommandLine(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "r:c:w:H:l:f:i:b:m:B:W:O:h")) != -1) {
        switch (opt) {
            case 'r':
                ROWS = atoi(optarg);
                if (ROWS < 5) ROWS = 5;
                break;
            case 'c':
                COLS = atoi(optarg);
                if (COLS < 5) COLS = 5;
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

