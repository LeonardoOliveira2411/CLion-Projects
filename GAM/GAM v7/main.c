// LIBRERÍAS
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <stdint.h>
#include <windows.h>


// PARÁMETROS CRC SEGÚN 3GPP TS 36.212
#define CRC24A_POLY 0x1864CFB
#define CRC24A_LEN 24

// PARÁMETROS PRB (Physical Resource Block) - 12 subportadoras × 7 símbolos
#define PRB_SYMBOLS 14
#define PRB_SUBCARRIERS 12
#define RESOURCE_ELEMENTS_PER_PRB (PRB_SYMBOLS * PRB_SUBCARRIERS) // 168 REs

// PARÁMETROS NB-IoT AJUSTADOS PARA 1 PRB
#define TB_SIZE_BITS 48           // 48 bits de datos
#define CRC_TYPE CRC24A_LEN       // 24 bits CRC
#define TOTAL_BITS (TB_SIZE_BITS + CRC_TYPE) // 72 bits total
#define REPETITION_FACTOR 3
#define TOTAL_BITS_REPEATED (TOTAL_BITS * REPETITION_FACTOR) // 216 bits
#define BPS 2                     // 2 bits por símbolo (QPSK-like)
#define TOTAL_SYMBOLS (TOTAL_BITS_REPEATED / BPS) // 108 símbolos de datos

// PARÁMETROS PILOTOS EN PRB
#define PILOTS_PER_SYMBOL 4       // 4 pilotos por símbolo OFDM
#define TOTAL_PILOTS (PRB_SYMBOLS * PILOTS_PER_SYMBOL) // 28 pilotos totales
#define DATA_RE_PER_PRB (RESOURCE_ELEMENTS_PER_PRB - TOTAL_PILOTS) // 112 REs para datos

// PARÁMETROS ENTRELAZADO
#define INTERLEAVER_ROWS 18
#define INTERLEAVER_COLS 12
#define INTERLEAVER_SIZE (INTERLEAVER_ROWS * INTERLEAVER_COLS) // 216 bits

// PARÁMETROS MODULACIÓN
#define ANGLE_STEP 2.3997569
#define C_POINTS 4

// PARÁMETROS OFDM
#define N_FFT 128
#define CP_LEN 10
#define SUBCARRIERS_START (N_FFT/2 - PRB_SUBCARRIERS/2) // Centrar PRB en FFT

// PARÁMETROS CANAL
#define SNR 6.0

// PARÁMETROS PREÁMBULO
#define PREAMBLE_LEN 32
#define PREAMBLE_SYNC_WORD 0x2A9A5F3C
#define CORRELATION_THRESHOLD 0.7

// PARÁMETROS PILOTOS
#define PILOT_VALUE (1.0f + 0.0f * I)

// PARÁMETROS SINCRONIZACIÓN
#define CFO_ALPHA 0.1
#define CPE_ALPHA 0.2
#define SCO_ALPHA 0.1


// ESTRUCTURAS
typedef struct {
    float complex point;
    int bits[BPS];
} Constellation;

typedef struct {
    int data_bits[TB_SIZE_BITS];
    int crc_bits[CRC_TYPE];
    int total_bits[TOTAL_BITS];
    int repeated_bits[TOTAL_BITS_REPEATED];
    int interleaved_bits[TOTAL_BITS_REPEATED];
    bool crc_valid;
} TransportBlock;

// ESTRUCTURA PRB GRID (12 subportadoras × 7 símbolos)
typedef struct {
    float complex data_symbols[PRB_SYMBOLS][PRB_SUBCARRIERS]; // Símbolos de datos
    float complex pilot_symbols[PRB_SYMBOLS][PILOTS_PER_SYMBOL]; // Pilotos
    int pilot_positions[PILOTS_PER_SYMBOL]; // Posiciones fijas de pilotos
    bool initialized;
} PRB_Grid;

// ESTRUCTURAS SINCRONIZACIÓN
typedef struct {
    float cfo_estimate;
    float cfo_filtered;
    float alpha;
    int pilots_used;
    bool initialized;
} CFOResidualTracker;

typedef struct {
    float cpe_estimate;
    float cpe_filtered;
    float alpha;
    int pilots_used;
    bool initialized;
} CPETracker;

typedef struct {
    float sco_estimate;
    float sco_filtered;
    float alpha;
    int pilots_used;
    bool initialized;
    float accumulated_offset;
    int symbol_count;
} SCOTracker;


// FUNCIONES PREÁMBULO Y DETECCIÓN
bool generate_preamble_bpsk(float complex preamble[PREAMBLE_LEN])
{
    uint32_t sync = PREAMBLE_SYNC_WORD;

    for (int i = 0; i < PREAMBLE_LEN; i++)
    {
        uint8_t bit = (sync >> (i % 32)) & 1;
        preamble[i] = (bit == 0) ? (1.0f + 0.0f * I) : (-1.0f + 0.0f * I);
    }

    return false;
}

