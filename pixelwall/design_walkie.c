/*
DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
                    Version 2, December 2004

Copyright (C) 2026 Salvatore Sanfilippo <antirez@gmail.com>

Everyone is permitted to copy and distribute verbatim or modified
copies of this license document, and changing it is allowed as long
as the name is changed.

DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION

0. You just DO WHAT THE FUCK YOU WANT TO.
*/

#include "pixelwall.h"
#include <math.h>
#include <stdio.h>
#include <stdbool.h>

#define WALKIE_DEBUG 1   /* set to 0 to disable runtime checks */

/* ---------- tunables ---------- */
#define MAX_WALKERS    256
#define HISTORY_LEN    8     /* recent positions: hard ban on revisiting */
#define FADE_STEPS     40    /* frames for color-to-green fade */
#define HOLD_STEPS     40    /* frames to hold completed pattern */
#define STALL_EPOCHS   6     /* epochs without progress before rescue */
#define RESCUE_SLIDE_COOLDOWN_S 1.0 /* min seconds between slide-swaps */
#define RESCUE_MODE_SWITCH_S 10.0 /* seconds of no progress before rescue mode swap */
#define NO_PROGRESS_TIMEOUT_S 30.0 /* fail current pattern after long no-progress */
#define DEFAULT_CYCLE  0.0208f /* seconds for one full round of moves */

/*
 * HOW THIS DESIGN WORKS (incremental overview)
 *
 * This design is a finite state machine with:
 * - 4 per-walker states: `W_WALK_IN`, `W_SETTLED`, `W_WALK_OUT`, `W_GONE`
 * - 7 global states: `GP_ASSEMBLE`, `GP_FADE`, `GP_HOLD`, `GP_UNFADE`,
 *   `GP_FAIL_FADE`, `GP_DISPERSE`, `GP_DONE`
 * The global state drives the round lifecycle, while each walker state
 * determines how that individual pixel moves.
 *
 * 1) Core model (simple part)
 *    - Each filled pattern cell is assigned to exactly one walker target.
 *      Target ownership is always a bijection.
 *    - Walkers spawn off-grid, then enter the grid and try to reach their
 *      own target.
 *    - `occ[]` stores occupancy (`-1` empty, walker index occupied), used as
 *      the single source of truth to avoid collisions.
 *
 * 2) Normal movement / assembly
 *    - The global phase starts in `GP_ASSEMBLE`.
 *    - Walkers are stepped in round-robin order (`robin`) for fairness.
 *    - `W_WALK_IN`: direction is chosen by local Monte Carlo:
 *      evaluate candidate first moves, run short random rollouts, and keep
 *      the move with the best expected score (distance closure dominates,
 *      with hit bonus when target is reached).
 *    - Recent history is used as a soft anti-oscillation ban.
 *    - A walker becomes `W_SETTLED` when it reaches its own target.
 *    - Before rescue is ever activated, we may run `reassign_targets()`:
 *      pairwise target swaps that reduce total BFS distance.
 *
 * 3) Success and failure phase machine
 *    - If all walkers settle: `GP_FADE` (to green) -> `GP_HOLD` ->
 *      `GP_UNFADE` -> `GP_DISPERSE` (walk out) -> `GP_DONE` (next pattern).
 *    - If no settled high-water progress for `NO_PROGRESS_TIMEOUT_S`:
 *      `GP_FAIL_FADE` (to red) -> `GP_DISPERSE` -> next pattern.
 *
 * 4) Stall detection (entry to rescue logic)
 *    - An "epoch" is one full round-robin cycle.
 *    - `unsolved_time[i]` counts how long target i has remained unsolved.
 *    - `max_settled` + `stall_epochs` track progress plateaus.
 *    - After `STALL_EPOCHS` without new high-water settled count, rescue mode
 *      is considered active and runs once per epoch.
 *
 * 5) Rescue mode A: trajectory cascade (first rescue mode)
 *    - Pick oldest unsolved targets first.
 *    - For a candidate walker, inspect its most obvious direction to target.
 *    - If the target lies on that exact ray and the path is blocked by a
 *      contiguous chain of settled pattern owners, perform pairwise target
 *      swaps along the chain ("cascade").
 *    - This preserves bijection and frees those settled walkers to move again,
 *      often unlocking deadlocks where one blocker prevents the last solves.
 *
 * 6) Rescue mode B: right/down slide (alternate rescue mode)
 *    - Deterministic nudge for local hole filling.
 *    - A settled walker may move to adjacent needed empty cell (right first,
 *      then down), with target ownership swapped against that destination
 *      owner. This keeps assignments consistent.
 *    - Throttled by `RESCUE_SLIDE_COOLDOWN_S` to avoid aggressive churn.
 *
 * 7) Rescue mode alternation
 *    - While still stalled, rescue alternates every
 *      `RESCUE_MODE_SWITCH_S` seconds:
 *        trajectory cascade <-> right/down slide.
 *    - Any new settled high-water progress resets stall counters and rescue
 *      alternation state.
 */

/* ---------- types ---------- */
typedef enum { W_WALK_IN, W_SETTLED, W_WALK_OUT, W_GONE } WPhase;
typedef enum { GP_ASSEMBLE, GP_FADE, GP_HOLD, GP_UNFADE, GP_FAIL_FADE, GP_DISPERSE, GP_DONE } GPhase;

typedef struct {
    Pos    cur;
    Pos    target;
    Pos    exit_pos;     /* off-grid target for walk-out */
    Color  own_color;    /* unique initial color */
    Color  draw_color;   /* currently displayed (changes during fade) */
    WPhase phase;
    Pos    history[HISTORY_LEN]; /* recent positions ring buffer */
    int    hist_idx;     /* next write position in ring buffer */
} Walker;

