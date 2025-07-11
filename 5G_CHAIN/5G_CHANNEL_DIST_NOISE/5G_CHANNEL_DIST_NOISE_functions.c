#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

// Canal AWGN
void add_awgn_noise(float complex *symbols, int numsymb, float snr_db, float avg_power)
{
    float snr_linear = pow(10.0, snr_db / 10.0);
    float noise_power = avg_power / snr_linear;  // Asumiendo potencia de señal unitaria

    for (int i = 0; i < numsymb; i++)
    {
        // Generar ruido gaussiano complejo
        float u1 = (float)rand() / RAND_MAX;
        float u2 = (float)rand() / RAND_MAX;
        while (u1 <= 1e-12) u1 = (float)rand() / RAND_MAX;

        float z0 = sqrt(-2.0 * log(u1)) * cos(2.0 * M_PI * u2);
        float z1 = sqrt(-2.0 * log(u1)) * sin(2.0 * M_PI * u2);

        symbols[i] += (z0 + z1 * I) * sqrt(noise_power / 2.0);
    }
}

//Calcular Potencia Media Todas Modulaciones
float calc_avg_power(float complex *symbols, int numsymb)
{
    float tx_total_power = 0.0;
    for (int i = 0; i < numsymb; i++)
    {
        tx_total_power = cabsf(symbols[i]);
    }
    float avg_power = tx_total_power/numsymb;

    return avg_power;
}