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

#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>  
#include <unistd.h>


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
    Pixel **grid;        // 2D array representing the game grid
    Worm worm;           // The snake/worm object
    Fruit fruit;         // The fruit object
    float moveTimer;     // Timer for movement updates
    Vector2 currentDir;  // Current movement direction
    bool gameOver;       // Game over flag
} GameState;

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

bool IsPositionValid(const GameState *state, Vector2 pos) {
    return pos.x >= 0 && pos.x < COLS && 
           pos.y >= 0 && pos.y < ROWS &&
           !state->grid[(int)pos.y][(int)pos.x].isWorm;
}

Vector2 CalculateNewHead(const GameState *state) {
    Vector2 head = state->worm.body[0].position;
    return (Vector2){head.x + state->currentDir.x, 
                    head.y + state->currentDir.y};
}

// Update worm position and handle growth
void UpdateWormPosition(GameState *state, Vector2 newHead, bool growing) {
    // If growing, increment length before updating positions
    if (growing) {
        state->worm.length++;
    } else {
        // Clear the old tail position only if not growing
        Vector2 oldTail = state->worm.body[state->worm.length - 1].position;
        state->grid[(int)oldTail.y][(int)oldTail.x].color = BACKGROUND_COLOR;
        state->grid[(int)oldTail.y][(int)oldTail.x].isWorm = false;
    }
    
    // Move worm segments forward
    for (int i = state->worm.length - 1; i > 0; i--) {
        state->worm.body[i] = state->worm.body[i - 1];
    }
    state->worm.body[0].position = newHead;
    
    // Update grid with new positions
    for (int i = 0; i < state->worm.length; i++) {
        Vector2 pos = state->worm.body[i].position;
        state->grid[(int)pos.y][(int)pos.x].color = WORM_COLOR;
        state->grid[(int)pos.y][(int)pos.x].isWorm = true;
    }
}

// Handle fruit collision and create new fruit
void HandleFruitCollision(GameState *state) {
    // Increase worm length if possible
    if (state->worm.length < MAX_WORM_LENGTH) {
        // Add new segment at the end
        state->worm.body[state->worm.length] = state->worm.body[state->worm.length - 1];
        state->worm.length++;
    }
    
    // Remove old fruit from grid
    Vector2 oldFruitPos = state->fruit.position;
    state->grid[(int)oldFruitPos.y][(int)oldFruitPos.x].isFruit = false;
    
    // Create new fruit at random position
    InitializeFruit(state);
}

// Helper function to check if a move is safe
bool IsSafeMove(const GameState *state, Vector2 pos) {
    return pos.x >= 0 && pos.x < COLS && 
           pos.y >= 0 && pos.y < ROWS && 
           !state->grid[(int)pos.y][(int)pos.x].isWorm;
}