typedef struct {
    Walker walkers[MAX_WALKERS];
    int    nw;           /* number of walkers */
    int    robin;        /* round-robin cursor */
    GPhase gp;           /* global phase */
    int    gp_ctr;       /* counter for current phase */
    int   *occ;          /* occupancy grid: walker index or -1 */
    int    rows, cols;
    float  cycle_s;
    Pos    pattern[MAX_WALKERS];
    int    npat;
    int    pat_idx;      /* which pattern to show next */
    /* Per-target starvation counter (index follows target ownership). */
    int    unsolved_time[MAX_WALKERS];
    int    max_settled;   /* highest settled count reached so far */
    int    stall_epochs;  /* consecutive epochs without progress */
    bool   rescue_ever_activated; /* once true, keep pre-rescue reassignment off */
    double rescue_slide_next_time; /* absolute time when next slide-swap is allowed */
    bool   rescue_use_slide_mode; /* false=cascade, true=right/down slide */
    double rescue_mode_next_switch_time; /* absolute time for next mode toggle */
    double last_progress_time; /* absolute time of last settled high-water progress */
} WalkieData;

/* ---------- small helpers ---------- */
static int isign(int x) { return (x > 0) - (x < 0); }
static int mdist(Pos a, Pos b) { return abs(a.x-b.x) + abs(a.y-b.y); }

static const Pos DIRS[4] = {{1,0},{-1,0},{0,1},{0,-1}};

static bool in_grid(WalkieData *d, Pos p) {
    return p.x >= 0 && p.x < d->cols && p.y >= 0 && p.y < d->rows;
}

/* Bounds-safe grid color set — no-op for off-grid positions */
static void safe_set_color(WalkieData *d, Grid *g, Pos p, Color c) {
    if (in_grid(d, p)) GridSetColor(g, p, c);
}

/* Check if position is in walker's recent history */
static bool in_history(Walker *w, Pos p) {
    for (int i = 0; i < HISTORY_LEN; i++)
        if (w->history[i].x == p.x && w->history[i].y == p.y) return true;
    return false;
}

/* Record current position in history ring buffer */
static void record_history(Walker *w) {
    w->history[w->hist_idx] = w->cur;
    w->hist_idx = (w->hist_idx + 1) % HISTORY_LEN;
}

/* ---------- occupancy ---------- */
static int occ_get(WalkieData *d, Pos p) {
    if (p.x < 0 || p.x >= d->cols || p.y < 0 || p.y >= d->rows) return -2;
    return d->occ[p.y * d->cols + p.x];
}

static void occ_set(WalkieData *d, Pos p, int v) {
    if (p.x >= 0 && p.x < d->cols && p.y >= 0 && p.y < d->rows)
        d->occ[p.y * d->cols + p.x] = v;
}

/* ---------- debug ---------- */
#if WALKIE_DEBUG
static void occ_check(WalkieData *d, const char *where) {
    for (int i = 0; i < d->nw; i++) {
        Walker *w = &d->walkers[i];
        if (w->phase == W_GONE) continue;
        if (!in_grid(d, w->cur)) continue;
        int v = d->occ[w->cur.y * d->cols + w->cur.x];
        if (v != i) {
            fprintf(stderr, "[WALKIE BUG] %s: walker %d at (%d,%d) "
                    "but occ says %d\n", where, i, w->cur.x, w->cur.y, v);
        }
    }
    for (int i = 0; i < d->nw; i++) {
        if (d->walkers[i].phase == W_GONE) continue;
        if (!in_grid(d, d->walkers[i].cur)) continue;
        for (int j = i + 1; j < d->nw; j++) {
            if (d->walkers[j].phase == W_GONE) continue;
            if (!in_grid(d, d->walkers[j].cur)) continue;
            if (d->walkers[i].cur.x == d->walkers[j].cur.x &&
                d->walkers[i].cur.y == d->walkers[j].cur.y) {
                fprintf(stderr, "[WALKIE BUG] %s: walkers %d and %d "
                        "both at (%d,%d)\n", where, i, j,
                        d->walkers[i].cur.x, d->walkers[i].cur.y);
            }
        }
    }
}
#else
#define occ_check(d, where) ((void)0)
#endif

/* ---------- walker movement primitives ---------- */

/* Move walker to a new cell (handles off-grid positions safely) */
static void walker_place(WalkieData *d, Grid *g, int idx, Pos to) {
    Walker *w = &d->walkers[idx];
    safe_set_color(d, g, w->cur, g->conf.backgroundColor);
    occ_set(d, w->cur, -1);
    w->cur = to;
    occ_set(d, to, idx);
    safe_set_color(d, g, to, w->draw_color);
}

/* Remove walker from grid (gone off-screen) */
static void walker_remove(WalkieData *d, Grid *g, int idx) {
    Walker *w = &d->walkers[idx];
    safe_set_color(d, g, w->cur, g->conf.backgroundColor);
    occ_set(d, w->cur, -1);
    w->phase = W_GONE;
}

/* ---------- color helpers ---------- */

static Color rand_color(void) {
    int h = rand() % 360;
    float s = 0.7f + (rand() % 30) / 100.0f;
    float v = 0.8f + (rand() % 20) / 100.0f;
    float c = v * s;
    float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
    float m = v - c;
    float r, gc, b;
    switch (h / 60) {
        case 0: r=c;gc=x;b=0; break;
        case 1: r=x;gc=c;b=0; break;
        case 2: r=0;gc=c;b=x; break;
        case 3: r=0;gc=x;b=c; break;
        case 4: r=x;gc=0;b=c; break;
        default:r=c;gc=0;b=x; break;
    }
    return (Color){
        (unsigned char)((r + m) * 255),
        (unsigned char)((gc + m) * 255),
        (unsigned char)((b + m) * 255),
        255
    };
}

static Color lerp_color(Color a, Color b, float t) {
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        255
    };
}

