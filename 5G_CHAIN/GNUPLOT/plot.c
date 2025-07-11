#include "plot.h"
#include <math.h>
#include <stdio.h>

#define POPEN _popen
#define PCLOSE _pclose
#define update_rate 5
#define MARGIN_FACTOR 0.1  // Margen adicional del 10% alrededor de los datos

void plot_init(PlotContext *ctx) {
    // Inicializar pipes para TX
    ctx->pipe_bits_tx = POPEN("gnuplot -persistent", "w");
    ctx->pipe_const_tx = POPEN("gnuplot -persistent", "w");
    ctx->pipe_fft_tx = POPEN("gnuplot -persistent", "w");

    // Inicializar pipes para RX
    ctx->pipe_bits_rx = POPEN("gnuplot -persistent", "w");
    ctx->pipe_const_rx = POPEN("gnuplot -persistent", "w");
    ctx->pipe_fft_rx = POPEN("gnuplot -persistent", "w");

    ctx->frame_count = 0;

    // Configuración inicial mínima (los rangos se ajustarán dinámicamente)
    fprintf(ctx->pipe_bits_tx, "set title 'TX Bits vs Time'\nset grid\n");
    fprintf(ctx->pipe_const_tx, "set title 'TX Constellation'\nset grid\n");
    fprintf(ctx->pipe_fft_tx, "set title 'TX FFT Spectrum'\nset grid\n");

    fprintf(ctx->pipe_bits_rx, "set title 'RX Bits vs Time'\nset grid\n");
    fprintf(ctx->pipe_const_rx, "set title 'RX Constellation'\nset grid\n");
    fprintf(ctx->pipe_fft_rx, "set title 'RX FFT Spectrum'\nset grid\n");
}

void plot_update_bits(PlotContext *ctx, PlotDirection dir, const uint8_t *bits, uint32_t num_bits, const char *title) {
    FILE *pipe = (dir == TX) ? ctx->pipe_bits_tx : ctx->pipe_bits_rx;

    if (ctx->frame_count % update_rate == 0) {
        // Calcular rangos dinámicos para bits (siempre entre 0 y 1)
        float y_min = -0.1;
        float y_max = 1.1;
        float x_min = 0;
        float x_max = num_bits > 0 ? num_bits - 1 : 0;

        // Aplicar márgenes adicionales
        float x_margin = (x_max - x_min) * MARGIN_FACTOR;
        x_min = x_min - x_margin;
        x_max = x_max + x_margin;

        fprintf(pipe, "set title '%s'\n", title);
        fprintf(pipe, "set xlabel 'Bits index'\n");
        fprintf(pipe, "set ylabel 'Bit Value'\n");
        fprintf(pipe, "set xrange [%f:%f]\n", x_min, x_max);
        fprintf(pipe, "set yrange [%f:%f]\n", y_min, y_max);
        fprintf(pipe, "plot '-' with steps title 'Bits'\n");

        for (uint32_t i = 0; i < num_bits; i++) {
            fprintf(pipe, "%u %u\n", i, bits[i]);
        }
        fprintf(pipe, "e\n");
        fflush(pipe);
    }
    ctx->frame_count++;
}

