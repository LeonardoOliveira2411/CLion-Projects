#include "Channel.h"

// FUNCIONES CANAL
float complex awgn_noise(float stddev)
{
    float u1, u2;

    do {
        u1 = (float)rand() / RAND_MAX;
        u2 = (float)rand() / RAND_MAX;
    } while (u1 <= 1e-10f);

    float radius = sqrtf(-2.0f * logf(u1));
    float angle = (float)(2.0f * M_PI * u2);

    return radius * cosf(angle) * stddev + I * radius * sinf(angle) * stddev;
}

float calculate_mean_power(const float complex symbols[], int length)
{
    float power = 0;

    for (int i = 0; i < length; i++)
    {
        power += crealf(symbols[i]) * crealf(symbols[i]) +
                 cimagf(symbols[i]) * cimagf(symbols[i]);
    }

    return power / (float)length;
}

bool awgn_channel(float complex in_symbols[], float complex out_symbols[], int length)
{
    float mean_power = calculate_mean_power(in_symbols, length);

    if (mean_power <= 1e-10f) return true;

    float SNR_lin = powf(10.0f, SNR/10.0f);
    float noise_power = mean_power / SNR_lin;
    float stddev = sqrtf(noise_power / 2.0f);

    for (int i = 0; i < length; i++)
    {
        out_symbols[i] = in_symbols[i] + awgn_noise(stddev);
    }

    return false;
}