/* ---------- spawn / edge helpers ---------- */

/* Random position off-screen, 1–6 cells beyond a random edge */
static Pos rand_offscreen(int rows, int cols) {
    int dist = 1 + rand() % 6;
    switch (rand() % 4) {
        case 0: return (Pos){rand() % cols, -dist};
        case 1: return (Pos){rand() % cols, rows - 1 + dist};
        case 2: return (Pos){-dist, rand() % rows};
        default:return (Pos){cols - 1 + dist, rand() % rows};
    }
}

/* Off-grid exit position (one step beyond a random edge) */
static Pos rand_exit(int rows, int cols) {
    switch (rand() % 4) {
        case 0: return (Pos){rand() % cols, -1};
        case 1: return (Pos){rand() % cols, rows};
        case 2: return (Pos){-1, rand() % rows};
        default:return (Pos){cols, rand() % rows};
    }
}

/* ---------- pattern ---------- */

/* Human-editable full maps: 16 rows x 22 columns.
 * Use '*' for a filled pattern cell and '.' for empty.
 * Edit these blocks directly to tweak shapes. */
#define EDIT_MAP_W 22
#define EDIT_MAP_H 16

static void make_pattern_from_edit_map(
        WalkieData *d,
        const char map[EDIT_MAP_H][EDIT_MAP_W + 1])
{
    int x0 = (d->cols - EDIT_MAP_W) / 2;
    int y0 = (d->rows - EDIT_MAP_H) / 2;

    d->npat = 0;
    for (int y = 0; y < EDIT_MAP_H; y++) {
        for (int x = 0; x < EDIT_MAP_W; x++) {
            if (map[y][x] != '*') continue;
            int gx = x0 + x;
            int gy = y0 + y;
            if (gx < 0 || gx >= d->cols || gy < 0 || gy >= d->rows) continue;
            if (d->npat < MAX_WALKERS)
                d->pattern[d->npat++] = (Pos){gx, gy};
        }
    }
}

static const char pattern_manic_map[EDIT_MAP_H][EDIT_MAP_W + 1] = {
    "............**........",
    ".........*****........",
    "........*****.........",
    ".........**.*.........",
    ".........*****........",
    ".........****.........",
    "..........**..........",
    ".........****.........",
    "........**.***........",
    "........**.***........",
    "........**.***........",
    "........***.**........",
    ".........****.........",
    "..........**..........",
    "..........**..........",
    "..........***.........",
};

static const char pattern_hack_map[EDIT_MAP_H][EDIT_MAP_W + 1] = {
    "......................",
    "......................",
    "......................",
    "......................",
    "......................",
    "...*.*..*..***.*.*....",
    "...*.*.*.*.*...**.....",
    "...***.***.*...*......",
    "...*.*.*.*.*...**.....",
    "...*.*.*.*.***.*.*....",
    "......................",
    "......................",
    "......................",
    "......................",
    "......................",
    "......................",
};

static const char pattern_101_map[EDIT_MAP_H][EDIT_MAP_W + 1] = {
    "......................",
    "......................",
    "......................",
    "......................",
    "....*....***....*.....",
    "...**...*...*..**.....",
    "....*...*...*...*.....",
    "....*...*...*...*.....",
    "....*...*...*...*.....",
    "....*...*...*...*.....",
    "...***...***...***....",
    "......................",
    "......................",
    "......................",
    "......................",
    "......................",
};

static const char pattern_spaceinvaders_map[EDIT_MAP_H][EDIT_MAP_W + 1] = {
    "......................",
    "....*.....*.........**",
    ".....*...*.........***",
    "....*******.......****",
    "...**.***.**.....**.**",
    "..***********....*****",
    "..*.*******.*......*..",
    "..*.*.....*.*.....*.**",
    ".....**.**.......*.*..",
    "......................",
    "......................",
    "........*.............",
    "........*.............",
    "........*..........*..",
    "...................*..",
    "...................*..",
};

static void make_pattern_hack(WalkieData *d) {
    make_pattern_from_edit_map(d, pattern_hack_map);
}

static void make_pattern_101(WalkieData *d) {
    make_pattern_from_edit_map(d, pattern_101_map);
}

static void make_pattern_manic(WalkieData *d) {
    make_pattern_from_edit_map(d, pattern_manic_map);
}

static void make_pattern_spaceinvaders(WalkieData *d) {
    make_pattern_from_edit_map(d, pattern_spaceinvaders_map);
}

static void dedup_pattern(WalkieData *d) {
    int out = 0;
    for (int i = 0; i < d->npat; i++) {
        Pos p = d->pattern[i];
        bool seen = false;
        for (int j = 0; j < out; j++) {
            if (d->pattern[j].x == p.x && d->pattern[j].y == p.y) {
                seen = true;
                break;
            }
        }
        if (!seen) d->pattern[out++] = p;
    }
    d->npat = out;
}

static void make_pattern(WalkieData *d) {
    typedef void (*PatternMaker)(WalkieData *);
    static const PatternMaker makers[] = {
        make_pattern_hack,
        make_pattern_spaceinvaders,
        make_pattern_101,
        make_pattern_manic,
    };
    int nmakers = (int)(sizeof(makers) / sizeof(makers[0]));

    makers[d->pat_idx % nmakers](d);
    dedup_pattern(d);
    d->pat_idx++;
}

/* ---------- BFS distance & target reassignment ---------- */

/* BFS shortest path distance from 'from' to 'to' on the grid.
 * Settled walkers are treated as walls. Returns large value if
 * unreachable. For off-grid 'from', returns Manhattan distance. */
