/*
 * video_hal.h — Camada de video para DE10-Standard ARM
 *
 * Projeto: Pac-Man DE10-Standard — UFSCar Sistemas Embarcados
 *
 * Baseado EXATAMENTE no video.c oficial da Intel University Program.
 * Mesmas funcoes video_box() / video_text() / resample_rgb() / get_data_bits().
 * Adicionadas: video_pixel(), video_circle(), video_clear().
 *
 * Resolucao: 320x240 logico → 640x480 VGA (upscale 2x automatico)
 * Pixel buffer: 0xC8000000 (heavyweight HPS→FPGA bridge)
 * Char buffer:  0xC9000000
 */

#ifndef VIDEO_HAL_H
#define VIDEO_HAL_H

#include "address_map_arm.h"

#define SCREEN_W   320
#define SCREEN_H   240

/* Cores RGB24 (use com resample_rgb antes de passar para video_box) */
#define C24_BLACK    0x000000
#define C24_WHITE    0xFFFFFF
#define C24_RED      0xFF0000
#define C24_GREEN    0x00C000
#define C24_BLUE     0x0000FF
#define C24_YELLOW   0xFFFF00
#define C24_CYAN     0x00FFFF
#define C24_MAGENTA  0xFF00FF
#define C24_ORANGE   0xFF8000
#define C24_PINK     0xFF80C0
#define C24_DKBLUE   0x000044   /* fundo do labirinto */

/* Variaveis de estado (definidas em video_hal.c) */
extern int screen_x, screen_y, res_offset, col_offset, data_bits;

/* Inicializacao — chame UMA vez no inicio do main() */
void video_init(void);

/* Limpa a tela inteira com uma cor RGB24 */
void video_clear(int color24);

/* Igual ao oficial — retangulo preenchido com cor ja convertida */
void video_box(int x1, int y1, int x2, int y2, short pixel_color);

/* Texto no character buffer */
void video_text(int x, int y, char *text_ptr);

/* Pixel individual */
void video_pixel(int x, int y, short color);

/* Circulo preenchido */
void video_circle(int cx, int cy, int r, short color);

/* Converte RGB24 → formato do hardware (8 ou 16 bits) */
int resample_rgb(int num_bits, int color);

/* Detecta profundidade de cor do hardware */
int get_data_bits(int mode);

/* Macro de conveniencia: converte RGB24 para o formato correto */
#define COLOR(r24)   ((short)resample_rgb(data_bits, (r24)))

#endif /* VIDEO_HAL_H */
