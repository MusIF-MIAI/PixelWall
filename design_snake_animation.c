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

// Initialize the worm at the center of the grid
void InitializeWorm(Grid *grid, GameState *state) {
    state->worm.max_length = state->conf.maxWormLength;
    state->worm.body = (Segment *)malloc(state->worm.max_length * sizeof(Segment));
    if (!state->worm.body) {
        fprintf(stderr, "Failed to allocate memory for worm body\n");
        exit(EXIT_FAILURE);
    }
    state->worm.length = state->conf.wormLength;
    // Start worm in the middle of the grid
    int startRow = grid->rows / 2;
    int startCol = grid->cols / 2;
    
    // Initialize worm segments from head to tail
    for (int i = 0; i < state->conf.wormLength; i++) {
        state->worm.body[i].position = (Vector2){startCol - i, startRow};
        state->worm.body[i].color = state->conf.wormColor;
        
        // Mark positions in grid
        int col = startCol - i;
        int row = startRow;
        grid->grid[row][col].color = state->conf.wormColor;
        grid->grid[row][col].isWorm = true;
    }
}

void InitializeFruit(Grid *grid, GameState *state) {
    // Place fruit in random position (not on worm)
    int row, col;
    do {
        row = GetRandomValue(0, grid->rows - 1);
        col = GetRandomValue(0, grid->cols - 1);
    } while (grid->grid[row][col].isWorm);
    
    state->fruit.position = (Vector2){col, row};
    state->fruit.color = GetRandomColor();
    
    // Mark position in grid
    grid->grid[row][col].color = state->fruit.color;
    grid->grid[row][col].isFruit = true;
}

bool IsPositionValid(const Grid *grid, Vector2 pos) {
    return pos.x >= 0 && pos.x < grid->cols &&
           pos.y >= 0 && pos.y < grid->rows &&
           !grid->grid[(int)pos.y][(int)pos.x].isWorm;
}

Vector2 CalculateNewHead(const GameState *state) {
    Vector2 head = state->worm.body[0].position;
    return (Vector2){head.x + state->currentDir.x, 
                    head.y + state->currentDir.y};
}

// Helper function to check if a move is safe
bool IsSafeMove(const Grid *grid, Vector2 pos) {
    return pos.x >= 0 && pos.x < grid->cols &&
           pos.y >= 0 && pos.y < grid->rows &&
           !grid->grid[(int)pos.y][(int)pos.x].isWorm;
}

// Check if new head position is invalid
bool CheckGameOver(const Grid *grid, Vector2 newHead) {
    return !IsPositionValid(grid, newHead);
}

// Update worm position and handle growth
void UpdateWormPosition(Grid *grid, GameState *state, Vector2 newHead, bool growing) {
    // If growing, increment length before updating positions
    if (growing) {
        state->worm.length++;
    } else {
        // Clear the old tail position only if not growing
        Vector2 oldTail = state->worm.body[state->worm.length - 1].position;
        grid->grid[(int)oldTail.y][(int)oldTail.x].color = state->conf.backgroundColor;
        grid->grid[(int)oldTail.y][(int)oldTail.x].isWorm = false;
    }
    
    // Move worm segments forward
    for (int i = state->worm.length - 1; i > 0; i--) {
        state->worm.body[i] = state->worm.body[i - 1];
    }
    state->worm.body[0].position = newHead;
    
    // Update grid with new positions
    for (int i = 0; i < state->worm.length; i++) {
        Vector2 pos = state->worm.body[i].position;
        grid->grid[(int)pos.y][(int)pos.x].color = state->conf.wormColor;
        grid->grid[(int)pos.y][(int)pos.x].isWorm = true;
    }
}

// Simplified UpdateAI function
void UpdateAI(Grid *grid, GameState *state) {
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
    
    if (IsSafeMove(grid, nextPos)) {
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
        
        if (IsSafeMove(grid, newPos)) {
            state->currentDir = dir;
            return;
        }
    }
    
    // If no safe moves found, keep current direction (will trigger game over)
}

// Handle fruit collision and create new fruit
void HandleFruitCollision(Grid *grid, GameState *state) {
    // Increase worm length if possible
    if (state->worm.length < state->conf.maxWormLength) {
        // Add new segment at the end
        state->worm.body[state->worm.length] = state->worm.body[state->worm.length - 1];
        state->worm.length++;
    }
    
    // Remove old fruit from grid
    Vector2 oldFruitPos = state->fruit.position;
    grid->grid[(int)oldFruitPos.y][(int)oldFruitPos.x].isFruit = false;
    
    // Create new fruit at random position
    InitializeFruit(grid, state);
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

void DesignCleanup(GameState *state) {
    if (state->worm.body) {
        free(state->worm.body);
        state->worm.body = NULL;
    }
    state->worm.length = 0;
    state->worm.max_length = 0;
}

void DesignInit(Grid *grid, GameState *state) {
    InitializeWorm(grid, state);
    InitializeFruit(grid, state);

    state->moveTimer = 0.0f;  // Initialize movement timer
    state->currentDir = GetRandomDirection();  // Set initial direction
    state->gameOver = false;  // Game is running
}

void DesignUpdateFrame(Grid *grid, GameState *state) {
     // Update game state
    state->moveTimer += GetFrameTime();
    if (state->moveTimer >= state->conf.moveInterval) {
        if (state->gameOver) {
            // Reset game state if game over
            GridFillColor(grid, state->conf.backgroundColor);
            InitializeWorm(grid, state);
            InitializeFruit(grid, state);
            state->currentDir = GetRandomDirection();
            state->gameOver = false;
        }
        else {
            // Update AI and check for collisions
            UpdateAI(grid, state);
            
            Vector2 newHead = CalculateNewHead(state);
            if (CheckGameOver(grid, newHead)) {
                state->gameOver = true;
            }
            else {
                // Check if the worm is about to eat fruit
                bool willGrow = grid->grid[(int)newHead.y][(int)newHead.x].isFruit;
                
                UpdateWormPosition(grid, state, newHead, willGrow);
                
                if (willGrow) {
                    HandleFruitCollision(grid, state);
                }
            }
        }
        state->moveTimer = 0.0f;  // Reset movement timer
    }
}
