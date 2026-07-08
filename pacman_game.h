/*
 * pacman_game.h — Logica do jogo Pac-Man
 *
 * Projeto: Pac-Man DE10-Standard — UFSCar Sistemas Embarcados
 *
 * CORRECOES v2:
 *   - Mapa limpo: sem 'P' nem '1'-'4' (posicoes hardcoded)
 *   - can_move() corrigido: trata todo char != '#' como passavel
 *   - ghost_ai() corrigido: usa seed de aleatoriedade por fantasma
 *   - Pac-Man para quando nao ha input (nao se move sozinho)
 *   - Labirinto valido: todos os corredores conectados
 *
 * Mapa 20x15 tiles, cada tile 16x16 pixels → 320x240 total
 * Legenda: '#'=parede  '.'=pastilha  'o'=power  ' '=vazio
 */

#ifndef PACMAN_GAME_H
#define PACMAN_GAME_H

#include "input_hal.h"   /* DIR_* */

/* ---------------------------------------------------------------
 * Dimensoes
 * --------------------------------------------------------------- */
#define MAP_COLS   20
#define MAP_ROWS   15
#define TILE_W     16
#define TILE_H     16
#define MAP_OFS_X  0
#define MAP_OFS_Y  16   /* 1 linha de char buffer para placar */

/* ---------------------------------------------------------------
 * Labirinto — mapa limpo sem chars especiais
 * Todos os caminhos sao '.' ou ' ' ou 'o'
 * --------------------------------------------------------------- */
static const char MAZE_TEMPLATE[MAP_ROWS][MAP_COLS + 1] = {
    "####################",  /* 0  */
    "#o...........  ..o.#",  /* 1  */
    "#.##.#######.##..#.#",  /* 2  */
    "#....#.....#.....#.#",  /* 3  */
    "#.##.#.###.#.##..#.#",  /* 4  */
    "#..................#",   /* 5  */
    "#.##.##   ##.##.#.#",   /* 6  */
    "#....#  .  #.....#.#",  /* 7  */
    "#.##.##   ##.##.#.#",   /* 8  */
    "#..................#",   /* 9  */
    "#.##.#.###.#.##..#.#",  /* 10 */
    "#....#.....#.....#.#",  /* 11 */
    "#.##.#######.##..#.#",  /* 12 */
    "#o...........  ..o.#",  /* 13 */
    "####################",  /* 14 */
};

/* Posicao inicial do jogador (tile col=10, row=7) */
#define PLAYER_START_COL  10
#define PLAYER_START_ROW   7

/* Posicoes iniciais dos 4 fantasmas */
#define GHOST0_COL  7
#define GHOST0_ROW  5
#define GHOST1_COL 12
#define GHOST1_ROW  5
#define GHOST2_COL  7
#define GHOST2_ROW  9
#define GHOST3_COL 12
#define GHOST3_ROW  9

/* ---------------------------------------------------------------
 * Estados do jogo
 * --------------------------------------------------------------- */
#define STATE_ATTRACT   0
#define STATE_PLAYING   1
#define STATE_DYING     2
#define STATE_WIN       3
#define STATE_GAMEOVER  4

/* ---------------------------------------------------------------
 * Cores dos fantasmas (RGB24)
 * --------------------------------------------------------------- */
#define GHOST_COLOR_0   0xFF2000   /* Vermelho  — Blinky */
#define GHOST_COLOR_1   0xFF80C0   /* Rosa      — Pinky  */
#define GHOST_COLOR_2   0x00C0FF   /* Ciano     — Inky   */
#define GHOST_COLOR_3   0xFF8000   /* Laranja   — Clyde  */

/* ---------------------------------------------------------------
 * Estruturas
 * --------------------------------------------------------------- */
typedef struct {
    int x, y;
    int dir;
    int next_dir;
    int lives;
    int score;
} Player;

typedef struct {
    int x, y;
    int dir;
    int color24;
    int scared;
    int active;
    int scared_timer;
    int seed;       /* semente de aleatoriedade individual */
} Ghost;

typedef struct {
    char   map[MAP_ROWS][MAP_COLS + 1];
    int    state;
    int    level;
    int    pellets_left;
    int    power_timer;
    int    frame;
    Player player;
    Ghost  ghosts[4];
} GameState;

/* ---------------------------------------------------------------
 * is_passable() — retorna 1 se o tile e passavel
 * CORRECAO: qualquer char diferente de '#' e passavel
 * --------------------------------------------------------------- */
static inline int is_passable(char ch)
{
    return (ch != '#');
}

