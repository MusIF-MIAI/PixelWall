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
#include <stdbool.h>

#define ENEMY_ROWS 3
#define ENEMY_COLS 5
#define ENEMY_SPACING 3
#define PLAYER_ROW 14
#define PLAYER_HALF 1
#define MOVE_TICKS 6
#define FIRE_TICKS 10

typedef struct {
    bool enemies[ENEMY_ROWS][ENEMY_COLS];
    int enemy_x, enemy_y;
    int enemy_dir;
    int player_x;
    int bullet_x, bullet_y;
    int ebullet_x, ebullet_y;
    int tick;
    int enemies_alive;
    bool colorful;
    Color color;
    Color enemy_color;
    Color player_color;
    Color bullet_color;
    Color ebullet_color;
} InvadersData;

static void ResetWave(InvadersData *id) {
    id->enemies_alive = ENEMY_ROWS * ENEMY_COLS;
    for (int r = 0; r < ENEMY_ROWS; r++)
        for (int c = 0; c < ENEMY_COLS; c++)
            id->enemies[r][c] = true;
    id->enemy_x = 2;
    id->enemy_y = 1;
    id->enemy_dir = 1;
    id->bullet_y = -1;
    id->ebullet_y = -1;
}

static int EnemyGridCol(int c) {
    return c * ENEMY_SPACING;
}

static void PrintHelp() {
    printf("  -C               Enable colored mode\n");
    printf("  -E <color>       Enemy color (R,G,B, default: 255,85,85)\n");
    printf("  -P <color>       Player color (R,G,B, default: 85,255,85)\n");
    printf("  -B <color>       Player bullet color (R,G,B, default: 255,255,85)\n");
    printf("  -K <color>       Enemy bullet color (R,G,B, default: 255,85,255)\n");
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    (void)grid;
    InvadersData *id = calloc(1, sizeof(InvadersData));
    if (!id) return NULL;

    id->color = GREEN;
    id->colorful = false;
    id->enemy_color = (Color){255, 85, 85, 255};
    id->player_color = (Color){85, 255, 85, 255};
    id->bullet_color = (Color){255, 255, 85, 255};
    id->ebullet_color = (Color){255, 85, 255, 255};
    id->player_x = 11;

    int opt;
    optind = 1;
    while ((opt = getopt(argc, argv, ":d:CE:P:B:K:")) != -1) {
        switch (opt) {
            case 'C': id->colorful = true; break;
            case 'E': id->enemy_color = ParseColor(optarg); break;
            case 'P': id->player_color = ParseColor(optarg); break;
            case 'B': id->bullet_color = ParseColor(optarg); break;
            case 'K': id->ebullet_color = ParseColor(optarg); break;
        }
    }

    srand(time(NULL));
    ResetWave(id);
    return id;
}

static int NearestEnemyCol(InvadersData *id) {
    int best = -1;
    int best_dist = 100;
    for (int c = 0; c < ENEMY_COLS; c++) {
        for (int r = ENEMY_ROWS - 1; r >= 0; r--) {
            if (!id->enemies[r][c]) continue;
            int ex = id->enemy_x + EnemyGridCol(c);
            int dist = abs(ex - id->player_x);
            if (dist < best_dist) {
                best_dist = dist;
                best = ex;
            }
            break;
        }
    }
    return best;
}