// Simplified UpdateAI function
void UpdateAI(GameState *state) {
    Vector2 head = state->worm.body[0].position;
    Vector2 fruitPos = state->fruit.position;
    
    // First try to move towards fruit
    Vector2 desiredDir = {0, 0};
    
    // Try horizontal movement first
    if (fruitPos.x != head.x) {
        desiredDir.x = (fruitPos.x > head.x) ? 1 : -1;
        desiredDir.y = 0;
    } 
    // Then try vertical movement
    else if (fruitPos.y != head.y) {
        desiredDir.x = 0;
        desiredDir.y = (fruitPos.y > head.y) ? 1 : -1;
    }
    
    // Check if desired move is safe
    Vector2 nextPos = {
        head.x + desiredDir.x,
        head.y + desiredDir.y
    };
    
    if (IsSafeMove(state, nextPos)) {
        state->currentDir = desiredDir;
        return;
    }
    
    // If desired move is not safe, try other directions
    Vector2 directions[] = {
        {1, 0},   // Right
        {-1, 0},  // Left
        {0, 1},   // Down
        {0, -1}   // Up
    };
    
    for (int i = 0; i < 4; i++) {
        Vector2 dir = directions[i];
        Vector2 newPos = {head.x + dir.x, head.y + dir.y};
        
        if (IsSafeMove(state, newPos)) {
            state->currentDir = dir;
            return;
        }
    }
    
    // If no safe moves found, keep current direction (will trigger game over)
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
    InitializeWorm(&state);
    InitializeFruit(&state);
    
    state.moveTimer = 0.0f;  // Initialize movement timer
    state.currentDir = GetRandomDirection();  // Set initial direction
    state.gameOver = false;  // Game is running

    // Main game loop
    while (!WindowShouldClose()) {
        // Update game state
        state.moveTimer += GetFrameTime();
        if (state.moveTimer >= MOVE_INTERVAL) {
            if (state.gameOver) {
                // Reset game state if game over
                InitializeGrid(&state);
                InitializeWorm(&state);
                InitializeFruit(&state);
                state.currentDir = GetRandomDirection();
                state.gameOver = false;
            }
            else {
                // Update AI and check for collisions
                UpdateAI(&state);
                
                Vector2 newHead = CalculateNewHead(&state);
                if (CheckGameOver(&state, newHead)) {
                    state.gameOver = true;
                }
                else {
                    // Check if the worm is about to eat fruit
                    bool willGrow = state.grid[(int)newHead.y][(int)newHead.x].isFruit;
                    
                    UpdateWormPosition(&state, newHead, willGrow);
                    
                    if (willGrow) {
                        HandleFruitCollision(&state);
                    }
                }
            }
            state.moveTimer = 0.0f;  // Reset movement timer
        }
        
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

// Initialize the worm at the center of the grid
void InitializeWorm(GameState *state) {
    state->worm.max_length = MAX_WORM_LENGTH;
    state->worm.body = (Segment *)malloc(state->worm.max_length * sizeof(Segment));
    if (!state->worm.body) {
        fprintf(stderr, "Failed to allocate memory for worm body\n");
        exit(EXIT_FAILURE);
    }
    state->worm.length = WORM_LENGTH;
    // Start worm in the middle of the grid
    int startRow = ROWS / 2;
    int startCol = COLS / 2;
    
    // Initialize worm segments from head to tail
    for (int i = 0; i < WORM_LENGTH; i++) {
        state->worm.body[i].position = (Vector2){startCol - i, startRow};
        state->worm.body[i].color = WORM_COLOR;
        
        // Mark positions in grid
        int col = startCol - i;
        int row = startRow;
        state->grid[row][col].color = WORM_COLOR;
        state->grid[row][col].isWorm = true;
    }
}

void InitializeFruit(GameState *state) {
    // Place fruit in random position (not on worm)
    int row, col;
    do {
        row = GetRandomValue(0, ROWS - 1);
        col = GetRandomValue(0, COLS - 1);
    } while (state->grid[row][col].isWorm);
    
    state->fruit.position = (Vector2){col, row};
    state->fruit.color = GetRandomColor();
    
    // Mark position in grid
    state->grid[row][col].color = state->fruit.color;
    state->grid[row][col].isFruit = true;
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

bool CheckGameOver(const GameState *state, Vector2 newHead) {
    // Check if new head position is invalid
    return !IsPositionValid(state, newHead);
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

void CleanupWorm(GameState *state) {
    if (state->worm.body) {
        free(state->worm.body);
        state->worm.body = NULL;
    }
    state->worm.length = 0;
    state->worm.max_length = 0;
}

void GrowWorm(Worm *worm) {
    if (worm->length >= worm->max_length) {
        // Handle maximum length reached
        return;
    }
    // Add new segment
    worm->length++;
}

void ResizeWorm(Worm *worm, int new_max_length) {
    Segment *new_body = (Segment *)realloc(worm->body, new_max_length * sizeof(Segment));
    if (!new_body) {
        fprintf(stderr, "Failed to resize worm body\n");
        return;
    }
    worm->body = new_body;
    worm->max_length = new_max_length;
    if (worm->length > worm->max_length) {
        worm->length = worm->max_length;
    }
}