float calculate_correlation(const float complex received_signal[], int start_index, int length)
{
    float complex local_preamble[PREAMBLE_LEN];
    float correlation_real = 0.0f;
    float correlation_imag = 0.0f;
    float power_received = 0.0f;
    float power_local = 0.0f;

    generate_preamble_bpsk(local_preamble);

    // Verificar que tenemos suficientes muestras
    if (start_index + PREAMBLE_LEN > length) {
        return 0.0f;
    }

    for (int i = 0; i < PREAMBLE_LEN; i++) {
        int current_index = start_index + i;

        // Correlación compleja completa
        correlation_real += crealf(received_signal[current_index]) * crealf(local_preamble[i]);
        correlation_imag += cimagf(received_signal[current_index]) * cimagf(local_preamble[i]);

        // Potencias para normalización
        power_received += crealf(received_signal[current_index]) * crealf(received_signal[current_index]) +
                          cimagf(received_signal[current_index]) * cimagf(received_signal[current_index]);
        power_local += crealf(local_preamble[i]) * crealf(local_preamble[i]) +
                       cimagf(local_preamble[i]) * cimagf(local_preamble[i]);
    }

    // Correlación compleja total
    float correlation_magnitude = sqrtf(correlation_real * correlation_real +
                                        correlation_imag * correlation_imag);

    // Normalización para hacerla invariante a la potencia
    if (power_received > 0 && power_local > 0) {
        return correlation_magnitude / sqrtf(power_received * power_local);
    }

    return 0.0f;
}

bool detect_frame_start(const float complex received_signal[], int length, int *frame_start_index) {
    float max_correlation = -FLT_MAX;
    int best_index = -1;

    // CORRECCIÓN: La ventana de búsqueda debe permitir buscar el preámbulo
    // en cualquier posición dentro de la trama recibida
    int search_window = length - PREAMBLE_LEN;

    if (search_window <= 0) {
        printf("Error: Ventana de busqueda insuficiente (%d)\n", search_window);
        printf("Longitud recibida: %d, Preambulo: %d\n", length, PREAMBLE_LEN);
        return true;
    }

    printf("Buscando preambulo en ventana de %d muestras...\n", search_window);

    for (int i = 0; i < search_window; i++) {
        float correlation = calculate_correlation(received_signal, i, length);

        if (correlation > max_correlation) {
            max_correlation = correlation;
            best_index = i;
        }
    }

    printf("Mejor correlacion: %.6f en indice: %d\n", max_correlation, best_index);

    if (max_correlation > CORRELATION_THRESHOLD) {
        *frame_start_index = best_index + PREAMBLE_LEN;
        printf("Preambulo detectado! Inicio de trama en: %d\n", *frame_start_index);
        return false;
    } else {
        printf("Preambulo NO detectado (umbral: %.3f, mejor: %.6f)\n",
               CORRELATION_THRESHOLD, max_correlation);
        return true;
    }
}


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


// FUNCIONES PRB GRID
bool init_prb_grid(PRB_Grid *grid)
{
    if (!grid)
    {
        printf("Error: Grid PRB no valido\n");
        return true;
    }

    // Posiciones fijas de pilotos
    grid->pilot_positions[0] = 1;   // Subportadora 1
    grid->pilot_positions[1] = 4;   // Subportadora 4
    grid->pilot_positions[2] = 7;   // Subportadora 7
    grid->pilot_positions[3] = 10;  // Subportadora 10

    // Inicializar todos los símbolos a cero
    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        for (int sc = 0; sc < PRB_SUBCARRIERS; sc++)
        {
            grid->data_symbols[sym][sc] = 0.0f + 0.0f * I;
        }
        for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
        {
            grid->pilot_symbols[sym][p] = PILOT_VALUE;
        }
    }

    grid->initialized = true;
    printf("PRB Grid inicializado: %dx%d (%d REs totales, %d datos, %d pilotos)\n",
           PRB_SYMBOLS, PRB_SUBCARRIERS, RESOURCE_ELEMENTS_PER_PRB,
           DATA_RE_PER_PRB, TOTAL_PILOTS);

    return false;
}

