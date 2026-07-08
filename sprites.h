/*
 * sprites.h — Sprites do Pac-Man UFSCar
 *
 * Projeto: Pac-Man DE10-Standard — UFSCar Sistemas Embarcados
 *
 * Usa video_box() e video_circle() do video_hal — identicos ao oficial Intel.
 * Por ora: Pac-Man = circulo amarelo com boca (sera substituido pelo professor).
 * Fantasma = pao de queijo com nutella.
 */

#ifndef SPRITES_H
#define SPRITES_H

#include "video_hal.h"
#include "address_map_arm.h"

#define SPRITE_R  10   /* raio dos sprites em pixels */

/* ---------------------------------------------------------------
 * draw_pacman() — circulo amarelo com boca aberta
 * dir: DIR_UP=1 DIR_DOWN=2 DIR_LEFT=3 DIR_RIGHT=4 DIR_NONE=0
 * --------------------------------------------------------------- */
void draw_pacman(int cx, int cy, int dir)
{
    short c_yellow = COLOR(C24_YELLOW);
    short c_black  = COLOR(C24_BLACK);
    int r = SPRITE_R;
    int x, y, ax, ay;

    for (y = -r; y <= r; y++) {
        for (x = -r; x <= r; x++) {
            if (x*x + y*y > r*r) continue;

            /* Boca: fatia triangular de ~45 graus na direcao atual */
            int bx = x, by = y;
            /* Rotaciona para que a boca aponte na direcao certa */
            int rx, ry;
            if (dir == DIR_UP)         { rx =  y; ry = -x; }
            else if (dir == DIR_DOWN)  { rx = -y; ry =  x; }
            else if (dir == DIR_LEFT)  { rx = -x; ry = -y; }
            else                       { rx =  x; ry =  y; } /* RIGHT ou NONE */

            ax = rx < 0 ? -rx : rx;
            ay = ry < 0 ? -ry : ry;  /* abs de y rotacionado */
            (void)bx; (void)by;

            /* Boca aberta: regiao onde rx > 0 e |ry| < rx */
            if (rx > 2 && ay < rx) continue;

            video_pixel(cx + x, cy + y, c_yellow);
        }
    }

    /* Olho: pequeno ponto escuro */
    if (dir == DIR_LEFT || dir == DIR_NONE)
        video_box(cx - 3, cy - r/2 - 1, cx - 1, cy - r/2 + 1, c_black);
    else if (dir == DIR_RIGHT)
        video_box(cx + 1, cy - r/2 - 1, cx + 3, cy - r/2 + 1, c_black);
    else if (dir == DIR_UP)
        video_box(cx + r/2 - 1, cy - 3, cx + r/2 + 1, cy - 1, c_black);
    else
        video_box(cx + r/2 - 1, cy + 1, cx + r/2 + 1, cy + 3, c_black);
}

/* ---------------------------------------------------------------
 * draw_ghost() — Pao de Queijo com Nutella
 * --------------------------------------------------------------- */
void draw_ghost(int cx, int cy, int r, int color24)
{
    short c_body  = COLOR(color24);
    short c_dark  = COLOR(0x3A1A00);
    short c_white = COLOR(C24_WHITE);
    short c_blue  = COLOR(C24_BLUE);
    short c_black = COLOR(C24_BLACK);
    int x, y;

    /* Semicirculo superior */
    for (y = -r; y <= 0; y++)
        for (x = -r; x <= r; x++)
            if (x*x + y*y <= r*r)
                video_pixel(cx+x, cy+y, c_body);

    /* Corpo retangular */
    video_box(cx-r, cy, cx+r, cy+r, c_body);

    /* Nutella no topo */
    for (y = -r; y <= -r+r/3; y++)
        for (x = -r+1; x <= r-1; x++)
            if (x*x + y*y <= (r-1)*(r-1))
                video_pixel(cx+x, cy+y, c_dark);

    /* Pingos de nutella */
    video_box(cx-r/3-1, cy-r/2, cx-r/3+1, cy,       c_dark);
    video_box(cx+r/3-1, cy-r/3, cx+r/3+1, cy+r/4,   c_dark);

    /* Saias */
    int sw = (2*r)/3;
    video_box(cx-r,      cy+r-3, cx-r+sw-1,     cy+r, c_black);
    video_box(cx-r+sw,   cy+r-3, cx-r+2*sw-1,   cy+r, c_black);

    /* Olhos */
    int ey = cy - r/2;
    video_box(cx-r/2-2, ey,   cx-r/2+2, ey+4, c_white);
    video_box(cx+r/2-2, ey,   cx+r/2+2, ey+4, c_white);
    video_box(cx-r/2-1, ey+1, cx-r/2+1, ey+3, c_blue);
    video_box(cx+r/2-1, ey+1, cx+r/2+1, ey+3, c_blue);
}

/* ---------------------------------------------------------------
 * draw_ghost_scared() — fantasma assustado (azul / piscando branco)
 * --------------------------------------------------------------- */
void draw_ghost_scared(int cx, int cy, int r, int blink)
{
    draw_ghost(cx, cy, r, blink ? 0xFFFFFF : 0x0000CC);
}

/* ---------------------------------------------------------------
 * draw_score() — placar no character buffer
 * --------------------------------------------------------------- */
void draw_score(int score, int lives, int level)
{
    /* Linha 0 do char buffer: "SCORE:XXXXX  LIVES:X  LVL:X  PAC-MAN UFSCar" */
    volatile char *cb = (volatile char *) FPGA_CHAR_BASE;
    int i;

    /* Limpa linha 0 */
    for (i = 0; i < 80; i++) cb[i] = ' ';

    /* SCORE */
    cb[1]='S'; cb[2]='C'; cb[3]='O'; cb[4]='R'; cb[5]='E'; cb[6]=':';
    int s = score, pos = 12;
    if (s == 0) { cb[7]='0'; }
    else {
        char tmp[8]; int ti=0;
        while (s>0 && ti<7) { tmp[ti++]='0'+(s%10); s/=10; }
        int d; for (d=ti-1; d>=0; d--) cb[7+ti-1-d] = tmp[d];
    }

    /* LIVES */
    cb[15]='V'; cb[16]='I'; cb[17]='D'; cb[18]='A'; cb[19]='S'; cb[20]=':';
    cb[21] = '0' + (lives > 9 ? 9 : lives);

    /* LEVEL */
    cb[24]='F'; cb[25]='A'; cb[26]='S'; cb[27]='E'; cb[28]=':';
    cb[29] = '0' + (level > 9 ? 9 : level);

    /* Titulo */
    volatile char *t = (volatile char *)(FPGA_CHAR_BASE + 55);
    t[0]='P'; t[1]='A'; t[2]='C'; t[3]='-'; t[4]='M'; t[5]='A';
    t[6]='N'; t[7]=' '; t[8]='U'; t[9]='F'; t[10]='S'; t[11]='C';
    t[12]='a'; t[13]='r';
}

#endif /* SPRITES_H */
