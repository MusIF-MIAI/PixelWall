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

typedef struct {
    int wormLength;
    int maxWormLength;
    Color wormColor;
} SnakeConf;

static SnakeConf defaultSnakeConf = {
    .wormLength = 5,
    .maxWormLength = 100,
    .wormColor = GREEN,
};

typedef struct {
    Pos position;
    Color color;
} Segment;

typedef struct {
    Pos position;
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
    SnakeConf conf;
    Worm worm;          // The snake/worm object
    Fruit fruit;        // The fruit object
    Pos currentDir;     // Current movement direction
    bool gameOver;      // Game over flag
} GameState;

#define CELL_WORM  (1 << 0)
#define CELL_FRUIT (1 << 1)

static bool GridIsWorm(const Grid *grid, Pos pos) {
    return !!(GridGetData(grid, pos) & CELL_WORM);
}

static bool GridIsFruit(const Grid *grid, Pos pos) {
    return !!(GridGetData(grid, pos) & CELL_FRUIT);
}

static void GridSetWorm(Grid *grid, Pos pos, bool bit) {
    uintptr_t data = GridGetData(grid, pos);
    GridSetData(grid, pos, (data & ~CELL_WORM) | (bit ? CELL_WORM : 0));
}

static void GridSetFruit(Grid *grid, Pos pos, bool bit) {
    uintptr_t data = GridGetData(grid, pos);
    GridSetData(grid, pos, (data & ~CELL_FRUIT) | (bit ? CELL_FRUIT : 0));
}

// Initialize the worm at the center of the grid
static void InitializeWorm(Grid *grid, GameState *state) {
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
        state->worm.body[i].position = (Pos){startCol - i, startRow};
        state->worm.body[i].color = state->conf.wormColor;
        
        // Mark positions in grid
        Pos pos = { startCol - 1, startRow };

        GridSetColor(grid, pos, state->conf.wormColor);
        GridSetWorm(grid, pos, true);
    }
}

static void InitializeFruit(Grid *grid, GameState *state) {
    // Place fruit in random position (not on worm)
    Pos pos;
    do {
        pos = GetRandomPositionIn(grid->rows, grid->cols);
    } while (GridIsWorm(grid, pos));
    
    state->fruit.position = pos;
    state->fruit.color = GetRandomColor();
    
    // Mark position in grid
    GridSetColor(grid, pos, state->fruit.color);
    GridSetFruit(grid, pos, true);
}

static bool IsPositionValid(const Grid *grid, Pos pos) {
    return pos.x >= 0 && pos.x < grid->cols &&
           pos.y >= 0 && pos.y < grid->rows &&
           !GridIsWorm(grid, pos);
}

static Pos CalculateNewHead(const GameState *state) {
    Pos head = state->worm.body[0].position;
    return (Pos){head.x + state->currentDir.x, 
                 head.y + state->currentDir.y};
}

// Helper function to check if a move is safe
static bool IsSafeMove(const Grid *grid, Pos pos) {
    return pos.x >= 0 && pos.x < grid->cols &&
           pos.y >= 0 && pos.y < grid->rows &&
           !GridIsWorm(grid, pos);
}

// Check if new head position is invalid
static bool CheckGameOver(const Grid *grid, Pos newHead) {
    return !IsPositionValid(grid, newHead);
}

// Update worm position and handle growth
static void UpdateWormPosition(Grid *grid, GameState *state, Pos newHead, bool growing) {
    // If growing, increment length before updating positions
    if (growing) {
        state->worm.length++;
    } else {
        // Clear the old tail position only if not growing
        Pos oldTail = state->worm.body[state->worm.length - 1].position;
        GridSetColor(grid, oldTail, grid->conf.backgroundColor);
        GridSetWorm(grid, oldTail, false);
    }
    
    // Move worm segments forward
    for (int i = state->worm.length - 1; i > 0; i--) {
        state->worm.body[i] = state->worm.body[i - 1];
    }
    state->worm.body[0].position = newHead;
    
    // Update grid with new positions
    for (int i = 0; i < state->worm.length; i++) {
        Pos pos = state->worm.body[i].position;
        GridSetColor(grid, pos, state->conf.wormColor);
        GridSetWorm(grid, pos, true);
    }
}