static int bfs_dist(WalkieData *d, Pos from, Pos to) {
    if (!in_grid(d, from))
        return mdist(from, to);
    if (from.x == to.x && from.y == to.y) return 0;

    int cells = d->rows * d->cols;
    int *dist = malloc(cells * sizeof(int));
    for (int i = 0; i < cells; i++) dist[i] = -1;

    int *q = malloc(cells * sizeof(int));
    int head = 0, tail = 0;

    int si = from.y * d->cols + from.x;
    dist[si] = 0;
    q[tail++] = si;

    int ti = to.y * d->cols + to.x;

    while (head < tail) {
        int ci = q[head++];
        if (ci == ti) break;
        int cx = ci % d->cols, cy = ci / d->cols;
        for (int di = 0; di < 4; di++) {
            int nx = cx + DIRS[di].x, ny = cy + DIRS[di].y;
            if (nx < 0 || nx >= d->cols || ny < 0 || ny >= d->rows) continue;
            int ni = ny * d->cols + nx;
            if (dist[ni] >= 0) continue;
            /* Settled walkers are walls (unless it's the target cell) */
            if (ni != ti) {
                int oc = d->occ[ni];
                if (oc >= 0 && d->walkers[oc].phase == W_SETTLED) continue;
            }
            dist[ni] = dist[ci] + 1;
            q[tail++] = ni;
        }
    }

    int result = dist[ti] >= 0 ? dist[ti] : 9999;
    free(dist);
    free(q);
    return result;
}

/* Swap target ownership between two walkers.
 * unsolved_time tracks target age, so it must move with the target. */
static void swap_targets(WalkieData *d, int a, int b) {
    Pos ptmp = d->walkers[a].target;
    d->walkers[a].target = d->walkers[b].target;
    d->walkers[b].target = ptmp;

    int itmp = d->unsolved_time[a];
    d->unsolved_time[a] = d->unsolved_time[b];
    d->unsolved_time[b] = itmp;
}

static int find_target_owner(WalkieData *d, Pos p);

static void reset_history(Walker *w) {
    w->hist_idx = 0;
    for (int k = 0; k < HISTORY_LEN; k++)
        w->history[k] = (Pos){-999, -999};
}

/* Rescue-mode deterministic nudge:
 * if a settled pixel can cover an adjacent empty needed pattern cell,
 * move it there (right first, then down), swapping target ownership with
 * the current owner of that destination target. */
static bool rescue_slide_settled_right_down(WalkieData *d, Grid *g) {
    const Pos pref[2] = {{1,0}, {0,1}}; /* right, then down */
    double now = GetTime();
    if (now < d->rescue_slide_next_time) return false;

    for (int i = 0; i < d->nw; i++) {
        Walker *ws = &d->walkers[i];
        if (ws->phase != W_SETTLED || !in_grid(d, ws->cur)) continue;

        for (int k = 0; k < 2; k++) {
            Pos dst = {ws->cur.x + pref[k].x, ws->cur.y + pref[k].y};
            if (!in_grid(d, dst)) continue;
            if (occ_get(d, dst) != -1) continue; /* must be empty */

            int dst_owner = find_target_owner(d, dst);
            if (dst_owner < 0 || dst_owner == i) continue; /* not a needed pattern cell */
            if (d->walkers[dst_owner].phase == W_SETTLED) continue; /* already covered */

            /* Keep assignment bijective, then physically cover dst now. */
            swap_targets(d, i, dst_owner);
            walker_place(d, g, i, dst);
            d->walkers[i].phase = W_SETTLED;
            reset_history(&d->walkers[i]);
            d->unsolved_time[i] = 0;

            /* The displaced target owner now has a fresh target (old ws target). */
            reset_history(&d->walkers[dst_owner]);
            d->unsolved_time[dst_owner] = 0;
            d->rescue_slide_next_time = now + RESCUE_SLIDE_COOLDOWN_S;
            return true;
        }
    }
    return false;
}

/* Attempt rescue by cascading target swaps along the obvious blocked line.
 *
 * Idea: for an unsolved walker 'worst', look at its primary direction to
 * target. If it is blocked by settled walkers and target lies on that same
 * ray, rotate target ownership along the chain with pairwise swaps:
 *   worst <-> w1, then w1 <-> w2, ... so no target is ever unassigned.
 */
static bool rescue_cascade_line(WalkieData *d, int worst) {
    Walker *ww = &d->walkers[worst];
    if (ww->phase == W_SETTLED || !in_grid(d, ww->cur)) return false;

    int dx = ww->target.x - ww->cur.x;
    int dy = ww->target.y - ww->cur.y;
    if (dx == 0 && dy == 0) return false;

    int dir_order[2];
    int nd = 0;
    if (abs(dx) >= abs(dy)) {
        if (dx) dir_order[nd++] = (dx > 0) ? 0 : 1;
        if (dy) dir_order[nd++] = (dy > 0) ? 2 : 3;
    } else {
        if (dy) dir_order[nd++] = (dy > 0) ? 2 : 3;
        if (dx) dir_order[nd++] = (dx > 0) ? 0 : 1;
    }

    for (int odi = 0; odi < nd; odi++) {
        int dir = dir_order[odi];
        Pos stepv = DIRS[dir];

        /* Target must be reachable on this exact ray. */
        if (stepv.x != 0) {
            if (ww->target.y != ww->cur.y) continue;
            if ((ww->target.x - ww->cur.x) * stepv.x <= 0) continue;
        } else {
            if (ww->target.x != ww->cur.x) continue;
            if ((ww->target.y - ww->cur.y) * stepv.y <= 0) continue;
        }

        /* Must be blocked immediately by a settled pixel on pattern. */
        Pos first = {ww->cur.x + stepv.x, ww->cur.y + stepv.y};
        if (!in_grid(d, first)) continue;
        int first_owner = find_target_owner(d, first);
        if (first_owner < 0 || first_owner == worst) continue;
        if (d->walkers[first_owner].phase != W_SETTLED) continue;

        int chain[MAX_WALKERS];
        int chain_len = 0;
        Pos p = ww->cur;
        bool valid = false;

        while (1) {
            p.x += stepv.x;
            p.y += stepv.y;
            if (!in_grid(d, p)) { chain_len = 0; break; }
            if (p.x == ww->target.x && p.y == ww->target.y) {
                valid = true;
                break;
            }

            int owner = find_target_owner(d, p);
            if (owner < 0) continue; /* non-pattern cell */
            if (owner == worst || d->walkers[owner].phase != W_SETTLED) {
                chain_len = 0;
                valid = false;
                break;
            }
            if (chain_len < MAX_WALKERS)
                chain[chain_len++] = owner;
        }

        if (!valid || chain_len == 0) continue;

        /* Cascading reassignment using swaps keeps assignment bijective. */
        int cur_owner = worst;
        for (int i = 0; i < chain_len; i++) {
            int nxt = chain[i];
            swap_targets(d, cur_owner, nxt);
            cur_owner = nxt;
        }

        for (int i = 0; i < chain_len; i++) {
            int wi = chain[i];
            if (d->walkers[wi].phase == W_SETTLED) {
                d->walkers[wi].phase = W_WALK_IN;
            }
            reset_history(&d->walkers[wi]);
            d->unsolved_time[wi] = 0;
        }
        reset_history(&d->walkers[worst]);
        d->unsolved_time[worst] = 0;
        return true;
    }

    return false;
}