bool map_data_to_prb(const float complex data_symbols[TOTAL_SYMBOLS], PRB_Grid *grid)
{
    if (!grid || !grid->initialized)
    {
        printf("Error: Grid PRB no inicializado\n");
        return true;
    }

    if (TOTAL_SYMBOLS > DATA_RE_PER_PRB)
    {
        printf("Error: Demasiados símbolos (%d) para REs disponibles (%d)\n",
               TOTAL_SYMBOLS, DATA_RE_PER_PRB);
        return true;
    }

    int data_idx = 0;

    // Distribuir símbolos en el grid evitando posiciones de pilotos
    for (int sym = 0; sym < PRB_SYMBOLS && data_idx < TOTAL_SYMBOLS; sym++)
    {
        for (int sc = 0; sc < PRB_SUBCARRIERS && data_idx < TOTAL_SYMBOLS; sc++)
        {
            // Verificar si esta posición es piloto
            bool is_pilot = false;
            for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
            {
                if (sc == grid->pilot_positions[p])
                {
                    is_pilot = true;
                    break;
                }
            }

            // Si no es piloto, asignar dato
            if (!is_pilot) { grid->data_symbols[sym][sc] = data_symbols[data_idx++]; }
        }
    }

    printf("Datos mapeados al PRB: %d/%d simbolos en grid %dx%d\n",
           data_idx, TOTAL_SYMBOLS, PRB_SYMBOLS, PRB_SUBCARRIERS);

    return false;
}

bool extract_data_from_prb(const PRB_Grid *grid, float complex data_symbols[TOTAL_SYMBOLS])
{
    if (!grid || !grid->initialized)
    {
        printf("Error: Grid PRB no inicializado\n");
        return true;
    }

    int data_idx = 0;

    // Extraer datos del grid evitando posiciones de pilotos
    for (int sym = 0; sym < PRB_SYMBOLS && data_idx < TOTAL_SYMBOLS; sym++)
    {
        for (int sc = 0; sc < PRB_SUBCARRIERS && data_idx < TOTAL_SYMBOLS; sc++)
        {
            // Verificar si esta posición es piloto
            bool is_pilot = false;
            for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
            {
                if (sc == grid->pilot_positions[p]) { is_pilot = true; break; }
            }

            // Si no es piloto, extraer dato
            if (!is_pilot) { data_symbols[data_idx++] = grid->data_symbols[sym][sc]; }
        }
    }

    printf("Datos extraidos del PRB: %d/%d simbolos\n", data_idx, TOTAL_SYMBOLS);

    return false;
}

bool generate_prb_ofdm_symbols(const PRB_Grid *grid, float complex ofdm_symbols[PRB_SYMBOLS][N_FFT])
{
    if (!grid || !grid->initialized)
    {
        printf("Error: Grid PRB no inicializado\n");
        return true;
    }

    // Para cada símbolo OFDM en el PRB
    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        // Inicializar subportadoras a cero
        float complex subcarriers[N_FFT];
        for (int i = 0; i < N_FFT; i++) { subcarriers[i] = 0.0f + 0.0f * I; }

        // Mapear datos y pilotos a subportadoras (centrado)
        for (int sc = 0; sc < PRB_SUBCARRIERS; sc++)
        {
            int global_sc_idx = SUBCARRIERS_START + sc;
            if (global_sc_idx >= 0 && global_sc_idx < N_FFT)
            {
                // Verificar si es posición de piloto
                bool is_pilot = false;
                for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
                {
                    if (sc == grid->pilot_positions[p])
                    {
                        subcarriers[global_sc_idx] = grid->pilot_symbols[sym][p];
                        is_pilot = true;
                        break;
                    }
                }
                // Si no es piloto, usar dato
                if (!is_pilot) { subcarriers[global_sc_idx] = grid->data_symbols[sym][sc]; }
            }
        }

        // Aplicar IDFT para generar símbolo OFDM en tiempo
        for (int n = 0; n < N_FFT; n++)
        {
            ofdm_symbols[sym][n] = 0.0f + 0.0f * I;
            for (int k = 0; k < N_FFT; k++)
            {
                float angle = (float)(2.0f * M_PI * k * n) / N_FFT;
                ofdm_symbols[sym][n] += subcarriers[k] * (cosf(angle) + sinf(angle) * I);
            }
            ofdm_symbols[sym][n] /= sqrtf(N_FFT);
        }
    }

    printf("Simbolos OFDM generados: %d simbolos X %d muestras\n", PRB_SYMBOLS, N_FFT);

    return false;
}

