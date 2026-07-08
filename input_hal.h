/*
 * input_hal.h — Leitura de botoes e switches DE10-Standard ARM
 *
 * Projeto: Pac-Man DE10-Standard — UFSCar Sistemas Embarcados
 *
 * KEY[1:0]  — 0xFF200050 — ativo-baixo (0 = pressionado)
 * SW[9:0]   — 0xFF200040 — ativo-alto  (1 = ligado)
 *
 * Mapeamento APENAS com botoes (KEYs) para movimento:
 *   KEY0 → Cima
 *   KEY1 → Baixo
 *   KEY2 → Esquerda  (se disponivel no seu hardware)
 *   KEY3 → Direita   (se disponivel no seu hardware)
 *
 * Como a DE10-Standard tem apenas 4 KEYs:
 *   KEY0 = Cima
 *   KEY1 = Baixo
 *   KEY2 = Esquerda
 *   KEY3 = Direita
 *
 * SW0 = Start / iniciar jogo (levante para iniciar)
 * Nenhum SW e usado para movimento
 */

#ifndef INPUT_HAL_H
#define INPUT_HAL_H

#include "address_map_arm.h"

#define DIR_NONE    0
#define DIR_UP      1
#define DIR_DOWN    2
#define DIR_LEFT    3
#define DIR_RIGHT   4

/*
 * input_get_direction()
 * Le os 4 botoes KEY[3:0] — todos ativo-baixo (0 = pressionado).
 * Retorna a direcao do botao pressionado, ou DIR_NONE.
 * Prioridade: KEY0 > KEY1 > KEY2 > KEY3
 */
static inline int input_get_direction(void)
{
    volatile int *key = (volatile int *) KEY_BASE;
    int k = *key & 0xF;   /* apenas bits 3:0 */

    if (!(k & 0x1)) return DIR_UP;      /* KEY0 pressionado */
    if (!(k & 0x2)) return DIR_DOWN;    /* KEY1 pressionado */
    if (!(k & 0x4)) return DIR_LEFT;    /* KEY2 pressionado */
    if (!(k & 0x8)) return DIR_RIGHT;   /* KEY3 pressionado */

    return DIR_NONE;
}

/*
 * input_start()
 * Retorna 1 se SW0 estiver levantado (iniciar jogo).
 * SW e ativo-alto: 1 = ligado.
 */
static inline int input_start(void)
{
    volatile int *sw = (volatile int *) SW_BASE;
    return (*sw & 0x1) ? 1 : 0;
}

/*
 * input_start_edge()
 * Detecta borda de subida do SW0 (evita iniciar multiplas vezes).
 * Chame a cada frame; retorna 1 apenas no frame em que SW0 foi ligado.
 */
static inline int input_start_edge(void)
{
    static int prev_sw = 0;
    volatile int *sw = (volatile int *) SW_BASE;
    int cur = (*sw & 0x1) ? 1 : 0;
    int edge = (cur && !prev_sw) ? 1 : 0;
    prev_sw = cur;
    return edge;
}

#endif /* INPUT_HAL_H */
