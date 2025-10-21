#ifndef GAM_MOD_H
#define GAM_MOD_H

#include "../../MODULES/Common.h"


// CONSTELACIÓN
bool init_golden_modulation(Constellation constellation[C_POINTS]);

int calculate_min_distance_hard(float complex symbol, Constellation constellation[C_POINTS]);


// MODULACIÓN Y DEMODULACIÓN
bool golden_modulation_hard(const int interleaved_bits[TOTAL_BITS_REPEATED], float complex symbols[TOTAL_SYMBOLS]);

bool golden_demodulation_hard(float complex symbols[TOTAL_SYMBOLS], Constellation constellation[C_POINTS],
                              int interleaved_bits[TOTAL_BITS_REPEATED]);


#endif //GAM_MOD_H
