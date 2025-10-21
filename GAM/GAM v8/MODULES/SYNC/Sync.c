#include "Sync.h"

// FUNCIONES SINCRONIZACIÓN CFO
bool init_cfo_residual_tracker(CFOResidualTracker *tracker, float alpha)
{

    if (!tracker || alpha <= 0.0f || alpha > 1.0f)
    {
        printf("Error: Parametros invalidos para CFO tracker\n");
        return true;
    }

    tracker->cfo_estimate = 0.0f;
    tracker->cfo_filtered = 0.0f;
    tracker->alpha = alpha;
    tracker->pilots_used = 0;
    tracker->initialized = false;

    return false;
}

bool estimate_cfo_residual(const float complex received_pilots[PRB_SYMBOLS][PILOTS_PER_SYMBOL], CFOResidualTracker *tracker)
{

    if (!tracker)
    {
        printf("Error: Tracker CFO no inicializado\n");
        return true;
    }

    float complex phase_sum = 0.0f + 0.0f * I;
    int valid_pilots = 0;

    // Estimación promediada sobre todos los símbolos del PRB
    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
        {
            float pilot_power = cabsf(received_pilots[sym][p]);
            float noise_estimate = 0.1f;

            if (pilot_power > noise_estimate)
            {
                float complex phase_diff = received_pilots[sym][p] * conjf(PILOT_VALUE);
                phase_sum += phase_diff;
                valid_pilots++;
            }
        }
    }

    if (valid_pilots < TOTAL_PILOTS / 3)
    {
        printf("Advertencia CFO: Solo %d/%d pilotos validos\n", valid_pilots, TOTAL_PILOTS);
        tracker->pilots_used = valid_pilots;
        return false;
    }

    float avg_angle = cargf(phase_sum);

    if (!tracker->initialized)
    {
        tracker->cfo_filtered = avg_angle;
        tracker->initialized = true;
    }
    else
    {
        tracker->cfo_filtered = tracker->alpha * avg_angle +
                                (1.0f - tracker->alpha) * tracker->cfo_filtered;
    }

    tracker->cfo_estimate = avg_angle;
    tracker->pilots_used = valid_pilots;

    printf("CFO Residual: %.6f rad, Filtrado: %.6f rad (%d pilotos)\n",
           avg_angle, tracker->cfo_filtered, valid_pilots);

    return false;
}

bool apply_cfo_residual_correction(PRB_Grid *grid, CFOResidualTracker *tracker)
{

    if (!tracker || !tracker->initialized)
    {
        printf("Advertencia: CFO tracker no inicializado, saltando correccion\n");
        return false;
    }

    float correction_angle = -tracker->cfo_filtered;
    float complex correction = cosf(correction_angle) + sinf(correction_angle) * I;

    printf("Aplicando correccion CFO residual: %.6f rad\n", correction_angle);

    // Aplicar a todos los datos del PRB
    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        for (int sc = 0; sc < PRB_SUBCARRIERS; sc++)
        {
            grid->data_symbols[sym][sc] = grid->data_symbols[sym][sc] * correction;
        }
        for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
        {
            grid->pilot_symbols[sym][p] = grid->pilot_symbols[sym][p] * correction;
        }
    }

    return false;
}


// FUNCIONES SINCRONIZACIÓN CPE
bool init_cpe_tracker(CPETracker *tracker, float alpha)
{

    if (!tracker || alpha <= 0.0f || alpha > 1.0f)
    {
        printf("Error: Parametros invalidos para CPE tracker\n");
        return true;
    }

    tracker->cpe_estimate = 0.0f;
    tracker->cpe_filtered = 0.0f;
    tracker->alpha = alpha;
    tracker->pilots_used = 0;
    tracker->initialized = false;

    return false;
}

bool estimate_cpe(const float complex received_pilots[PRB_SYMBOLS][PILOTS_PER_SYMBOL], CPETracker *tracker)
{

    if (!tracker)
    {
        printf("Error: CPE tracker no inicializado\n");
        return true;
    }

    float complex phase_sum = 0.0f + 0.0f * I;
    int valid_pilots = 0;

    // Estimación de fase común por símbolo
    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        float complex symbol_phase_sum = 0.0f + 0.0f * I;
        int symbol_valid_pilots = 0;

        for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
        {
            float pilot_power = cabsf(received_pilots[sym][p]);
            if (pilot_power > 0.1f)
            {
                float complex phase_diff = received_pilots[sym][p] * conjf(PILOT_VALUE);
                symbol_phase_sum += phase_diff;
                symbol_valid_pilots++;
            }
        }

        if (symbol_valid_pilots > 0)
        {
            float symbol_angle = cargf(symbol_phase_sum);
            phase_sum += cosf(symbol_angle) + sinf(symbol_angle) * I;
            valid_pilots++;
        }
    }

    if (valid_pilots < PRB_SYMBOLS / 2)
    {  // Mínimo la mitad de los símbolos
        printf("Advertencia CPE: Solo %d/%d simbolos con pilotos validos\n", valid_pilots, PRB_SYMBOLS);
        tracker->pilots_used = valid_pilots;
        return false;
    }

    float avg_angle = cargf(phase_sum);

    if (!tracker->initialized)
    {
        tracker->cpe_filtered = avg_angle;
        tracker->initialized = true;
    }
    else
    {
        tracker->cpe_filtered = tracker->alpha * avg_angle +
                                (1.0f - tracker->alpha) * tracker->cpe_filtered;
    }

    tracker->cpe_estimate = avg_angle;
    tracker->pilots_used = valid_pilots;

    printf("CPE: %.6f rad, Filtrado: %.6f rad (%d simbolos validos)\n",
           avg_angle, tracker->cpe_filtered, valid_pilots);

    return false;
}

