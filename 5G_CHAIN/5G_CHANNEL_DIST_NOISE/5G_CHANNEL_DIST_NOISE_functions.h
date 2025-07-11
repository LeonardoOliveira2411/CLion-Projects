#ifndef INC_5G_CHANNEL_DIST_NOISE_FUNCTIONS_H
#define INC_5G_CHANNEL_DIST_NOISE_FUNCTIONS_H

#include <complex.h>

// Canal AWGN
void add_awgn_noise(float complex *symbols, int length, float snr_db, float avg_power);
float calc_avg_power(float complex *symbols, int numsymb);

#endif //INC_5G_CHANNEL_DIST_NOISE_FUNCTIONS_H
