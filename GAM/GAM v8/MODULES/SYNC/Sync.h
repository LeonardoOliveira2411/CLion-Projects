#ifndef GAM_SYNC_H
#define GAM_SYNC_H

#include "../../MODULES/Common.h"


// INICIALIZAR
bool init_cfo_residual_tracker(CFOResidualTracker *tracker, float alpha);

bool init_cpe_tracker(CPETracker *tracker, float alpha);

bool init_sco_tracker(SCOTracker *tracker, float alpha);


//ESTIMAR
bool estimate_cfo_residual(const float complex received_pilots[PRB_SYMBOLS][PILOTS_PER_SYMBOL], CFOResidualTracker *tracker);

bool estimate_cpe(const float complex received_pilots[PRB_SYMBOLS][PILOTS_PER_SYMBOL], CPETracker *tracker);

bool estimate_sco(const float complex current_pilots[PRB_SYMBOLS][PILOTS_PER_SYMBOL],
                  const float complex previous_pilots[PRB_SYMBOLS][PILOTS_PER_SYMBOL], SCOTracker *tracker);


//CORREGIR
bool apply_cfo_residual_correction(PRB_Grid *grid, CFOResidualTracker *tracker);

bool apply_cpe_correction(PRB_Grid *grid, CPETracker *tracker);

bool apply_sco_compensation(PRB_Grid *grid, SCOTracker *tracker);


#endif //GAM_SYNC_H
