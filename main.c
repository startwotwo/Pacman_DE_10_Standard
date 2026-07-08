/*
 * main.c — Pac-Man DE10-Standard ARM Cortex-A9
 *
 * Projeto: Pac-Man DE10-Standard — UFSCar Sistemas Embarcados
 *
 * Controles:
 *   KEY0 = Cima   KEY1 = Baixo   KEY2 = Esquerda   KEY3 = Direita
 *   SW0  = Iniciar jogo (levante o switch)
 *
 * Arquivos no projeto (Monitor Program) — apenas .c:
 *   main.c   video_hal.c
 *
 * Arquivos .h na mesma pasta (NAO adicionar ao projeto):
 *   address_map_arm.h  video_hal.h  input_hal.h
 *   timer_hal.h  sprites.h  pacman_game.h
 *
 * Flags: -O1 -std=c99
 */

#include "address_map_arm.h"
#include "video_hal.h"
#include "input_hal.h"
#include "timer_hal.h"
#include "sprites.h"
#include "pacman_game.h"

/* ---------------------------------------------------------------
 * Estado global
 * --------------------------------------------------------------- */
static GameState game;

/* Posicoes anteriores para dirty-rect (apagar antes de redesenhar) */
static int prev_x[5], prev_y[5];

/* ---------------------------------------------------------------
 * render_tile() — desenha um tile do mapa
 * --------------------------------------------------------------- */
static void render_tile(int col, int row)
{
    int x1 = MAP_OFS_X + col * TILE_W;
    int y1 = MAP_OFS_Y + row * TILE_H;
    int x2 = x1 + TILE_W - 1;
    int y2 = y1 + TILE_H - 1;
    char ch = game.map[row][col];

    if (ch == '#') {
        video_box(x1, y1, x2, y2, COLOR(0x0000BB));
        /* Borda superior e esquerda mais clara */
        video_box(x1, y1, x2, y1 + 1, COLOR(0x4444FF));
        video_box(x1, y1, x1 + 1, y2, COLOR(0x4444FF));
    } else {
        video_box(x1, y1, x2, y2, COLOR(C24_BLACK));
        if (ch == '.') {
            /* Pastilha pequena centrada no tile */
            video_box(x1 + 6, y1 + 6, x1 + 9, y1 + 9, COLOR(C24_WHITE));
        } else if (ch == 'o') {
            /* Power pellet */
            video_circle(x1 + TILE_W/2, y1 + TILE_H/2, 4, COLOR(C24_WHITE));
        }
    }
}

/* ---------------------------------------------------------------
 * render_maze() — desenha o labirinto completo
 * --------------------------------------------------------------- */
static void render_maze(void)
{
    int r, c;
    for (r = 0; r < MAP_ROWS; r++)
        for (c = 0; c < MAP_COLS; c++)
            render_tile(c, r);
}

/* ---------------------------------------------------------------
 * erase_and_restore() — apaga sprite e restaura tiles da area
 * --------------------------------------------------------------- */
static void erase_and_restore(int cx, int cy)
{
    if (cx < 0) return;   /* posicao invalida (inicial) */

    int r = SPRITE_R + 2;
    int x1 = cx - r, y1 = cy - r;
    int x2 = cx + r, y2 = cy + r;

    /* Limpa a area com preto */
    video_box(x1, y1, x2, y2, COLOR(C24_BLACK));

    /* Restaura os tiles que a area cobria */
    int c0 = (x1 - MAP_OFS_X) / TILE_W;
    int c1 = (x2 - MAP_OFS_X) / TILE_W;
    int r0 = (y1 - MAP_OFS_Y) / TILE_H;
    int r1 = (y2 - MAP_OFS_Y) / TILE_H;
    int tr, tc;

    for (tr = r0; tr <= r1; tr++)
        for (tc = c0; tc <= c1; tc++)
            if (tc >= 0 && tc < MAP_COLS && tr >= 0 && tr < MAP_ROWS)
                render_tile(tc, tr);
}

/* ---------------------------------------------------------------
 * render_frame() — apaga posicoes antigas, desenha novas
 * --------------------------------------------------------------- */
