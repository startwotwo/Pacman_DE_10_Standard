/*
 * video_hal.c — Implementacao da camada de video
 *
 * Projeto: Pac-Man DE10-Standard — UFSCar Sistemas Embarcados
 *
 * video_box() e video_text() sao IDENTICAS ao video.c oficial da Intel.
 * video_pixel() usa o mesmo calculo de endereco do video_box().
 */

#include "address_map_arm.h"
#include "video_hal.h"

/* Variaveis globais (mesmo padrao do video.c oficial) */
int screen_x, screen_y, res_offset, col_offset, data_bits;

/* ---------------------------------------------------------------
 * video_init() — detecta resolucao e profundidade de cor
 * (equivalente ao bloco de inicializacao do main() oficial)
 * --------------------------------------------------------------- */
void video_init(void)
{
    volatile int *video_resolution = (int *)(PIXEL_BUF_CTRL_BASE + 0x8);
    screen_x = *video_resolution & 0xFFFF;
    screen_y = (*video_resolution >> 16) & 0xFFFF;

    volatile int *rgb_status = (int *)(RGB_RESAMPLER_BASE);
    data_bits = get_data_bits(*rgb_status & 0x3F);

    res_offset = (screen_x == 160) ? 1 : 0;
    col_offset = (data_bits == 8)  ? 1 : 0;
}

/* ---------------------------------------------------------------
 * video_clear() — limpa a tela inteira com cor RGB24
 * --------------------------------------------------------------- */
void video_clear(int color24)
{
    video_box(0, 0, SCREEN_W - 1, SCREEN_H - 1,
              (short)resample_rgb(data_bits, color24));
}

/* ---------------------------------------------------------------
 * video_text() — IDENTICA ao video.c oficial
 * --------------------------------------------------------------- */
void video_text(int x, int y, char *text_ptr)
{
    int offset;
    volatile char *character_buffer = (char *) FPGA_CHAR_BASE;
    offset = (y << 7) + x;
    while (*text_ptr) {
        *(character_buffer + offset) = *text_ptr;
        ++text_ptr;
        ++offset;
    }
}

/* ---------------------------------------------------------------
 * video_box() — IDENTICA ao video.c oficial
 * --------------------------------------------------------------- */
void video_box(int x1, int y1, int x2, int y2, short pixel_color)
{
    int pixel_buf_ptr = *(int *) PIXEL_BUF_CTRL_BASE;
    int pixel_ptr, row, col;
    int x_factor = 0x1 << (res_offset + col_offset);
    int y_factor = 0x1 << (res_offset);

    x1 = x1 / x_factor;
    x2 = x2 / x_factor;
    y1 = y1 / y_factor;
    y2 = y2 / y_factor;

    for (row = y1; row <= y2; row++)
        for (col = x1; col <= x2; ++col) {
            pixel_ptr = pixel_buf_ptr
                        + (row << (10 - res_offset - col_offset))
                        + (col << 1);
            *(short *) pixel_ptr = pixel_color;
        }
}

/* ---------------------------------------------------------------
 * video_pixel() — mesmo calculo de endereco do video_box()
 * --------------------------------------------------------------- */
void video_pixel(int x, int y, short color)
{
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;

    int pixel_buf_ptr = *(int *) PIXEL_BUF_CTRL_BASE;
    int x_factor = 0x1 << (res_offset + col_offset);
    int y_factor = 0x1 << res_offset;

    int cx = x / x_factor;
    int cy = y / y_factor;
    int pixel_ptr = pixel_buf_ptr
                    + (cy << (10 - res_offset - col_offset))
                    + (cx << 1);
    *(short *) pixel_ptr = color;
}

/* ---------------------------------------------------------------
 * video_circle() — circulo preenchido via video_pixel()
 * --------------------------------------------------------------- */
void video_circle(int cx, int cy, int r, short color)
{
    int x, y;
    for (y = -r; y <= r; y++)
        for (x = -r; x <= r; x++)
            if (x*x + y*y <= r*r)
                video_pixel(cx + x, cy + y, color);
}

/* ---------------------------------------------------------------
 * resample_rgb() — IDENTICA ao video.c oficial
 * --------------------------------------------------------------- */
int resample_rgb(int num_bits, int color)
{
    if (num_bits == 8) {
        color = (((color >> 16) & 0x000000E0) | ((color >> 11) & 0x0000001C) |
                 ((color >>  6) & 0x00000003));
        color = (color << 8) | color;
    } else if (num_bits == 16) {
        color = (((color >>  8) & 0x0000F800) | ((color >>  5) & 0x000007E0) |
                 ((color >>  3) & 0x0000001F));
    }
    return color;
}

/* ---------------------------------------------------------------
 * get_data_bits() — IDENTICA ao video.c oficial
 * --------------------------------------------------------------- */
int get_data_bits(int mode)
{
    switch (mode) {
        case 0x0:  return 1;
        case 0x7:  return 8;
        case 0x11: return 8;
        case 0x12: return 9;
        case 0x14: return 16;
        case 0x17: return 24;
        case 0x19: return 30;
        case 0x31: return 8;
        case 0x32: return 12;
        case 0x33: return 16;
        case 0x37: return 32;
        case 0x39: return 40;
        default:   return 16;
    }
}