static void UpdateFrame(Grid *grid, void *data) {
    InvadersData *id = (InvadersData *)data;
    Color bg = grid->conf.backgroundColor;

    id->tick++;
    if (id->tick < MOVE_TICKS) return;
    id->tick = 0;

    // Move enemies
    int leftmost = 100, rightmost = -1;
    for (int c = 0; c < ENEMY_COLS; c++) {
        for (int r = 0; r < ENEMY_ROWS; r++) {
            if (!id->enemies[r][c]) continue;
            int ex = id->enemy_x + EnemyGridCol(c);
            if (ex < leftmost) leftmost = ex;
            if (ex > rightmost) rightmost = ex;
            break;
        }
    }

    if (rightmost >= 0) {
        int next_left = leftmost + id->enemy_dir;
        int next_right = rightmost + id->enemy_dir;
        if (next_left < 0 || next_right >= grid->cols) {
            id->enemy_dir *= -1;
            id->enemy_y++;
        } else {
            id->enemy_x += id->enemy_dir;
        }
    }

    // Enemies reach bottom -> reset
    int bottom_enemy = -1;
    for (int r = ENEMY_ROWS - 1; r >= 0; r--) {
        for (int c = 0; c < ENEMY_COLS; c++) {
            if (id->enemies[r][c]) {
                int ey = id->enemy_y + r;
                if (ey > bottom_enemy) bottom_enemy = ey;
            }
        }
    }
    if (bottom_enemy >= PLAYER_ROW - 1) {
        ResetWave(id);
    }

    // Move player bullet
    if (id->bullet_y >= 0) {
        id->bullet_y--;
        if (id->bullet_y < 0) {
            // missed
        } else {
            // Check hit
            for (int r = 0; r < ENEMY_ROWS; r++) {
                for (int c = 0; c < ENEMY_COLS; c++) {
                    if (!id->enemies[r][c]) continue;
                    int ex = id->enemy_x + EnemyGridCol(c);
                    int ey = id->enemy_y + r;
                    if (id->bullet_x == ex && id->bullet_y == ey) {
                        id->enemies[r][c] = false;
                        id->enemies_alive--;
                        id->bullet_y = -1;
                        goto bullet_done;
                    }
                }
            }
        }
    }
bullet_done:

    // Move enemy bullet
    if (id->ebullet_y >= 0) {
        id->ebullet_y++;
        if (id->ebullet_y > 15) {
            id->ebullet_y = -1;
        } else if (id->ebullet_y == PLAYER_ROW &&
                   id->ebullet_x >= id->player_x - PLAYER_HALF &&
                   id->ebullet_x <= id->player_x + PLAYER_HALF) {
            id->ebullet_y = -1;
            // Player hit - just reset bullet, player respawns in place
        }
    }

    // Enemy fires
    if (id->ebullet_y < 0 && id->enemies_alive > 0 && (rand() % 3) == 0) {
        // Pick a random column that has enemies
        int tries = 10;
        while (tries-- > 0) {
            int c = rand() % ENEMY_COLS;
            for (int r = ENEMY_ROWS - 1; r >= 0; r--) {
                if (id->enemies[r][c]) {
                    id->ebullet_x = id->enemy_x + EnemyGridCol(c);
                    id->ebullet_y = id->enemy_y + r + 1;
                    goto fire_done;
                }
            }
        }
    }
fire_done:

    // AI player: move toward nearest enemy and fire
    int target = NearestEnemyCol(id);
    if (target >= 0) {
        if (id->player_x < target) id->player_x++;
        else if (id->player_x > target) id->player_x--;
    }
    if (id->player_x < PLAYER_HALF) id->player_x = PLAYER_HALF;
    if (id->player_x > grid->cols - 1 - PLAYER_HALF) id->player_x = grid->cols - 1 - PLAYER_HALF;

    // Player fires
    if (id->bullet_y < 0 && (rand() % 2) == 0) {
        id->bullet_x = id->player_x;
        id->bullet_y = PLAYER_ROW - 1;
    }

    // All enemies dead -> respawn
    if (id->enemies_alive <= 0) {
        ResetWave(id);
    }

    // Draw
    GridFillColor(grid, bg);

    // Draw enemies
    Color ec = id->colorful ? id->enemy_color : id->color;
    for (int r = 0; r < ENEMY_ROWS; r++) {
        for (int c = 0; c < ENEMY_COLS; c++) {
            if (!id->enemies[r][c]) continue;
            int ex = id->enemy_x + EnemyGridCol(c);
            int ey = id->enemy_y + r;
            if (ex >= 0 && ex < grid->cols && ey >= 0 && ey < grid->rows)
                GridSetColor(grid, (Pos){ex, ey}, ec);
        }
    }

    // Draw player
    Color pc = id->colorful ? id->player_color : id->color;
    for (int i = -PLAYER_HALF; i <= PLAYER_HALF; i++) {
        int px = id->player_x + i;
        if (px >= 0 && px < grid->cols)
            GridSetColor(grid, (Pos){px, PLAYER_ROW}, pc);
    }

    // Draw player bullet
    if (id->bullet_y >= 0 && id->bullet_y < grid->rows) {
        Color bc = id->colorful ? id->bullet_color : id->color;
        GridSetColor(grid, (Pos){id->bullet_x, id->bullet_y}, bc);
    }

    // Draw enemy bullet
    if (id->ebullet_y >= 0 && id->ebullet_y < grid->rows) {
        Color ebc = id->colorful ? id->ebullet_color : id->color;
        GridSetColor(grid, (Pos){id->ebullet_x, id->ebullet_y}, ebc);
    }
}

static void Destroy(void *data) {
    free(data);
}

Design invadersDesign = {
    .name = "invaders",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy,
};
