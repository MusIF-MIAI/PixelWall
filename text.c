#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "raylib.h"
#include <unistd.h>  // for getopt

// Configuration
#define DEFAULT_TEXT "SUCA"
#define DEFAULT_FONT_SIZE 10
#define DEFAULT_TEXT_COLOR WHITE
#define DEFAULT_GRID_COLOR BLUE
#define DEFAULT_BACKGROUND_COLOR BLACK
#define DEFAULT_SPEED 1 // pixel per second
#define DEFAULT_WINDOW_WIDTH 1280  // 720p width
#define DEFAULT_WINDOW_HEIGHT 720  // 720p height
#define DEFAULT_GRID_COLS 22
#define DEFAULT_GRID_ROWS 16
#define HORIZONTAL 0
#define VERTICAL 1

typedef struct {
    Vector2 position;
    Vector2 velocity;
    const char* text;
    Color color;
} TextState;

// Function prototypes
void InitializeGrid(bool*** grid, int cols, int rows);
void FreeGrid(bool** grid, int rows);
void ClearGrid(bool** grid, int cols, int rows);
void RenderTextToGrid(bool** grid, int cols, int rows, const char* text, Vector2 position, int fontSize, Color color);
void DrawPixelGrid(bool** grid, int cols, int rows, float cellWidth, float cellHeight);
void UpdateTextPosition(TextState* state, int cols, int rows, int fontSize);
Color ParseColor(const char* colorStr);
void DrawTextState(TextState* state, int fontSize);

// Default values
int windowWidth = DEFAULT_WINDOW_WIDTH;
int windowHeight = DEFAULT_WINDOW_HEIGHT;
int gridCols = DEFAULT_GRID_COLS;
int gridRows = DEFAULT_GRID_ROWS;
int fontSize = DEFAULT_FONT_SIZE;
float speed = DEFAULT_SPEED;
const char *text = DEFAULT_TEXT;
Color textColor = DEFAULT_TEXT_COLOR;
Color gridColor = DEFAULT_GRID_COLOR;
Color bgColor = DEFAULT_BACKGROUND_COLOR;
int direction = HORIZONTAL;

int main(int argc, char *argv[]) {
    // Parse command line options
    int opt;
    while ((opt = getopt(argc, argv, "c:r:s:t:f:T:G:B:W:H:d:")) != -1) {
        switch (opt) {
            case 'c': gridCols = atoi(optarg); break;
            case 'r': gridRows = atoi(optarg); break;
            case 's': speed = atof(optarg); break;
            case 't': text = optarg; break;
            case 'f': fontSize = atoi(optarg); break;
            case 'T': textColor = ParseColor(optarg); break;
            case 'G': gridColor = ParseColor(optarg); break;
            case 'B': bgColor = ParseColor(optarg); break;
            case 'W': windowWidth = atoi(optarg); break;
            case 'H': windowHeight = atoi(optarg); break;
            case 'd': 
                direction = atoi(optarg);
                if (direction != HORIZONTAL && direction != VERTICAL) {
                    fprintf(stderr, "Invalid direction. Use 0 for horizontal, 1 for vertical\n");
                    return 1;
                }
                break;
            default:
                fprintf(stderr, "Usage: %s [-c cols] [-r rows] [-s speed] [-t text] [-f fontsize] [-T textcolor] [-G gridcolor] [-B bgcolor] [-W width] [-H height] [-d direction]\n", argv[0]);
                fprintf(stderr, "Colors should be in R,G,B format (e.g., 255,0,0)\n");
                fprintf(stderr, "Direction: 0=horizontal (default), 1=vertical\n");
                return 1;
        }
    }

    // Calculate cell dimensions based on window size and grid size
    float cellWidth = (float)windowWidth / gridCols;
    float cellHeight = (float)windowHeight / gridRows;

    // Initialize window with specified dimensions
    InitWindow(windowWidth, windowHeight, "Scrolling Text");

    // Initialize text state with new parameters
    TextState textState = {
        .position = direction == HORIZONTAL ? 
            (Vector2){(float)(-MeasureText(text, fontSize)), (float)(gridRows - fontSize) / 2} :
            (Vector2){(float)(gridCols - fontSize) / 2, (float)(gridRows + fontSize * (strlen(text) - 1))},
        .velocity = direction == HORIZONTAL ? (Vector2){speed, 0} : (Vector2){0, -speed},
        .text = text,
        .color = textColor
    };

    SetTargetFPS(speed);
    
    // Initialize grid
    bool** grid;
    InitializeGrid(&grid, gridCols, gridRows);
    
    while (!WindowShouldClose()) {
        // Update
        UpdateTextPosition(&textState, gridCols, gridRows, DEFAULT_FONT_SIZE);
        
        // Clear and render text to grid
        ClearGrid(grid, gridCols, gridRows);
        RenderTextToGrid(grid, gridCols, gridRows, textState.text, textState.position, DEFAULT_FONT_SIZE, textState.color);
        
        // Draw
        BeginDrawing();
        ClearBackground(DEFAULT_BACKGROUND_COLOR);
        DrawPixelGrid(grid, gridCols, gridRows, cellWidth, cellHeight);
        EndDrawing();
    }
    
    // Clean up
    FreeGrid(grid, gridRows);
    CloseWindow();
    
    return 0;
}

