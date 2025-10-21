#ifndef GAM_CHANNEL_H
#define GAM_CHANNEL_H

#include "../../MODULES/Common.h"


// MODELADO DE CANAL
float complex awgn_noise(float stddev);

float calculate_mean_power(const float complex symbols[], int length);

bool awgn_channel(float complex in_symbols[], float complex out_symbols[], int length);


#endif //GAM_CHANNEL_H