/* Return which walker currently owns target position p, or -1 if none. */
static int find_target_owner(WalkieData *d, Pos p) {
    for (int i = 0; i < d->nw; i++) {
        if (d->walkers[i].target.x == p.x && d->walkers[i].target.y == p.y)
            return i;
    }
    return -1;
}

/* Pre-rescue optimizer: pairwise swap non-settled targets when total
 * BFS distance improves. Disabled forever after rescue mode is entered. */
static void reassign_targets(WalkieData *d) {
    bool improved = true;
    while (improved) {
        improved = false;
        for (int i = 0; i < d->nw; i++) {
            if (d->walkers[i].phase == W_SETTLED) continue;
            if (!in_grid(d, d->walkers[i].cur)) continue;
            for (int j = i + 1; j < d->nw; j++) {
                if (d->walkers[j].phase == W_SETTLED) continue;
                if (!in_grid(d, d->walkers[j].cur)) continue;
                int di_ti = bfs_dist(d, d->walkers[i].cur, d->walkers[i].target);
                int dj_tj = bfs_dist(d, d->walkers[j].cur, d->walkers[j].target);
                int di_tj = bfs_dist(d, d->walkers[i].cur, d->walkers[j].target);
                int dj_ti = bfs_dist(d, d->walkers[j].cur, d->walkers[i].target);
                if (di_tj + dj_ti < di_ti + dj_tj) {
                    swap_targets(d, i, j);
                    improved = true;
                }
            }
        }
    }
}

/* Called once per epoch (full round-robin cycle).
 * Updates per-target unsolved counters, detects global stall,
 * and performs one rescue. */
static bool epoch_update(WalkieData *d, Grid *g) {
    /* 1. Count settled and update per-target unsolved_time */
    int settled = 0;
    for (int i = 0; i < d->nw; i++) {
        if (d->walkers[i].phase == W_SETTLED) {
            settled++;
            d->unsolved_time[i] = 0;
        } else {
            d->unsolved_time[i]++;
        }
    }

    /* 2. Detect stall from progress high-water mark.
     * Once stall mode starts, keep rescuing every epoch until we beat
     * the previous best settled count. */
    if (settled > d->max_settled) {
        d->max_settled = settled;
        d->stall_epochs = 0;
        d->last_progress_time = GetTime();
        d->rescue_use_slide_mode = false;
        d->rescue_mode_next_switch_time = 0.0;
    } else {
        d->stall_epochs++;
    }

    /* Not stalled yet, or everything solved — nothing to do */
    if (d->stall_epochs < STALL_EPOCHS || settled == d->nw)
        return false;

    /* Stall/rescue mode has been entered at least once. */
    d->rescue_ever_activated = true;

    /* Alternate rescue modes every RESCUE_MODE_SWITCH_S while no progress:
     * start with trajectory/cascade, then switch with right/down slide. */
    double now = GetTime();
    if (d->rescue_mode_next_switch_time <= 0.0) {
        d->rescue_use_slide_mode = false; /* start from trajectory fix */
        d->rescue_mode_next_switch_time = now + RESCUE_MODE_SWITCH_S;
    } else {
        while (now >= d->rescue_mode_next_switch_time) {
            d->rescue_use_slide_mode = !d->rescue_use_slide_mode;
            d->rescue_mode_next_switch_time += RESCUE_MODE_SWITCH_S;
        }
    }

    if (d->rescue_use_slide_mode) {
        /* In slide mode, apply only right/down deterministic fill. */
        return rescue_slide_settled_right_down(d, g);
    }

    /* 3. Trajectory mode: try unsolved targets from oldest to youngest.
     * Only one rescue per epoch for a smooth incremental visual effect. */
    bool tried[MAX_WALKERS] = {0};
    for (int attempt = 0; attempt < d->nw; attempt++) {
        int worst = -1, worst_time = 0;
        for (int i = 0; i < d->nw; i++) {
            if (tried[i]) continue;
            if (d->walkers[i].phase == W_SETTLED) continue;
            if (d->unsolved_time[i] > worst_time) {
                worst_time = d->unsolved_time[i];
                worst = i;
            }
        }
        if (worst < 0 || worst_time < STALL_EPOCHS) break;
        tried[worst] = true;

        /* Only rescue strategy: cascading swaps along the obvious blocked
         * line. This keeps target assignment bijective by construction. */
        if (rescue_cascade_line(d, worst))
            return true;
    }
    return false;
}