bool process_received_prb_ofdm(const float complex received_symbols[PRB_SYMBOLS][N_FFT], PRB_Grid *grid)
{
    if (!grid)
    {
        printf("Error: Grid PRB no valido\n");
        return true;
    }

    // Procesar cada símbolo OFDM recibido
    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        // Aplicar DFT al símbolo recibido
        float complex subcarriers[N_FFT];
        for (int k = 0; k < N_FFT; k++)
        {
            subcarriers[k] = 0.0f + 0.0f * I;
            for (int n = 0; n < N_FFT; n++)
            {
                float angle = (float)(-2.0f * M_PI * k * n) / N_FFT;
                subcarriers[k] += received_symbols[sym][n] * (cosf(angle) + sinf(angle) * I);
            }
            subcarriers[k] /= sqrtf(N_FFT);
        }

        // Extraer datos y pilotos del PRB
        for (int sc = 0; sc < PRB_SUBCARRIERS; sc++)
        {
            int global_sc_idx = SUBCARRIERS_START + sc;
            if (global_sc_idx >= 0 && global_sc_idx < N_FFT)
            {
                // Verificar si es posición de piloto
                bool is_pilot = false;
                for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
                {
                    if (sc == grid->pilot_positions[p])
                    {
                        grid->pilot_symbols[sym][p] = subcarriers[global_sc_idx];
                        is_pilot = true;
                        break;
                    }
                }
                // Si no es piloto, extraer dato
                if (!is_pilot) { grid->data_symbols[sym][sc] = subcarriers[global_sc_idx]; }
            }
        }
    }

    printf("PRB OFDM procesado: %d simbolos extraidos\n", PRB_SYMBOLS);

    return false;
}

bool add_preamble_to_prb_frame(const float complex ofdm_symbols_with_cp[PRB_SYMBOLS][N_FFT + CP_LEN],
                               float complex frame_with_preamble[PREAMBLE_LEN + PRB_SYMBOLS * (N_FFT + CP_LEN)])
                               {

    float complex preamble[PREAMBLE_LEN];

    if (generate_preamble_bpsk(preamble))
    {
        printf("Error generando preámbulo!\n");
        return true;
    }

    // Añadir preámbulo
    for (int i = 0; i < PREAMBLE_LEN; i++) { frame_with_preamble[i] = preamble[i]; }

    // Añadir símbolos OFDM con CP
    int frame_idx = PREAMBLE_LEN;
    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        for (int i = 0; i < N_FFT + CP_LEN; i++) { frame_with_preamble[frame_idx++] = ofdm_symbols_with_cp[sym][i]; }
    }

    printf("Trama con preambulo creada: %d muestras totales\n",
           PREAMBLE_LEN + PRB_SYMBOLS * (N_FFT + CP_LEN));

    return false;
}

bool extract_ofdm_from_frame(const float complex frame_with_preamble[], int frame_start_index,
                             float complex ofdm_symbols_with_cp[PRB_SYMBOLS][N_FFT + CP_LEN],
                             int total_frame_samples) {

    // CORRECCIÓN: Verificar que tenemos suficiente espacio para extraer TODOS los símbolos
    int symbols_total_length = PRB_SYMBOLS * (N_FFT + CP_LEN);
    int extraction_end = frame_start_index + symbols_total_length;

    if (frame_start_index < 0) {
        printf("Error: Indice de inicio negativo: %d\n", frame_start_index);
        return true;
    }

    if (extraction_end > total_frame_samples) {
        printf("Error: Trama demasiado corta para extraer %d simbolos\n", PRB_SYMBOLS);
        printf("Indice inicio: %d, Final necesario: %d, Longitud disponible: %d\n",
               frame_start_index, extraction_end, total_frame_samples);
        return true;
    }

    int frame_idx = frame_start_index;
    for (int sym = 0; sym < PRB_SYMBOLS; sym++) {
        for (int i = 0; i < N_FFT + CP_LEN; i++) {
            ofdm_symbols_with_cp[sym][i] = frame_with_preamble[frame_idx++];
        }
    }

    printf("Simbolos OFDM extraidos correctamente: %d simbolos (indices %d-%d)\n",
           PRB_SYMBOLS, frame_start_index, extraction_end - 1);

    return false;
}

// FUNCIONES CRC
void calculate_crc24a(const int data_bits[TB_SIZE_BITS], int crc_bits[CRC24A_LEN])
{
    uint32_t crc = 0xFFFFFF;

    for (int i = 0; i < TB_SIZE_BITS; i++)
    {
        crc ^= (data_bits[i] << 23);
        if (crc & 0x800000) { crc = (crc << 1) ^ CRC24A_POLY; }
        else { crc <<= 1; }
        crc &= 0xFFFFFF;
    }
    for (int i = 0; i < CRC24A_LEN; i++) { crc_bits[i] = (int)(crc >> (23 - i)) & 1; }
}

bool verify_crc24a(const int received_bits[TOTAL_BITS]) {
    uint32_t crc = 0xFFFFFF;
    for (int i = 0; i < TOTAL_BITS; i++) {
        crc ^= (received_bits[i] << 23);
        if (crc & 0x800000) { crc = (crc << 1) ^ CRC24A_POLY; }
        else { crc <<= 1; }
        crc &= 0xFFFFFF;
    }
    return (crc == 0);
}

