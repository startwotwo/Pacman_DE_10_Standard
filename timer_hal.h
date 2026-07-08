/*
 * timer_hal.h — Timer para DE10-Standard ARM Cortex-A9
 *
 * Projeto: Pac-Man DE10-Standard — UFSCar Sistemas Embarcados
 *
 * Usa o ARM A9 Private Timer (MPCORE_PRIV_TIMER = 0xFFFEC600).
 * Clock: 200 MHz (metade do ARM A9 @ 800 MHz / prescaler).
 *
 * Registradores (offset em bytes):
 *   +0  Load       — valor de recarga
 *   +4  Counter    — contador atual (decrementa)
 *   +8  Control    — bit0=Enable, bit1=AutoReload, bit2=IrqEnable
 *   +12 IntStatus  — bit0=Event (escreva 1 para limpar)
 */

#ifndef TIMER_HAL_H
#define TIMER_HAL_H

#include "address_map_arm.h"

#define PRIV_TIMER_LOAD    ((volatile int *)(MPCORE_PRIV_TIMER + 0x0))
#define PRIV_TIMER_COUNTER ((volatile int *)(MPCORE_PRIV_TIMER + 0x4))
#define PRIV_TIMER_CTRL    ((volatile int *)(MPCORE_PRIV_TIMER + 0x8))
#define PRIV_TIMER_ISR     ((volatile int *)(MPCORE_PRIV_TIMER + 0xC))

/* Clock do private timer: ARM_CLK / 2 = 400 MHz */
#define PRIV_TIMER_CLK_HZ  400000000UL

/* ---------------------------------------------------------------
 * Estrutura de frame timer (igual ao timer_hal.h anterior)
 * --------------------------------------------------------------- */
typedef struct {
    unsigned long period_ticks;
    unsigned long next_tick;
} FrameTimer;

/* ---------------------------------------------------------------
 * timer_init() — inicia o private timer em modo continuo
 * --------------------------------------------------------------- */
static inline void timer_init(void)
{
    *PRIV_TIMER_CTRL = 0;                   /* para o timer */
    *PRIV_TIMER_LOAD = 0xFFFFFFFF;          /* periodo maximo */
    *PRIV_TIMER_CTRL = 0x3;                 /* Enable + AutoReload */
}

/* ---------------------------------------------------------------
 * timer_get_ticks() — retorna contagem atual (decrescente)
 * Converte para crescente invertendo
 * --------------------------------------------------------------- */
static inline unsigned long timer_get_ticks(void)
{
    return 0xFFFFFFFFUL - (unsigned long)(*PRIV_TIMER_COUNTER);
}

/* ---------------------------------------------------------------
 * timer_get_us() — retorna microsegundos desde o inicio
 * --------------------------------------------------------------- */
static inline unsigned long timer_get_us(void)
{
    return timer_get_ticks() / (PRIV_TIMER_CLK_HZ / 1000000UL);
}

/* ---------------------------------------------------------------
 * timer_delay_ms() — busy-wait por N milissegundos
 * --------------------------------------------------------------- */
static inline void timer_delay_ms(unsigned int ms)
{
    unsigned long start = timer_get_us();
    unsigned long wait  = (unsigned long)ms * 1000UL;
    while ((timer_get_us() - start) < wait) { /* busy wait */ }
}

/* ---------------------------------------------------------------
 * FrameTimer — mantem loop a N fps
 * --------------------------------------------------------------- */
static inline void frame_timer_init(FrameTimer *ft, int fps)
{
    timer_init();
    ft->period_ticks = PRIV_TIMER_CLK_HZ / (unsigned long)fps;
    ft->next_tick    = timer_get_ticks() + ft->period_ticks;
}

static inline void frame_timer_wait(FrameTimer *ft)
{
    while (timer_get_ticks() < ft->next_tick) { /* busy wait */ }
    ft->next_tick += ft->period_ticks;
}

#endif /* TIMER_HAL_H */