/* ---------- init walkers for a new round ---------- */

static void init_walkers(WalkieData *d, Grid *g) {
    d->nw = d->npat;
    d->robin = 0;
    d->gp = GP_ASSEMBLE;
    d->gp_ctr = 0;
    d->max_settled = 0;
    d->stall_epochs = 0;
    d->rescue_ever_activated = false;
    d->rescue_slide_next_time = 0.0;
    d->rescue_use_slide_mode = false;
    d->rescue_mode_next_switch_time = 0.0;
    d->last_progress_time = GetTime();
    for (int i = 0; i < d->nw; i++)
        d->unsolved_time[i] = 0;

    GridFillColor(g, g->conf.backgroundColor);
    for (int i = 0; i < d->rows * d->cols; i++)
        d->occ[i] = -1;

    for (int i = 0; i < d->nw; i++) {
        Walker *w = &d->walkers[i];
        w->target = d->pattern[i];
        w->own_color = rand_color();
        w->draw_color = w->own_color;
        w->phase = W_WALK_IN;
        w->hist_idx = 0;
        for (int j = 0; j < HISTORY_LEN; j++)
            w->history[j] = (Pos){-999, -999};
        w->cur = rand_offscreen(d->rows, d->cols);
    }
}

/* ---------- direction picking ---------- */

/* Monte Carlo direction picker for WALK_IN.
 * For each candidate first move, run multiple random simulations of
 * varying lengths. Score each simulation based on how close it got
 * to the target, with a big bonus for reaching it and only a mild
 * penalty for longer paths. The first move with the highest score wins. */
#define MC_SIMS        10   /* simulations per candidate direction */
#define MC_MIN_DEPTH   10   /* min steps in a single simulation */
#define MC_MAX_DEPTH   15   /* max steps in a single simulation */
#define MC_HIT_BONUS   50.0f /* bonus for reaching the target */
#define MC_DEPTH_PENALTY 0.20f /* mild cost per simulated step */

/* Cost of reaching a free border ingress from an off-grid position.
 * Lower is better. If no ingress is currently free, return a large fallback. */
static int offgrid_ingress_cost(WalkieData *d, Pos from, Pos target) {
    int best = 1 << 30;

    for (int x = 0; x < d->cols; x++) {
        Pos a = {x, 0};
        Pos b = {x, d->rows - 1};
        if (occ_get(d, a) == -1) {
            int c = mdist(from, a) * 4 + mdist(a, target);
            if (c < best) best = c;
        }
        if (d->rows > 1 && occ_get(d, b) == -1) {
            int c = mdist(from, b) * 4 + mdist(b, target);
            if (c < best) best = c;
        }
    }
    for (int y = 1; y < d->rows - 1; y++) {
        Pos a = {0, y};
        Pos b = {d->cols - 1, y};
        if (occ_get(d, a) == -1) {
            int c = mdist(from, a) * 4 + mdist(a, target);
            if (c < best) best = c;
        }
        if (d->cols > 1 && occ_get(d, b) == -1) {
            int c = mdist(from, b) * 4 + mdist(b, target);
            if (c < best) best = c;
        }
    }

    if (best == (1 << 30))
        return 100000 + mdist(from, target) * 16;
    return best;
}

static Pos pick_walk_in_dir(Walker *w, WalkieData *d) {
    /* Off-grid: move toward target, but allow edge sidestep when the direct
     * border entry is blocked. This avoids walkers stalling forever outside
     * when patterns touch a screen edge. */
    if (!in_grid(d, w->cur)) {
        int best_enter_dirs[4];
        int nbest_enter = 0;
        int best_enter_score = 1 << 30;
        int best_off_dirs[4];
        int nbest_off = 0;
        int best_off_score = 1 << 30;

        for (int i = 0; i < 4; i++) {
            Pos np = {w->cur.x + DIRS[i].x, w->cur.y + DIRS[i].y};
            int oc = occ_get(d, np);

            /* Same movement constraints as step_one(). */
            if (!(oc == -1 || (oc == -2 && !in_grid(d, w->cur)))) continue;

            if (in_grid(d, np)) {
                /* If we can enter now, prioritize entering immediately. */
                int score = mdist(np, w->target);
                if (in_history(w, np)) score += 3;
                if (score < best_enter_score) {
                    best_enter_score = score;
                    best_enter_dirs[0] = i;
                    nbest_enter = 1;
                } else if (score == best_enter_score && nbest_enter < 4) {
                    best_enter_dirs[nbest_enter++] = i;
                }
            } else {
                /* Off-grid moves should seek the nearest free ingress lane. */
                int score = offgrid_ingress_cost(d, np, w->target);
                if (in_history(w, np)) score += 3;
                if (score < best_off_score) {
                    best_off_score = score;
                    best_off_dirs[0] = i;
                    nbest_off = 1;
                } else if (score == best_off_score && nbest_off < 4) {
                    best_off_dirs[nbest_off++] = i;
                }
            }
        }

        if (nbest_enter > 0)
            return DIRS[best_enter_dirs[rand() % nbest_enter]];
        if (nbest_off > 0)
            return DIRS[best_off_dirs[rand() % nbest_off]];
        return (Pos){0, 0};
    }

    int start_dist = mdist(w->cur, w->target);
    float scores[4] = {0, 0, 0, 0};
    bool viable[4] = {false, false, false, false};

    for (int i = 0; i < 4; i++) {
        Pos first = {w->cur.x + DIRS[i].x, w->cur.y + DIRS[i].y};
        /* First step must be to a free cell */
        if (occ_get(d, first) != -1) continue;
        /* History ban (soft: skip if alternatives exist, checked later) */
        viable[i] = true;

        for (int sim = 0; sim < MC_SIMS; sim++) {
            /* Random simulation depth: MC_MIN_DEPTH to MC_MAX_DEPTH */
            int depth = MC_MIN_DEPTH + rand() % (MC_MAX_DEPTH - MC_MIN_DEPTH + 1);
            Pos pos = first;
            int best_dist = mdist(pos, w->target);
            bool hit = (best_dist == 0);

            for (int step = 1; step < depth && !hit; step++) {
                /* Pick a random free neighbor */
                int perm[4] = {0,1,2,3};
                for (int j = 3; j > 0; j--) {
                    int k = rand() % (j + 1);
                    int tmp = perm[j]; perm[j] = perm[k]; perm[k] = tmp;
                }
                bool moved = false;
                for (int j = 0; j < 4; j++) {
                    Pos np = {pos.x + DIRS[perm[j]].x, pos.y + DIRS[perm[j]].y};
                    if (occ_get(d, np) == -1) {
                        pos = np;
                        moved = true;
                        break;
                    }
                }
                if (!moved) break;
                int dd = mdist(pos, w->target);
                if (dd < best_dist) best_dist = dd;
                if (dd == 0) hit = true;
            }

            /* Score: prioritize how much distance was closed.
             * Use squared gain so bigger closures dominate path length. */
            int closed = start_dist - best_dist;
            float close_score = (closed >= 0) ? (float)(closed * closed)
                                              : -(float)(closed * closed);
            float score = close_score - MC_DEPTH_PENALTY * depth;
            if (hit) score += MC_HIT_BONUS;
            scores[i] += score;
        }
    }

    /* Apply history ban: prefer non-banned directions */
    float best_score = -1e9f;
    int best_dir = -1;

    for (int i = 0; i < 4; i++) {
        if (!viable[i]) continue;
        Pos next = {w->cur.x + DIRS[i].x, w->cur.y + DIRS[i].y};
        if (!in_history(w, next)) {
            if (scores[i] > best_score) {
                best_score = scores[i];
                best_dir = i;
            }
        }
    }

    /* Fallback to banned if no unbanned option */
    if (best_dir < 0) {
        for (int i = 0; i < 4; i++) {
            if (!viable[i]) continue;
            if (scores[i] > best_score) {
                best_score = scores[i];
                best_dir = i;
            }
        }
    }

    return (best_dir >= 0) ? DIRS[best_dir] : (Pos){0, 0};
}