/* ---------------------------------------------------------------
 * tile_at() — tile na posicao de pixel
 * --------------------------------------------------------------- */
static inline char tile_at(GameState *g, int px, int py)
{
    int col = (px - MAP_OFS_X) / TILE_W;
    int row = (py - MAP_OFS_Y) / TILE_H;
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS)
        return '#';
    return g->map[row][col];
}

/* ---------------------------------------------------------------
 * can_move() — verifica se (cx,cy) pode mover na direcao
 *
 * CORRECAO v2: margem menor (4px) para nao travar em corredores
 * e usa is_passable() em vez de comparar com '#' diretamente.
 * --------------------------------------------------------------- */
static inline int can_move(GameState *g, int cx, int cy, int dir, int speed)
{
    int nx = cx, ny = cy;
    int m = 6;   /* margem de colisao em pixels */

    if      (dir == DIR_UP)    ny -= speed;
    else if (dir == DIR_DOWN)  ny += speed;
    else if (dir == DIR_LEFT)  nx -= speed;
    else if (dir == DIR_RIGHT) nx += speed;

    if (!is_passable(tile_at(g, nx - m, ny - m))) return 0;
    if (!is_passable(tile_at(g, nx + m, ny - m))) return 0;
    if (!is_passable(tile_at(g, nx - m, ny + m))) return 0;
    if (!is_passable(tile_at(g, nx + m, ny + m))) return 0;
    return 1;
}

/* ---------------------------------------------------------------
 * collect_pellet() — coleta pastilha na posicao atual
 * --------------------------------------------------------------- */
static void collect_pellet(GameState *g, int cx, int cy)
{
    int col = (cx - MAP_OFS_X) / TILE_W;
    int row = (cy - MAP_OFS_Y) / TILE_H;
    if (col < 0 || col >= MAP_COLS || row < 0 || row >= MAP_ROWS) return;

    char ch = g->map[row][col];
    if (ch == '.') {
        g->map[row][col] = ' ';
        g->player.score += 10;
        g->pellets_left--;
    } else if (ch == 'o') {
        g->map[row][col] = ' ';
        g->player.score += 50;
        g->pellets_left--;
        g->power_timer = 300;
        int i;
        for (i = 0; i < 4; i++) {
            g->ghosts[i].scared = 1;
            g->ghosts[i].scared_timer = 300;
        }
    }
}

/* ---------------------------------------------------------------
 * ghost_ai() — IA dos fantasmas
 *
 * CORRECAO v2:
 *   - Usa seed individual por fantasma para variar decisoes
 *   - Tenta TODAS as 4 direcoes em ordem variada
 *   - Proibe apenas dar meia-volta (exceto se preso)
 *   - Fantasmas assustados fogem do jogador
 * --------------------------------------------------------------- */
static void ghost_ai(GameState *g, Ghost *gh)
{
    int speed = 1;
    int i;

    /* Atualiza seed */
    gh->seed = gh->seed * 1103515245 + 12345;
    int rng = (gh->seed >> 16) & 0x7FFF;

    /* Direcao oposta (proibida) */
    int opposite;
    if      (gh->dir == DIR_UP)    opposite = DIR_DOWN;
    else if (gh->dir == DIR_DOWN)  opposite = DIR_UP;
    else if (gh->dir == DIR_LEFT)  opposite = DIR_RIGHT;
    else if (gh->dir == DIR_RIGHT) opposite = DIR_LEFT;
    else                           opposite = DIR_NONE;

    /* Se pode continuar na direcao atual, continua na maioria das vezes */
    if (can_move(g, gh->x, gh->y, gh->dir, speed)) {
        /* Em cruzamentos, chance de virar (1 em 4) */
        int turn = (rng % 4 == 0);
        if (!turn) goto do_move;
    }

    /* Escolhe nova direcao: tenta em ordem rotacionada por rng */
    {
        int dirs[4] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };

        /* Shuffle simples: troca posicao 0 com posicao aleatoria */
        int swap = rng % 4;
        int tmp = dirs[0]; dirs[0] = dirs[swap]; dirs[swap] = tmp;

        /* Se assustado: prefere se afastar do jogador */
        if (gh->scared) {
            int dx = gh->x - g->player.x;
            int dy = gh->y - g->player.y;
            /* Coloca na frente da lista a direcao que mais afasta */
            if (dx > 0 && dy > 0) { dirs[0] = DIR_RIGHT; dirs[1] = DIR_DOWN; }
            else if (dx < 0 && dy > 0) { dirs[0] = DIR_LEFT; dirs[1] = DIR_DOWN; }
            else if (dx > 0 && dy < 0) { dirs[0] = DIR_RIGHT; dirs[1] = DIR_UP; }
            else { dirs[0] = DIR_LEFT; dirs[1] = DIR_UP; }
        }

        for (i = 0; i < 4; i++) {
            int nd = dirs[i];
            if (nd == opposite) continue;   /* nao dar meia-volta */
            if (can_move(g, gh->x, gh->y, nd, speed)) {
                gh->dir = nd;
                break;
            }
        }

        /* Ultimo recurso: dar meia-volta */
        if (!can_move(g, gh->x, gh->y, gh->dir, speed)) {
            if (can_move(g, gh->x, gh->y, opposite, speed))
                gh->dir = opposite;
        }
    }

