#ifndef GAM_PRB_H
#define GAM_PRB_H

#include "../../MODULES/Common.h"


bool init_prb_grid(PRB_Grid *grid);

bool map_data_to_prb(const float complex data_symbols[TOTAL_SYMBOLS], PRB_Grid *grid);

bool extract_data_from_prb(const PRB_Grid *grid, float complex data_symbols[TOTAL_SYMBOLS]);

bool generate_prb_ofdm_symbols(const PRB_Grid *grid, float complex ofdm_symbols[PRB_SYMBOLS][N_FFT]);

bool process_received_prb_ofdm(const float complex received_symbols[PRB_SYMBOLS][N_FFT], PRB_Grid *grid);


#endif //GAM_PRB_H
