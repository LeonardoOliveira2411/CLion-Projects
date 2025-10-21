#ifndef GAM_FRAME_H
#define GAM_FRAME_H

#include "../../MODULES/Common.h"


// Preámbulo y detección
bool generate_preamble_bpsk(float complex preamble[PREAMBLE_LEN]);

float calculate_correlation(const float complex received_signal[], int start_index, int length);

bool detect_frame_start(const float complex received_signal[], int length, int *frame_start_index);


// Formación de trama
bool add_preamble_to_prb_frame(const float complex ofdm_symbols_with_cp[PRB_SYMBOLS][N_FFT + CP_LEN],
                               float complex frame_with_preamble[PREAMBLE_LEN + PRB_SYMBOLS * (N_FFT + CP_LEN)]);

bool extract_ofdm_from_frame(const float complex frame_with_preamble[], int frame_start_index,
                             float complex ofdm_symbols_with_cp[PRB_SYMBOLS][N_FFT + CP_LEN], int total_frame_samples);


// Prefijo cíclico
void add_cyclic_prefix(const float complex ofdm_symbol[N_FFT], float complex ofdm_symbol_with_cp[N_FFT + CP_LEN]);

void remove_cyclic_prefix(const float complex ofdm_symbol_with_cp[N_FFT + CP_LEN], float complex ofdm_symbol[N_FFT]);


#endif //GAM_FRAME_H