static void render_frame(void)
{
    int i;

    /* 1. Apaga posicoes anteriores */
    erase_and_restore(prev_x[0], prev_y[0]);
    for (i = 0; i < 4; i++)
        erase_and_restore(prev_x[i+1], prev_y[i+1]);

    /* 2. Desenha fantasmas */
    for (i = 0; i < 4; i++) {
        Ghost *gh = &game.ghosts[i];
        if (!gh->active) { prev_x[i+1] = -1; prev_y[i+1] = -1; continue; }

        int blink = (game.frame % 20 < 10);
        if (gh->scared)
            draw_ghost_scared(gh->x, gh->y, SPRITE_R, blink);
        else
            draw_ghost(gh->x, gh->y, SPRITE_R, gh->color24);

        prev_x[i+1] = gh->x;
        prev_y[i+1] = gh->y;
    }

    /* 3. Desenha jogador (circulo amarelo com boca por ora) */
    draw_pacman(game.player.x, game.player.y, game.player.dir);
    prev_x[0] = game.player.x;
    prev_y[0] = game.player.y;

    /* 4. Placar */
    draw_score(game.player.score, game.player.lives, game.level);
}

/* ---------------------------------------------------------------
 * update_player() — move o jogador com input dos KEYs
 *
 * CORRECAO: Pac-Man se move continuamente na direcao atual.
 * Quando nenhum botao e pressionado, mantem a direcao atual.
 * Para quando bate na parede.
 * Para completamente apenas se dir == DIR_NONE (inicio).
 * --------------------------------------------------------------- */
static void update_player(int dir_input)
{
    Player *p = &game.player;
    int speed = 2;

    /* Atualiza direcao desejada quando botao pressionado */
    if (dir_input != DIR_NONE)
        p->next_dir = dir_input;

    /* Tenta virar na direcao desejada */
    if (p->next_dir != DIR_NONE && p->next_dir != p->dir) {
        if (can_move(&game, p->x, p->y, p->next_dir, speed))
            p->dir = p->next_dir;
    }

    /* Move continuamente na direcao atual */
    if (p->dir != DIR_NONE) {
        if (can_move(&game, p->x, p->y, p->dir, speed)) {
            if      (p->dir == DIR_UP)    p->y -= speed;
            else if (p->dir == DIR_DOWN)  p->y += speed;
            else if (p->dir == DIR_LEFT)  p->x -= speed;
            else if (p->dir == DIR_RIGHT) p->x += speed;
        }
        /* Se bateu na parede, para e aguarda nova direcao */
        else {
            /* Nao zera p->dir para que quando virar o canto continue */
        }
    }

    /* Coleta pastilha */
    collect_pellet(&game, p->x, p->y);
}

/* ---------------------------------------------------------------
 * reset_positions() — reposiciona jogador e fantasmas apos morte
 * --------------------------------------------------------------- */
static void reset_positions(void)
{
    int i;
    game.player.x    = MAP_OFS_X + PLAYER_START_COL * TILE_W + TILE_W/2;
    game.player.y    = MAP_OFS_Y + PLAYER_START_ROW * TILE_H + TILE_H/2;
    game.player.dir  = DIR_NONE;
    game.player.next_dir = DIR_NONE;

    int gcol[4] = { GHOST0_COL, GHOST1_COL, GHOST2_COL, GHOST3_COL };
    int grow[4] = { GHOST0_ROW, GHOST1_ROW, GHOST2_ROW, GHOST3_ROW };
    int gdir[4] = { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN };

    for (i = 0; i < 4; i++) {
        game.ghosts[i].x      = MAP_OFS_X + gcol[i]*TILE_W + TILE_W/2;
        game.ghosts[i].y      = MAP_OFS_Y + grow[i]*TILE_H + TILE_H/2;
        game.ghosts[i].dir    = gdir[i];
        game.ghosts[i].active = 1;
        game.ghosts[i].scared = 0;
        game.ghosts[i].scared_timer = 0;
    }

    for (i = 0; i < 5; i++) { prev_x[i] = -1; prev_y[i] = -1; }
}

/* ---------------------------------------------------------------
 * tela_attract() — tela de atracao
 * --------------------------------------------------------------- */
static void tela_attract(void)
{
    video_clear(C24_BLACK);
    video_text(28,  3, "=== PAC-MAN ===");
    video_text(25,  4, "UFSCar - Sist. Embarcados");
    video_text(20,  6, "PROFESSOR  vs  PAO DE QUEIJO");

    /* Pac-Man */
    draw_pacman(70, 120, DIR_RIGHT);

    /* 4 fantasmas */
    draw_ghost(150, 120, 16, GHOST_COLOR_0);
    draw_ghost(185, 120, 16, GHOST_COLOR_1);
    draw_ghost(220, 120, 16, GHOST_COLOR_2);
    draw_ghost(255, 120, 16, GHOST_COLOR_3);

    /* Pastilhas decorativas */
    int x;
    for (x = 95; x < 150; x += 14)
        video_circle(x, 120, 3, COLOR(C24_WHITE));

    video_text(23, 44, "CONTROLES:");
    video_text(22, 46, "KEY0=CIMA    KEY1=BAIXO");
    video_text(22, 47, "KEY2=ESQUERDA  KEY3=DIREITA");
    video_text(24, 50, "SW0 = INICIAR O JOGO");
}