void plot_update_constellation(PlotContext *ctx, PlotDirection dir, const float complex *symbols, uint32_t num_symbols, const char *title) {
    FILE *pipe = (dir == TX) ? ctx->pipe_const_tx : ctx->pipe_const_rx;

    if (ctx->frame_count % update_rate == 0 && num_symbols > 0) {
        // Calcular rangos dinámicos para la constelación
        float x_min = crealf(symbols[0]);
        float x_max = crealf(symbols[0]);
        float y_min = cimagf(symbols[0]);
        float y_max = cimagf(symbols[0]);

        for (uint32_t i = 1; i < num_symbols; i++) {
            float real = crealf(symbols[i]);
            float imag = cimagf(symbols[i]);

            if (real < x_min) x_min = real;
            if (real > x_max) x_max = real;
            if (imag < y_min) y_min = imag;
            if (imag > y_max) y_max = imag;
        }

        // Aplicar márgenes adicionales
        float x_margin = (x_max - x_min) * MARGIN_FACTOR;
        float y_margin = (y_max - y_min) * MARGIN_FACTOR;

        x_min = x_min - x_margin;
        x_max = x_max + x_margin;
        y_min = y_min - y_margin;
        y_max = y_max + y_margin;

        // Asegurar que el rango sea simétrico para mejor visualización
        float max_abs = fmaxf(fmaxf(fabsf(x_min), fabsf(x_max)), fmaxf(fabsf(y_min), fabsf(y_max)));
        x_min = -max_abs;
        x_max = max_abs;
        y_min = -max_abs;
        y_max = max_abs;

        fprintf(pipe, "set title '%s'\n", title);
        fprintf(pipe, "set xlabel 'InPhase'\n");
        fprintf(pipe, "set ylabel 'Quadrature'\n");
        fprintf(pipe, "set xrange [%f:%f]\n", x_min, x_max);
        fprintf(pipe, "set yrange [%f:%f]\n", y_min, y_max);
        fprintf(pipe, "plot '-' with points pt 7 ps 0.5 title 'Symbols'\n");

        for (uint32_t i = 0; i < num_symbols; i++) {
            fprintf(pipe, "%f %f\n", crealf(symbols[i]), cimagf(symbols[i]));
        }
        fprintf(pipe, "e\n");
        fflush(pipe);
    }
    ctx->frame_count++;
}

void plot_update_fft(PlotContext *ctx, PlotDirection dir, const float complex *ofdm_signal, uint32_t fft_size, double sample_rate, const char *title) {
    FILE *pipe = (dir == TX) ? ctx->pipe_fft_tx : ctx->pipe_fft_rx;

    if (ctx->frame_count % update_rate == 0 && fft_size > 0) {
        // Calcular rangos dinámicos para FFT
        double freq_resolution = sample_rate / fft_size;
        double min_freq = -sample_rate/2;
        double max_freq = sample_rate/2;

        float min_magnitude = cabsf(ofdm_signal[0]);
        float max_magnitude = cabsf(ofdm_signal[0]);

        for (uint32_t i = 1; i < fft_size; i++) {
            float mag = cabsf(ofdm_signal[i]);
            if (mag < min_magnitude) min_magnitude = mag;
            if (mag > max_magnitude) max_magnitude = mag;
        }

        // Aplicar márgenes adicionales
        float y_margin = (max_magnitude - min_magnitude) * MARGIN_FACTOR;
        min_magnitude = fmaxf(0, min_magnitude - y_margin);  // No puede ser negativo
        max_magnitude = max_magnitude + y_margin;

        fprintf(pipe, "set title '%s'\n", title);
        fprintf(pipe, "set xrange [%f:%f]\n", min_freq, max_freq);
        fprintf(pipe, "set yrange [%f:%f]\n", min_magnitude, max_magnitude);
        fprintf(pipe, "plot '-' with lines title 'FFT'\n");

        for (uint32_t i = 0; i < fft_size; i++) {
            double freq = (i < fft_size / 2) ? i * freq_resolution : (i - fft_size) * freq_resolution;
            fprintf(pipe, "%f %f\n", freq, cabsf(ofdm_signal[i]));
        }
        fprintf(pipe, "e\n");
        fflush(pipe);
    }
    ctx->frame_count++;
}

void plot_close(PlotContext *ctx) {
    // Cerrar pipes de TX
    fprintf(ctx->pipe_bits_tx, "exit\n");
    fprintf(ctx->pipe_const_tx, "exit\n");
    fprintf(ctx->pipe_fft_tx, "exit\n");
    PCLOSE(ctx->pipe_bits_tx);
    PCLOSE(ctx->pipe_const_tx);
    PCLOSE(ctx->pipe_fft_tx);

    // Cerrar pipes de RX
    fprintf(ctx->pipe_bits_rx, "exit\n");
    fprintf(ctx->pipe_const_rx, "exit\n");
    fprintf(ctx->pipe_fft_rx, "exit\n");
    PCLOSE(ctx->pipe_bits_rx);
    PCLOSE(ctx->pipe_const_rx);
    PCLOSE(ctx->pipe_fft_rx);
}