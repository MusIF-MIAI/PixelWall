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
#include "designs.h"
#include "font.h"

Config defaultConf = {
    .rows = 16,
    .cols = 22,
    .windowWidth = 1276,
    .windowHeight = 928,
    .frameRate = 10,
    .borderSize = 4,
    .borderColor = BLACK,
    .moveInterval = 0.2f,
    .backgroundColor = BLACK,
    .showData = false,
    .designIndex = 0,
    .horizontalFlip = false,
};

#define MAX_COLOR_VALUE 255

ArcadeFont fonts;
Grid theGrid;

// Initialize the game grid with default values
void GridFillColor(Grid *grid, Color color) {
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            Pos pos = { col, row };
            GridSetColor(grid, pos, color);
        }
    }
}

void GridFillData(Grid *grid, uintptr_t data) {
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            Pos pos = { col, row };
            GridSetData(grid, pos, data);
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
Pos GetRandomDirection() {
    Pos dirs[] = {
        {1, 0},   // Right
        {-1, 0},  // Left
        {0, 1},   // Down
        {0, -1}   // Up
    };
    return dirs[GetRandomValue(0, 3)];
}

Pos GetRandomPositionIn(int rows, int cols) {
    return (Pos) {
        GetRandomValue(0, cols - 1),
        GetRandomValue(0, rows - 1),
    };
}

void DrawPixelGrid(const Grid *grid) {
    // Calculate cell dimensions excluding borders
    int cellWidth = grid->conf.windowWidth / grid->cols;
    int cellHeight = grid->conf.windowHeight / grid->rows;

    int cellWidthNoBorder = cellWidth - grid->conf.borderSize;
    int cellHeightNoBorder = cellHeight - grid->conf.borderSize;
    
    // Draw grid cells with borders
    for (int row = 0; row < grid->rows; row++) {
        for (int col = 0; col < grid->cols; col++) {
            // Calculate cell position with border spacing
            Pos pos = { col, row };
            int x = pos.x * cellWidth + grid->conf.borderSize;
            int y = pos.y * cellHeight + grid->conf.borderSize;

            // Draw cell content
            if (grid->conf.horizontalFlip) {
                pos.x = grid->cols - 1 - pos.x;
            }

            Color cellColor = GridGetColor(grid, pos);
            DrawRectangle(x, y, cellWidthNoBorder, cellHeightNoBorder, cellColor);

            if (grid->conf.showData)
                DrawText(TextFormat("%d", GridGetData(grid, pos)), x + 5, y + 5, 18, WHITE);
        }
    }
    
    // Draw grid borders between cells
    for (int i = 0; i <= grid->cols; i++) {
        int x = i * cellWidth;
        DrawRectangle(x, 0, grid->conf.borderSize, grid->conf.windowHeight, grid->conf.borderColor);
    }

    for (int i = 0; i <= grid->rows; i++) {
        int y = i * cellHeight;
        DrawRectangle(0, y, grid->conf.windowWidth, grid->conf.borderSize, grid->conf.borderColor);
    }
}

void GridInitialize(Grid *grid, Config conf) {
    grid->conf = conf;
    grid->rows = conf.rows;
    grid->cols = conf.cols;

    // Allocate memory for rows
    grid->pixels = (Pixel **)malloc(grid->rows * sizeof(Pixel *));
    if (!grid->pixels) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    // Allocate memory for columns
    for (int i = 0; i < grid->rows; i++) {
        grid->pixels[i] = (Pixel *)malloc(grid->cols * sizeof(Pixel));
        if (!grid->pixels[i]) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }
    }
}

Color GridGetColor(const Grid *grid, Pos pos) {
    return grid->pixels[pos.y][pos.x].color;
}

void GridSetColor(Grid *grid, Pos pos, Color color) {
    grid->pixels[pos.y][pos.x].color = color;
}

uintptr_t GridGetData(const Grid *grid, Pos pos) {
    return grid->pixels[pos.y][pos.x].data;
}

void GridSetData(Grid *grid, Pos pos, uintptr_t data) {
    grid->pixels[pos.y][pos.x].data = data;
}

void GridCleanup(Grid *grid) {
    if (grid->pixels) {
        for (int i = 0; i < grid->rows; i++) {
            free(grid->pixels[i]);
        }
        free(grid->pixels);
    }

    grid->pixels = 0;
    grid->rows = 0;
    grid->cols = 0;
}

void PrintHelp(int argc, char *argv[]) {
    printf("The MIAI pixelwall\n");
    printf("Usage: %s [options]\n", argv[0]);
    printf("Options:\n");
    printf("  -d <design>      Set design (default: %s)\n", designs[0]->name);
    printf("  -B <color>       Set background color (R,G,B, default: %d,%d,%d)\n",
           defaultConf.backgroundColor.r, defaultConf.backgroundColor.g, defaultConf.backgroundColor.b);
    printf("  -O <color>       Set border color (R,G,B, default: %d,%d,%d)\n",
           defaultConf.borderColor.r, defaultConf.borderColor.g, defaultConf.borderColor.b);
    printf("  -r <rows>        Set number of rows (default: %d)\n", defaultConf.rows);
    printf("  -c <cols>        Set number of columns (default: %d)\n", defaultConf.cols);
    printf("  -w <width>       Set window width (default: %d)\n", defaultConf.windowWidth);
    printf("  -H <height>      Set window height (default: %d)\n", defaultConf.windowHeight);
    printf("  -F               Flip horizontally\n");
    printf("  -f <rate>        Set frame rate (default: %d)\n", defaultConf.frameRate);
    printf("  -i <interval>    Set move interval in seconds (default: %.1f)\n", defaultConf.moveInterval);
    printf("  -b <size>        Set border size (default: %d)\n", defaultConf.borderSize);
    printf("  -h               Show this help message\n");

    printf("\n");

    for (int i = 0; i < sizeof(designs) / sizeof(Design *); i++) {
        printf("Design: %s\n", designs[i]->name);
        designs[i]->PrintHelp();
        printf("\n");
    }
}