/* ---------------------------------------------------------------
 * main()
 * --------------------------------------------------------------- */
int main(void)
{
    volatile int *led = (volatile int *) LED_BASE;
    int i;

    /* Boot: acende todos os LEDs */
    *led = 0x3FF;

    /* Inicializa video */
    video_init();

    /* Inicializa timer */
    FrameTimer ft;
    frame_timer_init(&ft, 60);

    /* Zera posicoes anteriores */
    for (i = 0; i < 5; i++) { prev_x[i] = -1; prev_y[i] = -1; }

    /* Tela de atracao */
    tela_attract();

    /* Aguarda SW0 para iniciar (borda de subida) */
    {
        int blink_frame = 0;
        /* Espera SW0 ir para 0 primeiro (caso ja esteja levantado) */
        while (input_start()) { frame_timer_wait(&ft); }

        while (1) {
            blink_frame++;
            /* Pisca instrucao */
            if ((blink_frame / 30) % 2 == 0)
                video_text(24, 50, "SW0 = INICIAR O JOGO");
            else
                video_text(24, 50, "                    ");

            if (input_start_edge()) break;
            frame_timer_wait(&ft);
        }
    }

    /* Inicializa jogo */
    game_init(&game, 1);
    video_clear(C24_BLACK);
    render_maze();
    draw_score(0, game.player.lives, game.level);

    video_text(33, 28, "READY!");
    timer_delay_ms(2000);
    video_text(33, 28, "      ");

    /* ============================================================
     * Loop principal 60 Hz
     * ============================================================ */
    while (1) {
        game.frame++;

        /* LED: pisca lentamente para mostrar que esta rodando */
        if ((game.frame % 30) == 0)
            *led ^= 0x3FF;

        if (game.state == STATE_PLAYING) {

            /* --- Input --- */
            int dir = input_get_direction();
            update_player(dir);

            /* --- Fantasmas (movem a 30 Hz) --- */
            if (game.frame % 2 == 0) {
                for (i = 0; i < 4; i++)
                    if (game.ghosts[i].active)
                        ghost_ai(&game, &game.ghosts[i]);
            }

            /* --- Colisao --- */
            if (check_ghost_collision(&game)) {
                game.player.lives--;

                /* Apaga sprites */
                erase_and_restore(game.player.x, game.player.y);
                for (i = 0; i < 4; i++)
                    if (game.ghosts[i].active)
                        erase_and_restore(game.ghosts[i].x, game.ghosts[i].y);

                if (game.player.lives <= 0) {
                    game.state = STATE_GAMEOVER;
                    video_text(29, 26, "--- GAME  OVER ---");
                    video_text(26, 28, "Levante SW0 p/ jogar de novo");
                    /* Aguarda SW0 */
                    while (!input_start()) { frame_timer_wait(&ft); }
                    while (input_start())  { frame_timer_wait(&ft); }
                    /* Reinicia */
                    game_init(&game, 1);
                    video_clear(C24_BLACK);
                    render_maze();
                    draw_score(0, game.player.lives, game.level);
                    video_text(33, 28, "READY!");
                    timer_delay_ms(2000);
                    video_text(33, 28, "      ");
                } else {
                    video_text(32, 28, "  MORREU!  ");
                    timer_delay_ms(1500);
                    video_text(32, 28, "           ");
                    reset_positions();
                    render_maze();
                    video_text(33, 28, "READY!");
                    timer_delay_ms(1500);
                    video_text(33, 28, "      ");
                }
            }

            /* --- Vitoria --- */
            if (game.state == STATE_PLAYING && game.pellets_left <= 0) {
                video_text(29, 28, "  FASE COMPLETA!  ");
                timer_delay_ms(3000);
                video_text(29, 28, "                  ");
                int score_save = game.player.score;
                int lives_save = game.player.lives;
                game_init(&game, game.level + 1);
                game.player.score = score_save;
                game.player.lives = lives_save;
                video_clear(C24_BLACK);
                render_maze();
                draw_score(game.player.score, game.player.lives, game.level);
                video_text(33, 28, "READY!");
                timer_delay_ms(2000);
                video_text(33, 28, "      ");
            }

            /* --- Render --- */
            if (game.state == STATE_PLAYING)
                render_frame();
        }

        frame_timer_wait(&ft);
    }

    return 0;
}