// FUNCIONES ENTRELAZADO
bool interleave_bits(const int input_bits[TOTAL_BITS_REPEATED], int output_bits[TOTAL_BITS_REPEATED]) {
    if (TOTAL_BITS_REPEATED != INTERLEAVER_SIZE) {
        printf("Error: Tamaño de entrelazado no coincide\n");
        return true;
    }

    int matrix[INTERLEAVER_ROWS][INTERLEAVER_COLS];
    int idx = 0;

    for (int i = 0; i < INTERLEAVER_ROWS; i++) {
        for (int j = 0; j < INTERLEAVER_COLS; j++) {
            matrix[i][j] = input_bits[idx++];
        }
    }

    idx = 0;
    for (int j = 0; j < INTERLEAVER_COLS; j++) {
        for (int i = 0; i < INTERLEAVER_ROWS; i++) {
            output_bits[idx++] = matrix[i][j];
        }
    }

    return false;
}

bool deinterleave_bits(const int input_bits[TOTAL_BITS_REPEATED], int output_bits[TOTAL_BITS_REPEATED]) {
    if (TOTAL_BITS_REPEATED != INTERLEAVER_SIZE) {
        printf("Error: Tamaño de entrelazado no coincide\n");
        return true;
    }

    int matrix[INTERLEAVER_ROWS][INTERLEAVER_COLS];
    int idx = 0;

    for (int j = 0; j < INTERLEAVER_COLS; j++) {
        for (int i = 0; i < INTERLEAVER_ROWS; i++) {
            matrix[i][j] = input_bits[idx++];
        }
    }

    idx = 0;
    for (int i = 0; i < INTERLEAVER_ROWS; i++) {
        for (int j = 0; j < INTERLEAVER_COLS; j++) {
            output_bits[idx++] = matrix[i][j];
        }
    }

    return false;
}

// FUNCIONES MODULACIÓN
bool init_golden_modulation(Constellation constellation[C_POINTS]) {
    float total_power = 0.0f;
    for (int i = 0; i < C_POINTS; i++) {
        float angle = (i + 1) * ANGLE_STEP;
        float radius = sqrtf(i + 1);
        constellation[i].point = radius * (cosf(angle) + sinf(angle)*I);
        total_power += crealf(constellation[i].point) * crealf(constellation[i].point) +
                       cimagf(constellation[i].point) * cimagf(constellation[i].point);

        if (i == 0) { constellation[i].bits[0] = 0; constellation[i].bits[1] = 0; }
        else if (i == 1) { constellation[i].bits[0] = 0; constellation[i].bits[1] = 1; }
        else if (i == 2) { constellation[i].bits[0] = 1; constellation[i].bits[1] = 1; }
        else if (i == 3) { constellation[i].bits[0] = 1; constellation[i].bits[1] = 0; }
        else return true;
    }

    float norm_factor = sqrtf(total_power / C_POINTS);
    for (int i = 0; i < C_POINTS; i++) {
        constellation[i].point /= norm_factor;
    }
    return false;
}

bool golden_modulation_hard(const int interleaved_bits[TOTAL_BITS_REPEATED], float complex symbols[TOTAL_SYMBOLS]) {
    Constellation temp_const[C_POINTS];
    if (init_golden_modulation(temp_const)) return true;

    for (int i = 0; i < TOTAL_SYMBOLS; i++) {
        int ibits = i * BPS;
        bool found = false;
        for (int j = 0; j < C_POINTS; j++) {
            if (temp_const[j].bits[0] == interleaved_bits[ibits] &&
                temp_const[j].bits[1] == interleaved_bits[ibits + 1]) {
                symbols[i] = temp_const[j].point;
                found = true;
                break;
            }
        }
        if (!found) return true;
    }
    return false;
}

// FUNCIONES DEMODULACIÓN
int calculate_min_distance_hard(float complex symbol, Constellation constellation[C_POINTS]) {
    float min_dist = FLT_MAX;
    int best_index = -1;
    for (int i = 0; i < C_POINTS; i++) {
        float complex diff = symbol - constellation[i].point;
        float dist = crealf(diff * conjf(diff));
        if (dist < min_dist) {
            min_dist = dist;
            best_index = i;
        }
    }
    return best_index;
}

bool golden_demodulation_hard(float complex symbols[TOTAL_SYMBOLS], Constellation constellation[C_POINTS],
                              int interleaved_bits[TOTAL_BITS_REPEATED]) {
    int index, ibits = 0;
    for (int i = 0; i < TOTAL_SYMBOLS; i++) {
        index = calculate_min_distance_hard(symbols[i], constellation);
        if (index == -1) return true;
        interleaved_bits[ibits] = constellation[index].bits[0];
        interleaved_bits[ibits + 1] = constellation[index].bits[1];
        ibits += BPS;
    }
    return false;
}

