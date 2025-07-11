//LIBRERÍAS
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <complex.h>

//HEADERS
#include "5G_DATASOURCE/5G_DATASOURCE_functions.h"
#include "5G_MOD_QAM/5G_MOD_QAM_functions.h"
#include "5G_MOD_GOLDEN/5G_MOD_GAM_functions.h"
#include "5G_CHANNEL_DIST_NOISE/5G_CHANNEL_DIST_NOISE_functions.h"
#include "GNUPLOT/plot.h"

//PARAMETERS
#define NUM_BITS 256
#define SNR_dB 0.0  // Relaci�n se�al-ruido en dB

int main(void)
{

    // Inicializar semilla aleatoria
    srand(time(NULL));

    PlotContext ctx;
    plot_init(&ctx);
    ctx.update_rate = 100;

    for (ctx.frame_count = 0; ctx.frame_count < 10000; ctx.frame_count++)
    {
        // 1. Generar bits aleatorios
        int* tx_bits = malloc(NUM_BITS * sizeof(int));
        generate_random_bits(tx_bits, NUM_BITS);
        //plot_update_bits(&ctx, TX, tx_bits, NUM_BITS, "TX Bits en tiempo real");

        // 2. Modulacion
        int BPS = 4;
        int NUM_SYMBOLS = NUM_BITS / BPS;
        float complex* symbols = malloc(NUM_SYMBOLS * sizeof(float complex));
        modulate_GAM(tx_bits, symbols, NUM_BITS, BPS);
        plot_update_constellation(&ctx, TX, symbols, NUM_SYMBOLS, "TX Constellation");

        // 3. Calcular potencia media y pasar por canal AWGN
        float avg_power = calc_avg_power(symbols, NUM_SYMBOLS);
        add_awgn_noise(symbols, NUM_SYMBOLS, SNR_dB, avg_power);
        //plot_update_constellation(&ctx, RX, symbols, NUM_SYMBOLS, "RX Constellation");

        // 4. Demodulacion
        int* rx_bits = malloc(NUM_BITS * sizeof(int));
        demodulate_GAM(symbols, rx_bits, NUM_SYMBOLS, BPS);

        // 5. Calcular BER
        float ber = calculate_ber(tx_bits, rx_bits, NUM_BITS);
        //if (ctx.frame_count % ctx.update_rate == 0) { printf("BER para SNR_dB = %.1f BER: %.6f\n", SNR_dB, ber);}
    }
    plot_close(&ctx);
    return 0;
}