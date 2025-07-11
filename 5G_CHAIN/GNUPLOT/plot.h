#ifndef PLOT_H
#define PLOT_H

#include <stdint.h>
#include <complex.h>
#include <stdio.h>

typedef enum {
    TX,
    RX
} PlotDirection;

// Estructura para mantener pipes persistentes
typedef struct {
    FILE *pipe_bits_tx;
    FILE *pipe_const_tx;
    FILE *pipe_fft_tx;
    FILE *pipe_bits_rx;
    FILE *pipe_const_rx;
    FILE *pipe_fft_rx;
    int frame_count;
    int update_rate;
} PlotContext;

// Inicializar (abre pipes una sola vez)
void plot_init(PlotContext *ctx);

// Actualizar gráficas (solo datos)
void plot_update_bits(PlotContext *ctx, PlotDirection dir, const uint8_t *bits, uint32_t num_bits, const char *title);
void plot_update_constellation(PlotContext *ctx, PlotDirection dir, const float complex *symbols, uint32_t num_symbols, const char *title);
void plot_update_fft(PlotContext *ctx, PlotDirection dir, const float complex *ofdm_signal, uint32_t fft_size, double sample_rate, const char *title);

// Cerrar pipes al finalizar
void plot_close(PlotContext *ctx);

#endif