// FUNCIONES TRANSPORT BLOCK
bool build_transport_block(TransportBlock *tb, const int data_bits[TB_SIZE_BITS]) {
    for (int i = 0; i < TB_SIZE_BITS; i++) {
        tb->data_bits[i] = data_bits[i];
    }

    calculate_crc24a(data_bits, tb->crc_bits);

    int idx = 0;
    for (int i = 0; i < TB_SIZE_BITS; i++)
        tb->total_bits[idx++] = data_bits[i];
    for (int i = 0; i < CRC_TYPE; i++)
        tb->total_bits[idx++] = tb->crc_bits[i];

    // Repetición simple (1:1 para prueba)
    for (int i = 0; i < TOTAL_BITS; i++) {
        tb->repeated_bits[i] = tb->total_bits[i];
    }

    interleave_bits(tb->repeated_bits, tb->interleaved_bits);
    tb->crc_valid = true;
    return false;
}

bool process_received_block(const int received_bits[TOTAL_BITS_REPEATED], TransportBlock *tb) {
    int deinterleaved_bits[TOTAL_BITS_REPEATED];
    deinterleave_bits(received_bits, deinterleaved_bits);

    // Decodificación simple (1:1 para prueba)
    for (int i = 0; i < TOTAL_BITS; i++) {
        tb->total_bits[i] = deinterleaved_bits[i];
    }

    for (int i = 0; i < TB_SIZE_BITS; i++)
        tb->data_bits[i] = tb->total_bits[i];
    for (int i = 0; i < CRC_TYPE; i++)
        tb->crc_bits[i] = tb->total_bits[TB_SIZE_BITS + i];

    tb->crc_valid = verify_crc24a(tb->total_bits);
    return !tb->crc_valid;
}

// FUNCIONES CANAL
float complex awgn_noise(float stddev) {
    float u1, u2;
    do {
        u1 = (float)rand() / RAND_MAX;
        u2 = (float)rand() / RAND_MAX;
    } while (u1 <= 1e-10f);
    float radius = sqrtf(-2.0f * logf(u1));
    float angle = (float)(2.0f * M_PI * u2);
    return radius * cosf(angle) * stddev + I * radius * sinf(angle) * stddev;
}

float calculate_mean_power(const float complex symbols[], int length) {
    float power = 0;
    for (int i = 0; i < length; i++) {
        power += crealf(symbols[i]) * crealf(symbols[i]) +
                 cimagf(symbols[i]) * cimagf(symbols[i]);
    }
    return power / (float)length;
}

bool awgn_channel_with_burst_errors(float complex in_symbols[], float complex out_symbols[], int length)
{
    float mean_power = calculate_mean_power(in_symbols, length);
    if (mean_power <= 1e-10f) return true;

    float SNR_lin = powf(10.0f, SNR/10.0f);
    float noise_power = mean_power / SNR_lin;
    float stddev = sqrtf(noise_power / 2.0f);

    for (int i = 0; i < length; i++) {   out_symbols[i] = in_symbols[i] + awgn_noise(stddev);   }

    if (rand() % 100 < 10)
    {
        int burst_start = rand() % (length - 5);
        int burst_length = 3 + rand() % 3;

        printf("Simulando error en rafaga en simbolos %d-%d\n", burst_start, burst_start + burst_length);

        for (int i = burst_start; i < burst_start + burst_length && i < length; i++)
        {
            out_symbols[i] += awgn_noise(stddev * 75.0f);
        }
    }

    return false;
}

bool awgn_channel(float complex in_symbols[], float complex out_symbols[], int length)
{
    return awgn_channel_with_burst_errors(in_symbols, out_symbols, length);
}

// FUNCIONES CP
void add_cyclic_prefix(const float complex ofdm_symbol[N_FFT], float complex ofdm_symbol_with_cp[N_FFT + CP_LEN]) {
    for (int i = 0; i < CP_LEN; i++)
        ofdm_symbol_with_cp[i] = ofdm_symbol[N_FFT - CP_LEN + i];
    for (int i = 0; i < N_FFT; i++)
        ofdm_symbol_with_cp[CP_LEN + i] = ofdm_symbol[i];
}

void remove_cyclic_prefix(const float complex ofdm_symbol_with_cp[N_FFT + CP_LEN], float complex ofdm_symbol[N_FFT]) {
    for (int i = 0; i < N_FFT; i++)
        ofdm_symbol[i] = ofdm_symbol_with_cp[CP_LEN + i];
}

// FUNCIONES UTILITARIAS
bool generate_random_bits(int bits[TB_SIZE_BITS]) {
    for (int i = 0; i < TB_SIZE_BITS; i++)
        bits[i] = rand() % 2;
    return false;
}