// Initialize 2D grid array
void InitializeGrid(bool*** grid, int cols, int rows) {
    *grid = (bool**)malloc(rows * sizeof(bool*));
    for (int i = 0; i < rows; i++) {
        (*grid)[i] = (bool*)malloc(cols * sizeof(bool));
        memset((*grid)[i], 0, cols * sizeof(bool));
    }
}

// Free grid memory
void FreeGrid(bool** grid, int rows) {
    for (int i = 0; i < rows; i++) {
        free(grid[i]);
    }
    free(grid);
}

// Clear grid to all false
void ClearGrid(bool** grid, int cols, int rows) {
    for (int i = 0; i < rows; i++) {
        memset(grid[i], 0, cols * sizeof(bool));
    }
}

// Render text to grid (updated version)
void RenderTextToGrid(bool** grid, int cols, int rows, const char* text, Vector2 position, int fontSize, Color color) {
    // Create an image from text using the new API
    Image textImage = GenImageColor(cols, rows, BLANK);
    
    if (direction == HORIZONTAL) {
        // Draw horizontal text
        ImageDrawText(&textImage, text, 
                     (int)position.x, (int)position.y, 
                     fontSize, color);
    } else {
        // Draw vertical text
        int yOffset = 0;
        for (int i = 0; i < strlen(text); i++) {
            char singleChar[2] = {text[i], '\0'};
            ImageDrawText(&textImage, singleChar,
                         (int)position.x, (int)position.y + yOffset,
                         fontSize, color);
            yOffset += fontSize;
        }
    }
    
    // Convert image to grid
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            Color pixel = GetImageColor(textImage, x, y);
            grid[y][x] = (pixel.a > 0);  // Set grid cell if pixel is not transparent
        }
    }
    
    UnloadImage(textImage);
}

// Draw the pixel grid
void DrawPixelGrid(bool** grid, int cols, int rows, float cellWidth, float cellHeight) {
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            if (grid[y][x]) {
                DrawRectangle(x * cellWidth, y * cellHeight, cellWidth, cellHeight, textColor);
            }
            // Draw grid lines
            DrawRectangleLinesEx((Rectangle){
                x * cellWidth,
                y * cellHeight,
                cellWidth,
                cellHeight
            }, 1.0f, gridColor);
        }
    }
}

// Update text position with wrapping
void UpdateTextPosition(TextState* state, int cols, int rows, int fontSize) {
    if (direction == HORIZONTAL) {
        state->position.x += 1;
        int textWidth = MeasureText(state->text, fontSize);
        if (state->position.x > cols) {
            state->position.x = -textWidth;
        }
        else if (state->position.x + textWidth < 0) {
            state->position.x = cols;
        }
    } else {
        state->position.y += 1;
        int textHeight = strlen(state->text) * fontSize;
        if (state->position.y > rows) {
            state->position.y = -textHeight;
        }
        else if (state->position.y + textHeight < 0) {
            state->position.y = rows;
        }
    }
}

// Add this helper function
Color ParseColor(const char* colorStr) {
    int r, g, b;
    if (sscanf(colorStr, "%d,%d,%d", &r, &g, &b) == 3) {
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
            fprintf(stderr, "Color values must be between 0 and 255\n");
            exit(EXIT_FAILURE);
        }
        return (Color){r, g, b, 255};
    }
    fprintf(stderr, "Invalid color format. Use R,G,B (e.g., 255,0,0)\n");
    exit(EXIT_FAILURE);
}

// Update DrawText function
void DrawTextState(TextState* state, int fontSize) {
    if (direction == HORIZONTAL) {
        // Original horizontal text
        DrawText(state->text, (int)state->position.x, (int)state->position.y, fontSize, state->color);
    } else {
        // Vertical text - draw each character separately
        int yOffset = 0;
        for (int i = 0; i < strlen(state->text); i++) {
            char singleChar[2] = {state->text[i], '\0'};
            DrawText(singleChar, 
                    (int)state->position.x, 
                    (int)state->position.y + yOffset, 
                    fontSize, 
                    state->color);
            yOffset += fontSize;  // Move down for next character
        }
    }
}