// Simplified UpdateAI function
static void UpdateAI(Grid *grid, GameState *state) {
    Pos head = state->worm.body[0].position;
    Pos fruitPos = state->fruit.position;
    
    // First try to move towards fruit
    Pos desiredDir = {0, 0};
    
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
    Pos nextPos = {
        head.x + desiredDir.x,
        head.y + desiredDir.y
    };
    
    if (IsSafeMove(grid, nextPos)) {
        state->currentDir = desiredDir;
        return;
    }
    
    // If desired move is not safe, try other directions
    Pos directions[] = {
        {1, 0},   // Right
        {-1, 0},  // Left
        {0, 1},   // Down
        {0, -1}   // Up
    };
    
    for (int i = 0; i < 4; i++) {
        Pos dir = directions[i];
        Pos newPos = {head.x + dir.x, head.y + dir.y};
        if (IsSafeMove(grid, newPos)) {
            state->currentDir = dir;
            return;
        }
    }

    // If no safe moves found, keep current direction (will trigger game over)
}

// Handle fruit collision and create new fruit
static void HandleFruitCollision(Grid *grid, GameState *state) {
    // Increase worm length if possible
    if (state->worm.length < state->conf.maxWormLength) {
        // Add new segment at the end
        state->worm.body[state->worm.length] = state->worm.body[state->worm.length - 1];
        state->worm.length++;
    }
    
    // Remove old fruit from grid
    Pos oldFruitPos = state->fruit.position;
    GridSetFruit(grid, oldFruitPos, false);
    
    // Create new fruit at random position
    InitializeFruit(grid, state);
}

static void GrowWorm(Worm *worm) {
    if (worm->length >= worm->max_length) {
        // Handle maximum length reached
        return;
    }
    // Add new segment
    worm->length++;
}

static void ResizeWorm(Worm *worm, int new_max_length) {
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

static void ParseSnakeOptions(SnakeConf *conf, int argc, char *argv[]) {
    int opt;

    while ((opt = getopt(argc, argv, ":l:m:W:")) != -1) {
        switch (opt) {
            case 'l':
                conf->wormLength = atoi(optarg);
                if (conf->wormLength < 1) conf->wormLength = 1;
                break;
            case 'm':
                conf->maxWormLength = atoi(optarg);
                break;
            case 'W':
                conf->wormColor = ParseColor(optarg);
                break;
        }
    }
}

static void SnakePrintHelp() {
    printf("  -W <color>       Set worm color (R,G,B, default: %d,%d,%d)\n",
           defaultSnakeConf.wormColor.r, defaultSnakeConf.wormColor.g, defaultSnakeConf.wormColor.b);
    printf("  -l <length>      Set initial worm length (default: %d)\n", defaultSnakeConf.wormLength);
    printf("  -m <length>      Set maximum worm length (default: %d)\n", defaultSnakeConf.maxWormLength);
}

static void *SnakeCreate(Grid *grid, int argc, char *argv[]) {
    GameState *state = (GameState *)malloc(sizeof(GameState));
    if (!state) return NULL;

    memset(state, 0, sizeof(GameState));
    state->conf = defaultSnakeConf;
    ParseSnakeOptions(&state->conf, argc, argv);

    InitializeWorm(grid, state);
    InitializeFruit(grid, state);

    state->currentDir = GetRandomDirection();  // Set initial direction
    state->gameOver = false;  // Game is running
    return state;
}

static void SnakeUpdateFrame(Grid *grid, void *data) {
    // Update game state
    GameState *state = (GameState *)data;
    if (state->gameOver) {
        // Reset game state if game over
        GridFillColor(grid, grid->conf.backgroundColor);
        GridFillData(grid, 0);
        InitializeWorm(grid, state);
        InitializeFruit(grid, state);
        state->currentDir = GetRandomDirection();
        state->gameOver = false;
    }
    else {
        // Update AI and check for collisions
        UpdateAI(grid, state);
        
        Pos newHead = CalculateNewHead(state);
        if (CheckGameOver(grid, newHead)) {
            state->gameOver = true;
        }
        else {
            // Check if the worm is about to eat fruit
            bool willGrow = GridIsFruit(grid, newHead);
            UpdateWormPosition(grid, state, newHead, willGrow);
            
            if (willGrow) {
                HandleFruitCollision(grid, state);
            }
        }
    }
}

static void SnakeDestroy(void *data) {
    GameState *state = (GameState *)data;
    if (state->worm.body) {
        free(state->worm.body);
        state->worm.body = NULL;
    }
    free(state);
}

Design snakeDesign = {
    .name = "snake",
    .PrintHelp = SnakePrintHelp,
    .Create = SnakeCreate,
    .UpdateFrame = SnakeUpdateFrame,
    .Destroy = SnakeDestroy,
};