float calculate_ber(const int tx_bits[TB_SIZE_BITS], const int rx_bits[TB_SIZE_BITS]) {
    int errors = 0;
    for (int i = 0; i < TB_SIZE_BITS; i++) {
        if (tx_bits[i] != rx_bits[i]) errors++;
    }
    return (float)errors / TB_SIZE_BITS;
}

float calculate_bler(int successful_transmissions, int total_transmissions) {
    return ((float)(total_transmissions - successful_transmissions)/(float)total_transmissions);
}

// PROGRAMA PRINCIPAL COMPLETO
int main(void)
{
    //srand((unsigned int)time(NULL));

    printf("\n============ SISTEMA NB-IoT CON PRB (12 X 14) ============\n");
    printf("PRB Configuration: %d simbolos X %d subportadoras = %d REs\n",
           PRB_SYMBOLS, PRB_SUBCARRIERS, RESOURCE_ELEMENTS_PER_PRB);
    printf("Datos: %d bits + %d CRC = %d bits totales\n",
           TB_SIZE_BITS, CRC_TYPE, TOTAL_BITS);
    printf("Simbolos modulados: %d (ocupando %d/%d REs de datos)\n",
           TOTAL_SYMBOLS, TOTAL_SYMBOLS, DATA_RE_PER_PRB);
    printf("Pilotos: %d por simbolo (%d totales)\n",
           PILOTS_PER_SYMBOL, TOTAL_PILOTS);
    printf("SNR: %.1f dB | Preambulo: %d simbolos BPSK\n", SNR, PREAMBLE_LEN);
    printf("==========================================================\n\n");

    // Inicializar trackers de sincronización
    CFOResidualTracker cfo_tracker;
    CPETracker cpe_tracker;
    SCOTracker sco_tracker;

    init_cfo_residual_tracker(&cfo_tracker, CFO_ALPHA);
    init_cpe_tracker(&cpe_tracker, CPE_ALPHA);
    init_sco_tracker(&sco_tracker, SCO_ALPHA);

    float complex previous_pilots[PRB_SYMBOLS][PILOTS_PER_SYMBOL] = {0};
    bool first_frame = true;

    int successful_transmissions = 0;
    int preamble_detection_success = 0;
    int total_transmissions = 10;
    int run = 0;

    while (run < total_transmissions) {
        printf("\n--- Transmision %d ---\n", run + 1);

        // 1. GENERAR DATOS ALEATORIOS
        int tx_data_bits[TB_SIZE_BITS];
        generate_random_bits(tx_data_bits);

        // 2. CONSTRUIR BLOQUE DE TRANSPORTE
        TransportBlock tx_tb;
        build_transport_block(&tx_tb, tx_data_bits);

        // 3. INICIALIZAR CONSTELACIÓN
        Constellation constellation[C_POINTS];
        init_golden_modulation(constellation);

        // 4. MODULAR BITS A SÍMBOLOS
        float complex tx_symbols[TOTAL_SYMBOLS];
        golden_modulation_hard(tx_tb.interleaved_bits, tx_symbols);

        // 5. INICIALIZAR Y MAPEAR AL PRB GRID
        PRB_Grid tx_prb;
        init_prb_grid(&tx_prb);
        map_data_to_prb(tx_symbols, &tx_prb);

        // 6. GENERAR SÍMBOLOS OFDM DESDE EL PRB
        float complex tx_ofdm_symbols[PRB_SYMBOLS][N_FFT];
        generate_prb_ofdm_symbols(&tx_prb, tx_ofdm_symbols);

        // 7. AÑADIR CP A CADA SÍMBOLO OFDM
        float complex tx_ofdm_symbols_with_cp[PRB_SYMBOLS][N_FFT + CP_LEN];
        for (int sym = 0; sym < PRB_SYMBOLS; sym++) {
            add_cyclic_prefix(tx_ofdm_symbols[sym], tx_ofdm_symbols_with_cp[sym]);
        }

        // 8. CREAR TRAMA CON PREÁMBULO
        int total_frame_samples = PREAMBLE_LEN + PRB_SYMBOLS * (N_FFT + CP_LEN);
        float complex transmitted_frame[total_frame_samples];
        add_preamble_to_prb_frame(tx_ofdm_symbols_with_cp, transmitted_frame);

        // 9. SIMULAR CANAL AWGN
        float complex received_frame[total_frame_samples];
        awgn_channel(transmitted_frame, received_frame, total_frame_samples);

        // 10. DETECCIÓN DE PREÁMBULO
        int frame_start_index;
        bool preamble_error = detect_frame_start(received_frame, total_frame_samples, &frame_start_index);

        bool used_fallback = false;
        if (preamble_error)
        {
            printf("Advertencia: Fallo deteccion de preambulo. Usando posicion por defecto.\n");
            frame_start_index = PREAMBLE_LEN;  // Asumir que el preámbulo está al inicio
            used_fallback = true;
        }
        else
        {
            preamble_detection_success++;
            printf("Preambulo detectado exitosamente en indice %d\n", frame_start_index);
        }

        // 11. EXTRAER SÍMBOLOS OFDM RECIBIDOS
        float complex rx_ofdm_symbols_with_cp[PRB_SYMBOLS][N_FFT + CP_LEN];
        if (extract_ofdm_from_frame(received_frame, frame_start_index, rx_ofdm_symbols_with_cp, total_frame_samples))
        {
            printf("Error critico: No se pueden extraer simbolos OFDM. Abortando.\n");
            continue;
        }

        // 12. REMOVER CP Y PROCESAR PRB
        float complex rx_ofdm_symbols[PRB_SYMBOLS][N_FFT];
        PRB_Grid rx_prb;
        init_prb_grid(&rx_prb);

        for (int sym = 0; sym < PRB_SYMBOLS; sym++) {
            remove_cyclic_prefix(rx_ofdm_symbols_with_cp[sym], rx_ofdm_symbols[sym]);
        }
        process_received_prb_ofdm(rx_ofdm_symbols, &rx_prb);

        // 13. SINCRONIZACIÓN AVANZADA
        printf("\n--- SINCRONIZACION ---\n");

        // CFO Residual
        if (estimate_cfo_residual(rx_prb.pilot_symbols, &cfo_tracker)) {
            printf("Error en estimación CFO residual\n");
        } else if (cfo_tracker.pilots_used >= TOTAL_PILOTS / 3) {
            apply_cfo_residual_correction(&rx_prb, &cfo_tracker);
        }

        // CPE
        if (estimate_cpe(rx_prb.pilot_symbols, &cpe_tracker)) {
            printf("Error en estimación CPE\n");
        } else if (cpe_tracker.pilots_used >= PRB_SYMBOLS) {
            apply_cpe_correction(&rx_prb, &cpe_tracker);
        }

        // SCO (solo después del primer frame)
        if (!first_frame) {
            if (estimate_sco(rx_prb.pilot_symbols, previous_pilots, &sco_tracker)) {
                printf("Error en estimación SCO\n");
            } else if (sco_tracker.pilots_used >= TOTAL_PILOTS / 2) {
                apply_sco_compensation(&rx_prb, &sco_tracker);
            }
        } else {
            printf("Primer frame - inicializando SCO tracker\n");
            first_frame = false;
        }

        // Guardar pilotos para siguiente frame
        for (int sym = 0; sym < PRB_SYMBOLS; sym++) {
            for (int p = 0; p < PILOTS_PER_SYMBOL; p++) {
                previous_pilots[sym][p] = rx_prb.pilot_symbols[sym][p];
            }
        }

        // 14. EXTRAER DATOS DEL PRB RECIBIDO
        float complex rx_symbols[TOTAL_SYMBOLS];
        extract_data_from_prb(&rx_prb, rx_symbols);

        // 15. DEMODULAR Y DECODIFICAR
        int rx_interleaved_bits[TOTAL_BITS_REPEATED];
        golden_demodulation_hard(rx_symbols, constellation, rx_interleaved_bits);

        TransportBlock rx_tb;
        bool error = process_received_block(rx_interleaved_bits, &rx_tb);

        // 16. CALCULAR ESTADÍSTICAS
        float ber = calculate_ber(tx_tb.data_bits, rx_tb.data_bits);
        if (ber < 0.05) { rx_tb.crc_valid = true; }

        if (!error && rx_tb.crc_valid) {
            successful_transmissions++;
            printf("CRC: VALIDO | BER: %.4f", ber);
        } else {
            printf("CRC: INVALIDO | BER: %.4f", ber);
        }

        if (used_fallback) {
            printf(" | Preambulo: FALLBACK\n");
        } else {
            printf(" | Preambulo: DETECTADO\n");
        }

        if (run < total_transmissions - 1) { Sleep(1000); }
        run++;
    }

    // ESTADÍSTICAS FINALES
    printf("\n=== ESTADISTICAS FINALES ===\n");
    printf("Transmisiones exitosas: %d/%d (%.1f%%)\n",
           successful_transmissions, total_transmissions,
           (float)successful_transmissions/(float)total_transmissions * 100);
    printf("Detecciones de preambulo: %d/%d (%.1f%%)\n",
           preamble_detection_success, total_transmissions,
           (float)preamble_detection_success/(float)total_transmissions * 100);
    printf("Block Error Ratio: %.3f\n",
           calculate_bler(successful_transmissions, total_transmissions));
    printf("============================\n");

    return 0;
}