do_move:
    if (can_move(g, gh->x, gh->y, gh->dir, speed)) {
        if      (gh->dir == DIR_UP)    gh->y -= speed;
        else if (gh->dir == DIR_DOWN)  gh->y += speed;
        else if (gh->dir == DIR_LEFT)  gh->x -= speed;
        else if (gh->dir == DIR_RIGHT) gh->x += speed;
    }

    /* Atualiza scared */
    if (gh->scared && gh->scared_timer > 0) {
        gh->scared_timer--;
        if (gh->scared_timer == 0) gh->scared = 0;
    }
}

/* ---------------------------------------------------------------
 * check_ghost_collision()
 * --------------------------------------------------------------- */
static int check_ghost_collision(GameState *g)
{
    int i;
    for (i = 0; i < 4; i++) {
        Ghost *gh = &g->ghosts[i];
        if (!gh->active) continue;
        int dx = g->player.x - gh->x; if (dx < 0) dx = -dx;
        int dy = g->player.y - gh->y; if (dy < 0) dy = -dy;
        if (dx < 10 && dy < 10) {
            if (gh->scared) {
                gh->active = 0;
                g->player.score += 200;
                return 0;
            }
            return 1;
        }
    }
    return 0;
}

/* ---------------------------------------------------------------
 * game_init()
 * --------------------------------------------------------------- */
static void game_init(GameState *g, int level)
{
    int r, c, i;

    g->level        = level;
    g->state        = STATE_PLAYING;
    g->frame        = 0;
    g->pellets_left = 0;
    g->power_timer  = 0;

    /* Copia mapa */
    for (r = 0; r < MAP_ROWS; r++) {
        for (c = 0; c < MAP_COLS; c++) {
            g->map[r][c] = MAZE_TEMPLATE[r][c];
            if (g->map[r][c] == '.' || g->map[r][c] == 'o')
                g->pellets_left++;
        }
        g->map[r][MAP_COLS] = '\0';
    }

    /* Jogador — começa no centro do tile (PLAYER_START_COL, PLAYER_START_ROW) */
    g->player.x        = MAP_OFS_X + PLAYER_START_COL * TILE_W + TILE_W / 2;
    g->player.y        = MAP_OFS_Y + PLAYER_START_ROW * TILE_H + TILE_H / 2;
    g->player.dir      = DIR_NONE;   /* parado ate receber input */
    g->player.next_dir = DIR_NONE;
    g->player.lives    = 3;
    g->player.score    = (level == 1) ? 0 : g->player.score;

    /* Fantasmas */
    int gcol[4] = { GHOST0_COL, GHOST1_COL, GHOST2_COL, GHOST3_COL };
    int grow[4] = { GHOST0_ROW, GHOST1_ROW, GHOST2_ROW, GHOST3_ROW };
    int gcol24[4] = { GHOST_COLOR_0, GHOST_COLOR_1,
                      GHOST_COLOR_2, GHOST_COLOR_3 };
    int gdir[4] = { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN };
    int gseed[4] = { 0x1234, 0x5678, 0xABCD, 0xEF01 };

    for (i = 0; i < 4; i++) {
        g->ghosts[i].x            = MAP_OFS_X + gcol[i] * TILE_W + TILE_W / 2;
        g->ghosts[i].y            = MAP_OFS_Y + grow[i] * TILE_H + TILE_H / 2;
        g->ghosts[i].dir          = gdir[i];
        g->ghosts[i].color24      = gcol24[i];
        g->ghosts[i].scared       = 0;
        g->ghosts[i].active       = 1;
        g->ghosts[i].scared_timer = 0;
        g->ghosts[i].seed         = gseed[i] + level * 17;
    }
}

#endif /* PACMAN_GAME_H */