/* WALK_OUT: head toward exit, free cells or off-grid */
static Pos pick_walk_out_dir(Walker *w, WalkieData *d) {
    int dx = isign(w->exit_pos.x - w->cur.x);
    int dy = isign(w->exit_pos.y - w->cur.y);

    /* Build preferred directions (longer axis first) */
    Pos prefs[2];
    int np = 0;
    if (abs(w->exit_pos.x - w->cur.x) >= abs(w->exit_pos.y - w->cur.y)) {
        if (dx) prefs[np++] = (Pos){dx, 0};
        if (dy) prefs[np++] = (Pos){0, dy};
    } else {
        if (dy) prefs[np++] = (Pos){0, dy};
        if (dx) prefs[np++] = (Pos){dx, 0};
    }

    for (int i = 0; i < np; i++) {
        Pos next = {w->cur.x + prefs[i].x, w->cur.y + prefs[i].y};
        int oc = occ_get(d, next);
        if (oc == -1 || oc == -2) return prefs[i];
    }

    /* Any free or off-grid direction */
    for (int i = 0; i < 4; i++) {
        Pos next = {w->cur.x + DIRS[i].x, w->cur.y + DIRS[i].y};
        int oc = occ_get(d, next);
        if (oc == -1 || oc == -2) return DIRS[i];
    }

    return (Pos){0, 0};
}

/* ---------- step one walker ---------- */

static void step_one(WalkieData *d, Grid *g, int idx) {
    Walker *w = &d->walkers[idx];

    /* Already at target? Settle. */
    if (w->phase == W_WALK_IN &&
        w->cur.x == w->target.x && w->cur.y == w->target.y) {
        w->phase = W_SETTLED;
        return;
    }

    record_history(w);

    Pos dir = {0, 0};
    switch (w->phase) {
        case W_WALK_IN:
            dir = pick_walk_in_dir(w, d);
            break;
        case W_WALK_OUT:
            dir = pick_walk_out_dir(w, d);
            break;
        default:
            return;
    }

    if (dir.x == 0 && dir.y == 0) return;

    Pos next = {w->cur.x + dir.x, w->cur.y + dir.y};

    /* Walking out and stepping off grid → gone */
    if (w->phase == W_WALK_OUT &&
        (next.x < 0 || next.x >= d->cols || next.y < 0 || next.y >= d->rows)) {
        walker_remove(d, g, idx);
        return;
    }

    int oc = occ_get(d, next);
    if (oc == -1 || (oc == -2 && !in_grid(d, w->cur))) {
        walker_place(d, g, idx, next);

        if (w->phase == W_WALK_IN && in_grid(d, w->cur)) {
            if (w->cur.x == w->target.x && w->cur.y == w->target.y) {
                /* Settle immediately when reaching own target. */
                d->walkers[idx].phase = W_SETTLED;
                d->unsolved_time[idx] = 0;
            }
        }
    }

    occ_check(d, "step_one");
}

/* Count walkers in a given phase. */
static int count_walkers_in_phase(WalkieData *d, WPhase phase) {
    int n = 0;
    for (int i = 0; i < d->nw; i++) {
        if (d->walkers[i].phase == phase) n++;
    }
    return n;
}