bool apply_cpe_correction(PRB_Grid *grid, CPETracker *tracker)
{

    if (!tracker || !tracker->initialized)
    {
        printf("Advertencia: CPE tracker no inicializado, saltando correccion\n");
        return false;
    }

    float correction_angle = -tracker->cpe_filtered;
    float complex correction = cosf(correction_angle) + sinf(correction_angle) * I;

    printf("Aplicando correccion CPE: %.6f rad\n", correction_angle);

    // Aplicar a todos los datos del PRB
    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        for (int sc = 0; sc < PRB_SUBCARRIERS; sc++)
        {
            grid->data_symbols[sym][sc] = grid->data_symbols[sym][sc] * correction;
        }
    }

    return false;
}


// FUNCIONES SINCRONIZACIÓN SCO
bool init_sco_tracker(SCOTracker *tracker, float alpha)
{

    if (!tracker || alpha <= 0.0f || alpha > 1.0f) {
        printf("Error: Parametros invalidos para SCO tracker\n");
        return true;
    }

    tracker->sco_estimate = 0.0f;
    tracker->sco_filtered = 0.0f;
    tracker->alpha = alpha;
    tracker->pilots_used = 0;
    tracker->initialized = false;
    tracker->accumulated_offset = 0.0f;
    tracker->symbol_count = 0;
    return false;
}

bool estimate_sco(const float complex current_pilots[PRB_SYMBOLS][PILOTS_PER_SYMBOL],
                  const float complex previous_pilots[PRB_SYMBOLS][PILOTS_PER_SYMBOL],
                  SCOTracker *tracker) {

    if (!tracker)
    {
        printf("Error: SCO tracker no inicializado\n");
        return true;
    }

    if (!previous_pilots)
    {
        printf("Primera estimacion SCO - inicializando\n");
        tracker->initialized = true;
        return false;
    }

    float phase_differences[PRB_SYMBOLS][PILOTS_PER_SYMBOL];
    int valid_measurements = 0;

    // Calcular diferencias de fase entre PRBs consecutivos
    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
        {
            float current_power = cabsf(current_pilots[sym][p]);
            float previous_power = cabsf(previous_pilots[sym][p]);

            if (current_power > 0.1f && previous_power > 0.1f)
            {
                float complex phase_diff = current_pilots[sym][p] * conjf(previous_pilots[sym][p]);
                phase_differences[sym][p] = cargf(phase_diff);
                valid_measurements++;
            }
            else
            {
                phase_differences[sym][p] = 0.0f;
            }
        }
    }

    if (valid_measurements < TOTAL_PILOTS / 2)
    {
        printf("Advertencia SCO: Solo %d/%d mediciones validas\n", valid_measurements, TOTAL_PILOTS);
        tracker->pilots_used = valid_measurements;

        return false;
    }

    // Estimación simple del gradiente temporal
    float total_phase_diff = 0.0f;
    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
        {
            if (phase_differences[sym][p] != 0.0f) { total_phase_diff += phase_differences[sym][p]; }
        }
    }

    float avg_phase_diff = total_phase_diff / valid_measurements;

    if (!tracker->initialized)
    {
        tracker->sco_filtered = avg_phase_diff;
        tracker->initialized = true;
    }
    else
    {
        tracker->sco_filtered = tracker->alpha * avg_phase_diff +
                                (1.0f - tracker->alpha) * tracker->sco_filtered;
    }

    tracker->sco_estimate = avg_phase_diff;
    tracker->pilots_used = valid_measurements;
    tracker->accumulated_offset += tracker->sco_filtered;
    tracker->symbol_count++;

    printf("SCO: %.6f rad/PRB, Filtrado: %.6f rad/PRB (%d mediciones)\n",
           avg_phase_diff, tracker->sco_filtered, valid_measurements);

    return false;
}

bool apply_sco_compensation(PRB_Grid *grid, SCOTracker *tracker)
{

    if (!tracker || !tracker->initialized)
    {
        printf("Advertencia: SCO tracker no inicializado, saltando compensacion\n");
        return false;
    }

    printf("Aplicando compensacion SCO: offset acumulado %.6f rad\n", tracker->accumulated_offset);

    // Compensación simple basada en offset acumulado
    float complex compensation = cosf(-tracker->accumulated_offset) + sinf(-tracker->accumulated_offset) * I;

    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        for (int sc = 0; sc < PRB_SUBCARRIERS; sc++)
        {
            grid->data_symbols[sym][sc] = grid->data_symbols[sym][sc] * compensation;
        }
    }

    return false;
}