Direction ParseDirection(const char *string) {
    Direction d = atoi(optarg);

    if (d != HORIZONTAL && d != VERTICAL) {
        fprintf(stderr, "Invalid direction. Use 0 for horizontal, 1 for vertical\n");
        exit(EXIT_FAILURE);
    }

    return d;
}

Color ParseColor(const char *string) {
    int r, g, b;

    if (sscanf(string, "%d,%d,%d", &r, &g, &b) == 3) {
        if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
            fprintf(stderr, "Color values must be between 0 and 255\n");
            exit(EXIT_FAILURE);
        }   

        return (Color){ r, g, b, 255 };
    } else {
        fprintf(stderr, "Invalid color format. Use R,G,B (e.g., 255,0,0)\n");
        exit(EXIT_FAILURE);
    }
}

Config ParseCommandLine(int argc, char *argv[]) {
    int opt;
    Config conf = defaultConf;
    while ((opt = getopt(argc, argv, ":d:r:c:w:H:Ff:b:B:O:h")) != -1) {
        switch (opt) {
            case 'd':
                for (int i = 0; i < sizeof(designs) / sizeof(Design *); i++) {
                    if (strcmp(designs[i]->name, optarg) == 0) {
                        conf.designIndex = i;
                        break;
                    }
                    printf("No design found with name: %s\n", optarg);
                }
                break;
            case 'r':
                conf.rows = atoi(optarg);
                if (conf.rows < 5) conf.rows = 5;
                break;
            case 'c':
                conf.cols = atoi(optarg);
                if (conf.cols < 5) conf.cols = 5;
                break;
            case 'w':
                conf.windowWidth = atoi(optarg);
                if (conf.windowWidth < 100) conf.windowWidth = 100;
                break;
            case 'H':
                conf.windowHeight = atoi(optarg);
                if (conf.windowHeight < 100) conf.windowHeight = 100;
                break;
            case 'F':
                conf.horizontalFlip = true;
                break;
            case 'f':
                conf.frameRate = atoi(optarg);
                if (conf.frameRate < 1) conf.frameRate = 1;
                break;
            case 'i':
                conf.moveInterval = atof(optarg);
                if (conf.moveInterval < 0.1f) {
                    fprintf(stderr, "Move interval must be at least 0.1 seconds\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'b':
                conf.borderSize = atoi(optarg);
                if (conf.borderSize < 0) conf.borderSize = 0;
                break;
            case 'B':
                conf.backgroundColor = ParseColor(optarg);
                break;
            case 'O':
                conf.borderColor = ParseColor(optarg);
                break;
            case 'h':
                PrintHelp(argc, argv);
                exit(EXIT_SUCCESS);
        }
    }

    optind = 1;
    return conf;
}

// Main game loop
int main(int argc, char *argv[]) {
    Grid *grid = &theGrid;
    Config conf = ParseCommandLine(argc, argv);

    // Initialize window and set frame rate
    InitWindow(conf.windowWidth, conf.windowHeight, "Pixelwall");
    SetTargetFPS(conf.frameRate);

    GridInitialize(grid, conf);
    GridFillColor(grid, grid->conf.backgroundColor);
    GridFillData(grid, 0);

    Design *design = designs[conf.designIndex];

    printf("Starting design: %s\n", design->name);
    void *data = design->Create(grid, argc, argv);

    float timer = 0;
    bool changeDesign = false;
    int designCount = sizeof(designs) / sizeof(Design *);

    int cellWidth = grid->conf.windowWidth / grid->cols;
    int cellHeight = grid->conf.windowHeight / grid->rows;
    int cellMin = cellWidth < cellHeight ? cellWidth : cellHeight;

    // Load arcade fonts
    fonts.image = LoadImageFromMemory(".png", fontone_png, fontone_png_len);
    fonts.texture = LoadTextureFromImage(fonts.image);
    fonts.size = 8;
    fonts.charOffset = 32;
    fonts.count = fonts.image.height / fonts.size;

    // Main game loop
    while (!WindowShouldClose()) {
        if (IsKeyPressed(KEY_SPACE)) {
            grid->conf.showData = !grid->conf.showData;
        }

        if (IsKeyPressed(KEY_F)) {
            grid->conf.horizontalFlip = !grid->conf.horizontalFlip;
        }

        if (IsKeyPressed(KEY_PERIOD)) {
            conf.designIndex++;
            if (conf.designIndex >= designCount) {
                conf.designIndex = 0;
            }

            changeDesign = true;
        }

        if (IsKeyPressed(KEY_COMMA)) {
            conf.designIndex--;
            if (conf.designIndex < 0) {
                conf.designIndex = designCount - 1;
            }

            changeDesign = true;
        }

        if (changeDesign) {
            design->Destroy(data);
            design = designs[conf.designIndex];
            printf("Switching to design: %s\n", design->name);
            GridFillColor(grid, grid->conf.backgroundColor);
            GridFillData(grid, 0);
            data = design->Create(grid, argc, argv);
            changeDesign = false;
        }

        // Update game state
        timer += GetFrameTime();

        if (timer >= grid->conf.moveInterval) {
            timer = 0;
            design->UpdateFrame(grid, data);
        }
        
        // Draw game state
        BeginDrawing();
            ClearBackground(grid->conf.backgroundColor);
            DrawPixelGrid(grid);
        EndDrawing();
    }

    design->Destroy(data);
    GridCleanup(grid);
    CloseWindow();
    return 0;
}