/* Start the walk-out phase for every walker still present on screen. */
static void start_disperse(WalkieData *d) {
    d->gp = GP_DISPERSE;
    d->gp_ctr = 0;
    d->robin = 0;
    for (int i = 0; i < d->nw; i++) {
        if (d->walkers[i].phase == W_GONE) continue;
        d->walkers[i].phase = W_WALK_OUT;
        d->walkers[i].exit_pos = rand_exit(d->rows, d->cols);
    }
}

/* ---------- design interface ---------- */

static void PrintHelp(void) {
    printf("  -W <seconds>     Walk cycle time (default: %.4f)\n", DEFAULT_CYCLE);
    printf("  Key N            Skip to next walkie pattern\n");
}

static void *Create(Grid *grid, int argc, char *argv[]) {
    WalkieData *d = calloc(1, sizeof(WalkieData));
    if (!d) return NULL;

    d->rows = grid->rows;
    d->cols = grid->cols;
    d->cycle_s = DEFAULT_CYCLE;

    int opt;
    optind = 1;
    while ((opt = getopt(argc, argv, ":W:")) != -1) {
        switch (opt) {
            case 'W':
                d->cycle_s = (float)atof(optarg);
                if (d->cycle_s < 0.001f) d->cycle_s = 0.001f;
                break;
        }
    }

    int cells = d->rows * d->cols;
    d->occ = malloc(cells * sizeof(int));
    if (!d->occ) { free(d); return NULL; }

    srand((unsigned int)time(NULL));

    d->pat_idx = 0;
    make_pattern(d);
    init_walkers(d, grid);

    grid->conf.moveInterval = d->cycle_s;

    return d;
}

static void UpdateFrame(Grid *grid, void *data) {
    WalkieData *d = (WalkieData *)data;

    if (IsKeyPressed(KEY_N)) {
        make_pattern(d);
        init_walkers(d, grid);
        return;
    }

    switch (d->gp) {

    case GP_ASSEMBLE: {
        int steps = d->nw / 4;
        if (steps < 1) steps = 1;

        for (int s = 0; s < steps; s++) {
            /* Every full round-robin cycle:
             * - check stalls and rescue via directional cascading swaps
             * - before rescue mode ever activates, allow reassignment pass */
            if (d->robin == 0) {
                int settled_now = count_walkers_in_phase(d, W_SETTLED);
                if (settled_now < d->nw &&
                    (GetTime() - d->last_progress_time) >= NO_PROGRESS_TIMEOUT_S) {
                    d->gp = GP_FAIL_FADE;
                    d->gp_ctr = 0;
                    return;
                }

                bool rescued = epoch_update(d, grid);
                if (!rescued && !d->rescue_ever_activated)
                    reassign_targets(d);
            }

            int idx = d->robin;
            d->robin = (d->robin + 1) % d->nw;

            if (d->walkers[idx].phase == W_SETTLED) {
                /* Switch phase once every target is covered. */
                if (count_walkers_in_phase(d, W_SETTLED) == d->nw) {
                    d->gp = GP_FADE;
                    d->gp_ctr = 0;
                    return;
                }
                continue;
            }

            step_one(d, grid, idx);
        }
        return;
    }

    case GP_FADE: {
        d->gp_ctr++;
        float t = (float)d->gp_ctr / FADE_STEPS;
        if (t > 1.0f) t = 1.0f;

        for (int i = 0; i < d->nw; i++) {
            Walker *w = &d->walkers[i];
            w->draw_color = lerp_color(w->own_color, GREEN, t);
            GridSetColor(grid, w->cur, w->draw_color);
        }

        if (d->gp_ctr >= FADE_STEPS) {
            d->gp = GP_HOLD;
            d->gp_ctr = 0;
        }
        break;
    }

    case GP_HOLD: {
        d->gp_ctr++;
        if (d->gp_ctr >= HOLD_STEPS) {
            d->gp = GP_UNFADE;
            d->gp_ctr = 0;
        }
        break;
    }

    case GP_UNFADE: {
        d->gp_ctr++;
        float t = (float)d->gp_ctr / FADE_STEPS;
        if (t > 1.0f) t = 1.0f;

        for (int i = 0; i < d->nw; i++) {
            Walker *w = &d->walkers[i];
            w->draw_color = lerp_color(GREEN, w->own_color, t);
            GridSetColor(grid, w->cur, w->draw_color);
        }

        if (d->gp_ctr >= FADE_STEPS) {
            start_disperse(d);
        }
        break;
    }

    case GP_FAIL_FADE: {
        d->gp_ctr++;
        float t = (float)d->gp_ctr / FADE_STEPS;
        if (t > 1.0f) t = 1.0f;

        for (int i = 0; i < d->nw; i++) {
            Walker *w = &d->walkers[i];
            if (w->phase == W_GONE) continue;
            w->draw_color = lerp_color(w->own_color, RED, t);
            safe_set_color(d, grid, w->cur, w->draw_color);
        }

        if (d->gp_ctr >= FADE_STEPS) {
            start_disperse(d);
        }
        break;
    }

    case GP_DISPERSE: {
        int steps = d->nw / 4;
        if (steps < 1) steps = 1;

        for (int s = 0; s < steps; s++) {
            int idx = d->robin;
            d->robin = (d->robin + 1) % d->nw;

            if (d->walkers[idx].phase == W_GONE) {
                if (count_walkers_in_phase(d, W_GONE) == d->nw) {
                    d->gp = GP_DONE;
                    return;
                }
                continue;
            }

            step_one(d, grid, idx);
        }
        return;
    }

    case GP_DONE:
        make_pattern(d);
        init_walkers(d, grid);
        break;
    }
}

static void Destroy(void *data) {
    WalkieData *d = (WalkieData *)data;
    if (d) {
        free(d->occ);
        free(d);
    }
}

Design walkieDesign = {
    .name = "walkie",
    .PrintHelp = PrintHelp,
    .Create = Create,
    .UpdateFrame = UpdateFrame,
    .Destroy = Destroy,
};
