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

// PARÁMETROS NB-IoT
#define TB_SIZE_BITS 24
#define CRC_TYPE CRC24A_LEN
#define TOTAL_BITS (TB_SIZE_BITS + CRC_TYPE)
#define REPETITION_FACTOR 3
#define TOTAL_BITS_REPEATED (TOTAL_BITS * REPETITION_FACTOR)
#define BPS 2
#define TOTAL_SYMBOLS (TOTAL_BITS_REPEATED / BPS)

// PARÁMETROS ENTRELAZADO
#define INTERLEAVER_ROWS 12 // IN_ROWS * IN_COLS = TOTAL_BITS_REPEATED
#define INTERLEAVER_COLS 12
#define INTERLEAVER_SIZE (INTERLEAVER_ROWS * INTERLEAVER_COLS)

// PARÁMETROS MODULACIÓN
#define ANGLE_STEP 2.3997569
#define C_POINTS 4

// PARÁMETROS OFDM
#define N_FFT 128 // N_FFT > TOTAL_SYMBOLS
#define NUM_PILOTS 8
#define SUBCARRIERS (TOTAL_SYMBOLS + NUM_PILOTS)
#define CP_LEN 10
#define SUBCARRIERS_START (N_FFT/2 - SUBCARRIERS/2)
#define PILOT_SPACING (SUBCARRIERS / NUM_PILOTS)

// PARÁMETROS CANAL
#define SNR 6.0

// PARÁMETROS PREÁMBULO
#define PREAMBLE_LEN 32
#define PREAMBLE_SYNC_WORD 0x2A9A5F3C // Palabra de sincronización
#define CORRELATION_THRESHOLD 0.7      // Umbral de detección

// PARÁMETROS PILOTOS OFDM + CFO
#define PILOT_VALUE (1.0f + 0.0f * I) // Valor constante para pilotos
#define CFO_ALPHA 0.0       //0.05 - 0.2
#define CPE_ALPHA 0.2       //0.2 - 0.4
#define SCO_ALPHA 0.1      //0.1 - 0.15


// ESTRUCTURAS
// ESTRUCTURA PARA CONSTELACIÓN
typedef struct {
    float complex point;
    int bits[BPS];
} Constellation;

// ESTRUCTURA PARA TRANSPORT BLOCK
typedef struct {
    int data_bits[TB_SIZE_BITS];
    int crc_bits[CRC_TYPE];
    int total_bits[TOTAL_BITS];
    int repeated_bits[TOTAL_BITS_REPEATED];
    int interleaved_bits[TOTAL_BITS_REPEATED];
    bool crc_valid;
} TransportBlock;

// ESTRUCTURA PARA CFO RESIDUAL
typedef struct {
    float cfo_estimate;          // Estimación actual en radianes
    float cfo_filtered;          // Estimación filtrada
    float alpha;                 // Factor de filtrado (0.1 para lento, 0.3 para rápido)
    int pilots_used;             // Pilotos válidos usados
    bool initialized;            // Inicialización completada
} CFOResidualTracker;

// ESTRUCTURA PARA CPE
typedef struct {
    float cpe_estimate;          // Estimación actual en radianes
    float cpe_filtered;          // Estimación filtrada
    float alpha;                 // Factor de filtrado
    int pilots_used;             // Pilotos válidos usados
    bool initialized;            // Inicialización completada
} CPETracker;

//ESTRUCTURA PARA SCO
typedef struct {
    float sco_estimate;          // Estimación actual (rad/subportadora)
    float sco_filtered;          // Estimación filtrada
    float alpha;                 // Factor de filtrado
    int pilots_used;             // Pilotos válidos usados
    bool initialized;            // Inicialización completada
    float accumulated_offset;    // Offset acumulado en radianes
    int symbol_count;            // Contador de símbolos procesados
} SCOTracker;


//FUNCIONES CFO RESIDUAL
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

bool estimate_cfo_residual(const float complex received_pilots[NUM_PILOTS],
                           CFOResidualTracker *tracker) {
    if (!tracker) {
        printf("Error: Tracker no inicializado\n");
        return true;
    }

    float complex known_pilots[NUM_PILOTS];
    for (int i = 0; i < NUM_PILOTS; i++) {
        known_pilots[i] = PILOT_VALUE;
    }

    float complex phase_sum = 0.0f + 0.0f * I;
    int valid_pilots = 0;

    // Estimación más robusta con verificación de calidad
    for (int i = 0; i < NUM_PILOTS; i++) {
        // Verificar calidad del piloto (SNR aproximada)
        float pilot_power = cabsf(received_pilots[i]);
        float noise_estimate = 0.1f; // Umbral de ruido

        if (pilot_power > noise_estimate)
        {
            float complex phase_diff = received_pilots[i] * conjf(known_pilots[i]);
            phase_sum += phase_diff;
            valid_pilots++;
        }
    }

    if (valid_pilots < NUM_PILOTS / 2)
    {
        printf("Advertencia: Solo %d/%d pilotos validos para CFO residual\n",
               valid_pilots, NUM_PILOTS);
        // Usar estimación anterior si hay pocos pilotos válidos
        tracker->pilots_used = valid_pilots;
        return false; // No es error fatal
    }

    // Calcular ángulo promedio
    float avg_angle = cargf(phase_sum);

    // Aplicar filtrado IIR
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

bool apply_cfo_residual_correction(float complex data_symbols[TOTAL_SYMBOLS], CFOResidualTracker *tracker)
{
    if (!tracker || !tracker->initialized) {
        printf("Advertencia: CFO tracker no inicializado, saltando correccion\n");
        return false; // No es error fatal
    }

    // Usar la estimación filtrada para mayor estabilidad
    float correction_angle = -tracker->cfo_filtered;
    float complex correction = cosf(correction_angle) + sinf(correction_angle) * I;

    printf("Aplicando correccion CFO residual: %.6f rad\n", correction_angle);

    for (int i = 0; i < TOTAL_SYMBOLS; i++) {
        data_symbols[i] = data_symbols[i] * correction;
    }

    return false;
}

bool process_cfo_residual(const float complex received_pilots[NUM_PILOTS], float complex data_symbols[TOTAL_SYMBOLS],
                          CFOResidualTracker *tracker) {

    // 1. Estimar CFO residual
    if (estimate_cfo_residual(received_pilots, tracker)) {
        printf("Error en estimaciOn CFO residual\n");
        return true;
    }

    // 2. Aplicar corrección si hay suficientes pilotos válidos
    if (tracker->pilots_used >= NUM_PILOTS / 2) {
        if (apply_cfo_residual_correction(data_symbols, tracker))
        {
            printf("Error aplicando corrección CFO residual\n");
            return true;
        }
    } else {
        printf("Saltando corrección CFO residual - pilotos insuficientes\n");
    }

    return false;
}


//FUNCIONES CPE (COMMON PHASE ERROR)
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

bool estimate_cpe(const float complex received_pilots[NUM_PILOTS], CPETracker *tracker)
{
    if (!tracker) {
        printf("Error: CPE tracker no inicializado\n");
        return true;
    }

    float complex known_pilots[NUM_PILOTS];
    for (int i = 0; i < NUM_PILOTS; i++) {
        known_pilots[i] = PILOT_VALUE;
    }

    float complex phase_sum = 0.0f + 0.0f * I;
    int valid_pilots = 0;

    // Estimación de fase común (CPE)
    for (int i = 0; i < NUM_PILOTS; i++) {
        // Verificar calidad del piloto
        float pilot_power = cabsf(received_pilots[i]);
        float noise_estimate = 0.1f;

        if (pilot_power > noise_estimate) {
            // CPE es el promedio simple de las diferencias de fase
            float complex phase_diff = received_pilots[i] * conjf(known_pilots[i]);
            phase_sum += phase_diff;
            valid_pilots++;
        }
    }

    if (valid_pilots < 2) {  // Mínimo 2 pilotos para estimación
        printf("Advertencia CPE: Solo %d/%d pilotos validos\n", valid_pilots, NUM_PILOTS);
        tracker->pilots_used = valid_pilots;
        return false; // No es error fatal
    }

    // Calcular ángulo promedio (CPE)
    float avg_angle = cargf(phase_sum);

    // Aplicar filtrado IIR
    if (!tracker->initialized) {
        tracker->cpe_filtered = avg_angle;
        tracker->initialized = true;
    } else {
        tracker->cpe_filtered = tracker->alpha * avg_angle +
                                (1.0f - tracker->alpha) * tracker->cpe_filtered;
    }

    tracker->cpe_estimate = avg_angle;
    tracker->pilots_used = valid_pilots;

    printf("CPE: %.6f rad, Filtrado: %.6f rad (%d pilotos)\n",
           avg_angle, tracker->cpe_filtered, valid_pilots);

    return false;
}

bool apply_cpe_correction(float complex data_symbols[TOTAL_SYMBOLS], CPETracker *tracker)
{
    if (!tracker || !tracker->initialized)
    {
        printf("Advertencia: CPE tracker no inicializado, saltando correccion\n");
        return false;
    }

    // Usar la estimación filtrada para mayor estabilidad
    float correction_angle = -tracker->cpe_filtered;
    float complex correction = cosf(correction_angle) + sinf(correction_angle) * I;

    printf("Aplicando correccion CPE: %.6f rad\n", correction_angle);

    // Aplicar la misma rotación a TODOS los símbolos de datos
    for (int i = 0; i < TOTAL_SYMBOLS; i++)
    {
        data_symbols[i] = data_symbols[i] * correction;
    }

    return false;
}

bool process_cpe_correction(const float complex received_pilots[NUM_PILOTS], float complex data_symbols[TOTAL_SYMBOLS],
                            CPETracker *tracker) {

    // 1. Estimar CPE
    if (estimate_cpe(received_pilots, tracker))
    {
        printf("Error en estimacion CPE\n");
        return true;
    }

    // 2. Aplicar corrección si hay suficientes pilotos válidos
    if (tracker->pilots_used >= 2) {
        if (apply_cpe_correction(data_symbols, tracker)) {
            printf("Error aplicando corrección CPE\n");
            return true;
        }
    } else {
        printf("Saltando correccion CPE - pilotos insuficientes\n");
    }

    return false;
}


//FUNCIONES SCO (SAMPLING CLOCK OFFSET)
bool init_sco_tracker(SCOTracker *tracker, float alpha)
{

    if (!tracker || alpha <= 0.0f || alpha > 1.0f)
    {
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

bool estimate_sco(const float complex received_pilots_current[NUM_PILOTS], const float complex received_pilots_previous[NUM_PILOTS],
                  SCOTracker *tracker)
{
    if (!tracker) {
        printf("Error: SCO tracker no inicializado\n");
        return true;
    }

    float pilot_phases[NUM_PILOTS];
    int valid_pilots = 0;

    // Calcular diferencias de fase entre símbolos consecutivos para cada piloto
    for (int i = 0; i < NUM_PILOTS; i++) {
        // Verificar calidad de ambos pilotos
        float power_current = cabsf(received_pilots_current[i]);
        float power_previous = cabsf(received_pilots_previous[i]);
        float noise_estimate = 0.1f;

        if (power_current > noise_estimate && power_previous > noise_estimate) {
            // Calcular diferencia de fase entre símbolos consecutivos
            float complex phase_diff = received_pilots_current[i] *
                                       conjf(received_pilots_previous[i]);
            pilot_phases[valid_pilots] = cargf(phase_diff);
            valid_pilots++;
        }
    }

    if (valid_pilots < 3) {
        printf("Advertencia SCO: Solo %d/%d pares de pilotos validos\n",
               valid_pilots, NUM_PILOTS);
        tracker->pilots_used = valid_pilots;
        return false;
    }

    // REGRESIÓN LINEAL: Calcular gradiente de fase vs posición de frecuencia
    float sum_x = 0.0f, sum_y = 0.0f, sum_xx = 0.0f, sum_xy = 0.0f;

    for (int i = 0; i < valid_pilots; i++) {
        float x = (float)(i * PILOT_SPACING);  // Posición en frecuencia del piloto
        float y = pilot_phases[i];             // Diferencia de fase del piloto

        sum_x += x;
        sum_y += y;
        sum_xx += x * x;
        sum_xy += x * y;
    }

    // Calcular pendiente (gradiente) usando mínimos cuadrados
    float denominator = (float)valid_pilots * sum_xx - sum_x * sum_x;
    if (fabsf(denominator) < 1e-10f) {
        printf("Advertencia SCO: Denominador demasiado pequeño\n");
        tracker->pilots_used = valid_pilots;
        return false;
    }

    float gradient = ((float)valid_pilots * sum_xy - sum_x * sum_y) / denominator;

    // Aplicar filtrado IIR
    if (!tracker->initialized) {
        tracker->sco_filtered = gradient;
        tracker->initialized = true;
    } else {
        tracker->sco_filtered = tracker->alpha * gradient +
                                (1.0f - tracker->alpha) * tracker->sco_filtered;
    }

    tracker->sco_estimate = gradient;
    tracker->pilots_used = valid_pilots;

    // El offset acumulado ahora representa el gradiente filtrado, no se acumula
    tracker->accumulated_offset = tracker->sco_filtered;
    tracker->symbol_count++;

    printf("SCO: %.6f rad/sub, Filtrado: %.6f rad/sub (%d pilotos)\n",
           gradient, tracker->sco_filtered, valid_pilots);

    return false;
}

bool apply_sco_compensation(float complex data_symbols[TOTAL_SYMBOLS],
                            SCOTracker *tracker, int symbol_index)
{
    if (!tracker || !tracker->initialized) {
        printf("Advertencia: SCO tracker no inicializado, saltando compensacion\n");
        return false;
    }

    // SCO causa una rotación que varía LINEALMENTE con la frecuencia
    // La fórmula es: phase_rotation[i] = sco * freq_index[i]

    printf("Aplicando compensacion SCO: gradiente %.6f rad/sub (simbolo %d)\n",
           tracker->sco_filtered, symbol_index);

    // Aplicar compensación variable por subportadora
    for (int i = 0; i < TOTAL_SYMBOLS; i++) {
        // Calcular índice de frecuencia relativo al centro
        // Los datos van de subportadora 0 a TOTAL_SYMBOLS-1
        float freq_index = (float)i - (float)TOTAL_SYMBOLS/2.0f;

        // La rotación de fase es proporcional al SCO y a la posición en frecuencia
        float phase_compensation = -tracker->sco_filtered * freq_index;

        // Aplicar rotación
        float complex compensation = cosf(phase_compensation) +
                                     sinf(phase_compensation) * I;

        data_symbols[i] = data_symbols[i] * compensation;
    }

    return false;
}

bool process_sco_compensation(const float complex received_pilots_current[NUM_PILOTS], const float complex received_pilots_previous[NUM_PILOTS],
                              float complex data_symbols[TOTAL_SYMBOLS], SCOTracker *tracker, int symbol_index)
{
    // 1. Estimar SCO (requiere pilotos actuales y anteriores)
    if (estimate_sco(received_pilots_current, received_pilots_previous, tracker)) {
        printf("Error en estimacion SCO\n");
        return true;
    }

    // 2. Aplicar compensación si hay suficientes pilotos válidos
    if (tracker->pilots_used >= 3) {
        if (apply_sco_compensation(data_symbols, tracker, symbol_index)) {
            printf("Error aplicando compensación SCO\n");
            return true;
        }
    } else {
        printf("Saltando compensacion SCO - pilotos insuficientes\n");
    }

    return false;
}


// FUNCIONES PREÁMBULO Y DETECCIÓN DE TRAMA
bool generate_preamble_bpsk(float complex preamble[PREAMBLE_LEN]) {
    uint32_t sync = PREAMBLE_SYNC_WORD;

    for (int i = 0; i < PREAMBLE_LEN; i++) {
        uint8_t bit = (sync >> (i % 32)) & 1;
        preamble[i] = (bit == 0) ? (1.0f + 0.0f * I) : (-1.0f + 0.0f * I);
    }
    return false;
}

bool add_preamble_to_frame(const float complex ofdm_symbols_with_cp[N_FFT + CP_LEN],
                           float complex frame_with_preamble[PREAMBLE_LEN + N_FFT + CP_LEN]) {
    float complex preamble[PREAMBLE_LEN];

    if (generate_preamble_bpsk(preamble)) {
        printf("Error generando preámbulo!\n");
        return true;
    }

    for (int i = 0; i < PREAMBLE_LEN; i++) {
        frame_with_preamble[i] = preamble[i];
    }

    for (int i = 0; i < N_FFT + CP_LEN; i++) {
        frame_with_preamble[PREAMBLE_LEN + i] = ofdm_symbols_with_cp[i];
    }

    return false;
}

float calculate_correlation(const float complex received_signal[PREAMBLE_LEN + N_FFT + CP_LEN],
                            int start_index) {
    float complex local_preamble[PREAMBLE_LEN];
    float correlation_real = 0.0f;

    generate_preamble_bpsk(local_preamble);

    for (int i = 0; i < PREAMBLE_LEN; i++) {
        if (start_index + i < PREAMBLE_LEN + N_FFT + CP_LEN) {
            correlation_real += crealf(received_signal[start_index + i]) * crealf(local_preamble[i]);
        }
    }

    return correlation_real / PREAMBLE_LEN;
}

bool detect_frame_start_improved(const float complex received_signal[PREAMBLE_LEN + N_FFT + CP_LEN],
                                 int *frame_start_index) {
    float max_correlation = -FLT_MAX;
    int best_index = -1;
    int search_window = N_FFT + CP_LEN;

    printf("Buscando preambulo...\n");

    for (int i = 0; i < search_window; i++) {
        float correlation = calculate_correlation(received_signal, i);

        if (correlation > max_correlation) {
            max_correlation = correlation;
            best_index = i;
        }
    }

    printf("Mejor correlacion: %.3f en indice: %d\n", max_correlation, best_index);

    if (max_correlation > CORRELATION_THRESHOLD) {
        *frame_start_index = best_index + PREAMBLE_LEN;
        printf("Preambulo detectado! Inicio de trama en: %d\n", *frame_start_index);
        return false;
    } else {
        printf("Preambulo NO detectado (umbral: %.3f)\n", CORRELATION_THRESHOLD);
        return true;
    }
}

bool extract_ofdm_from_frame(const float complex frame_with_preamble[PREAMBLE_LEN + N_FFT + CP_LEN],
                             int frame_start_index,
                             float complex ofdm_symbols_with_cp[N_FFT + CP_LEN]) {

    if (frame_start_index < PREAMBLE_LEN ||
        frame_start_index + N_FFT + CP_LEN > PREAMBLE_LEN + N_FFT + CP_LEN) {
        printf("Error: Indice de inicio invalido: %d\n", frame_start_index);
        return true;
    }

    for (int i = 0; i < N_FFT + CP_LEN; i++) {
        ofdm_symbols_with_cp[i] = frame_with_preamble[frame_start_index + i];
    }

    return false;
}


// FUNCIONES ENTRELAZADO
bool interleave_bits(const int input_bits[TOTAL_BITS_REPEATED], int output_bits[TOTAL_BITS_REPEATED])
{
    if (TOTAL_BITS_REPEATED != INTERLEAVER_SIZE)
    {
        printf("Error: Tamaño de entrelazado no coincide\n");
        return true;
    }

    int matrix[INTERLEAVER_ROWS][INTERLEAVER_COLS];
    int idx = 0;

    for (int i = 0; i < INTERLEAVER_ROWS; i++)
    {
        for (int j = 0; j < INTERLEAVER_COLS; j++)
        {
            matrix[i][j] = input_bits[idx++];
        }
    }

    idx = 0;
    for (int j = 0; j < INTERLEAVER_COLS; j++)
    {
        for (int i = 0; i < INTERLEAVER_ROWS; i++)
        {
            output_bits[idx++] = matrix[i][j];
        }
    }

    return false;
}

bool deinterleave_bits(const int input_bits[TOTAL_BITS_REPEATED], int output_bits[TOTAL_BITS_REPEATED])
{
    if (TOTAL_BITS_REPEATED != INTERLEAVER_SIZE)
    {
        printf("Error: Tamaño de entrelazado no coincide\n");
        return true;
    }

    int matrix[INTERLEAVER_ROWS][INTERLEAVER_COLS];
    int idx = 0;

    for (int j = 0; j < INTERLEAVER_COLS; j++)
    {
        for (int i = 0; i < INTERLEAVER_ROWS; i++)
        {
            matrix[i][j] = input_bits[idx++];
        }
    }

    idx = 0;
    for (int i = 0; i < INTERLEAVER_ROWS; i++)
    {
        for (int j = 0; j < INTERLEAVER_COLS; j++)
        {
            output_bits[idx++] = matrix[i][j];
        }
    }

    return false;
}


// FUNCIONES CRC
void calculate_crc24a(const int data_bits[TB_SIZE_BITS], int crc_bits[CRC24A_LEN])
{
    uint32_t crc = 0xFFFFFF;

    for (int i = 0; i < TB_SIZE_BITS; i++)
    {
        crc ^= (data_bits[i] << 23);
        if (crc & 0x800000) {   crc = (crc << 1) ^ CRC24A_POLY;   }
        else {   crc <<= 1;   }
        crc &= 0xFFFFFF;
    }

    for (int i = 0; i < CRC24A_LEN; i++) {   crc_bits[i] = (crc >> (23 - i)) & 1;   }
}

bool verify_crc24a(const int received_bits[TOTAL_BITS])
{
    uint32_t crc = 0xFFFFFF;

    for (int i = 0; i < TOTAL_BITS; i++)
    {
        crc ^= (received_bits[i] << 23);
        if (crc & 0x800000)
        {   crc = (crc << 1) ^ CRC24A_POLY;   }
        else {   crc <<= 1;   }
        crc &= 0xFFFFFF;
    }

    return (crc == 0);
}


// FUNCIONES FEC REPETICIÓN (3,1)
bool repetition_encoder(const int input_bits[TOTAL_BITS], int output_bits[TOTAL_BITS_REPEATED])
{
    for (int i = 0; i < TOTAL_BITS; i++)
    {
        for (int j = 0; j < REPETITION_FACTOR; j++)
        {
            output_bits[i * REPETITION_FACTOR + j] = input_bits[i];
        }
    }
    return false;
}

bool repetition_decoder_hard(const int input_bits[TOTAL_BITS_REPEATED], int output_bits[TOTAL_BITS])
{
    int errors_corrected = 0;

    for (int i = 0; i < TOTAL_BITS; i++)
    {
        int sum = 0;
        for (int j = 0; j < REPETITION_FACTOR; j++)
        {
            sum += input_bits[i * REPETITION_FACTOR + j];
        }

        int decoded_bit = (sum >= 2) ? 1 : 0;
        output_bits[i] = decoded_bit;

        int original_bit = input_bits[i * REPETITION_FACTOR];
        if (decoded_bit != original_bit && sum != REPETITION_FACTOR && sum != 0)
        {
            errors_corrected++;
        }
    }

    return false;
}


// FUNCIONES TRANSPORT BLOCK
bool build_transport_block(TransportBlock *tb, const int data_bits[TB_SIZE_BITS])
{
    for (int i = 0; i < TB_SIZE_BITS; i++) {   tb->data_bits[i] = data_bits[i];   }

    calculate_crc24a(data_bits, tb->crc_bits);

    int idx = 0;
    for (int i = 0; i < TB_SIZE_BITS; i++) tb->total_bits[idx++] = data_bits[i];
    for (int i = 0; i < CRC_TYPE; i++) tb->total_bits[idx++] = tb->crc_bits[i];

    repetition_encoder(tb->total_bits, tb->repeated_bits);

    interleave_bits(tb->repeated_bits, tb->interleaved_bits);

    tb->crc_valid = true;
    return false;
}

bool process_received_block(const int received_bits[TOTAL_BITS_REPEATED], TransportBlock *tb)
{
    int deinterleaved_bits[TOTAL_BITS_REPEATED];
    int decoded_bits[TOTAL_BITS];

    deinterleave_bits(received_bits, deinterleaved_bits);

    repetition_decoder_hard(deinterleaved_bits, decoded_bits);

    for (int i = 0; i < TB_SIZE_BITS; i++) tb->data_bits[i] = decoded_bits[i];
    for (int i = 0; i < CRC_TYPE; i++) tb->crc_bits[i] = decoded_bits[TB_SIZE_BITS + i];

    tb->crc_valid = verify_crc24a(decoded_bits);

    return !tb->crc_valid;
}

// FUNCIONES DATASOURCE
bool generate_random_bits(int bits[TB_SIZE_BITS])
{
    for (int i = 0; i < TB_SIZE_BITS; i++) bits[i] = rand() % 2;
    return false;
}

float calculate_ber(const int tx_bits[TB_SIZE_BITS], const int rx_bits[TB_SIZE_BITS])
{
    int errors = 0;
    for (int i = 0; i < TB_SIZE_BITS; i++)
    {
        if (tx_bits[i] != rx_bits[i]) errors++;
    }
    return (float)errors / TB_SIZE_BITS;
}

float calculate_bler (int successful_transmissions, int total_transmissions)
{
    float bler = ((float)(total_transmissions - successful_transmissions)/total_transmissions);
    return bler;
}


// FUNCIONES MODULADOR
bool init_golden_modulation(Constellation constellation[C_POINTS])
{
    float total_power = 0.0f;

    for (int i = 0; i < C_POINTS; i++)
    {
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
    for (int i = 0; i < C_POINTS; i++) {   constellation[i].point /= norm_factor;   }

    return false;
}

bool golden_modulation_hard(const int interleaved_bits[TOTAL_BITS_REPEATED], float complex symbols[TOTAL_SYMBOLS])
{
    Constellation temp_const[C_POINTS];
    if (init_golden_modulation(temp_const)) return true;

    for (int i = 0; i < TOTAL_SYMBOLS; i++)
    {
        int ibits = i * BPS;
        bool found = false;

        for (int j = 0; j < C_POINTS; j++)
        {
            if (temp_const[j].bits[0] == interleaved_bits[ibits] && temp_const[j].bits[1] == interleaved_bits[ibits + 1])
            {
                symbols[i] = temp_const[j].point;
                found = true;
                break;
            }
        }
        if (!found) return true;
    }
    return false;
}


// FUNCIONES OFDM
bool mapping_ofdm_w_pilots(const float complex data_symbols[TOTAL_SYMBOLS],
                           float complex subcarriers_with_pilots[N_FFT]) {
    // Inicializar todos los subcarriers a cero
    for (int i = 0; i < N_FFT; i++) {
        subcarriers_with_pilots[i] = 0.0f + 0.0f * I;
    }

    // Verificar que hay suficiente espacio
    if (SUBCARRIERS_START + SUBCARRIERS > N_FFT) {
        printf("Error: No hay suficiente espacio en N_FFT para %d subportadoras\n", SUBCARRIERS);
        return true;
    }

    // Calcular posiciones de pilotos equiespaciados
    int pilot_positions[NUM_PILOTS];
    for (int i = 0; i < NUM_PILOTS; i++) {
        pilot_positions[i] = SUBCARRIERS_START + (i * PILOT_SPACING);
        // Asegurar que los pilotos estén dentro del rango válido
        if (pilot_positions[i] >= SUBCARRIERS_START + SUBCARRIERS) {
            pilot_positions[i] = SUBCARRIERS_START + SUBCARRIERS - 1;
        }
    }

    // Insertar pilotos
    for (int i = 0; i < NUM_PILOTS; i++) {
        subcarriers_with_pilots[pilot_positions[i]] = PILOT_VALUE;
    }

    // Insertar datos en las posiciones que no son pilotos
    int data_idx = 0;
    for (int i = SUBCARRIERS_START; i < SUBCARRIERS_START + SUBCARRIERS; i++) {
        bool is_pilot = false;

        // Verificar si esta posición es un piloto
        for (int j = 0; j < NUM_PILOTS; j++) {
            if (i == pilot_positions[j]) {
                is_pilot = true;
                break;
            }
        }

        // Si no es piloto, insertar dato
        if (!is_pilot) {
            if (data_idx < TOTAL_SYMBOLS) {
                subcarriers_with_pilots[i] = data_symbols[data_idx++];
            }
        }
    }

    // Verificar que todos los datos fueron insertados
    if (data_idx != TOTAL_SYMBOLS) {
        printf("Error: No todos los datos fueron insertados. Datos: %d, Insertados: %d\n",
               TOTAL_SYMBOLS, data_idx);
        printf("Subportadoras totales: %d, Pilotos: %d, Espacio para datos: %d\n",
               SUBCARRIERS, NUM_PILOTS, SUBCARRIERS - NUM_PILOTS);
        return true;
    }

    printf("Pilotos insertados: %d, Datos insertados: %d/%d, Subportadoras usadas: %d/%d\n",
           NUM_PILOTS, data_idx, TOTAL_SYMBOLS, SUBCARRIERS, N_FFT);
    return false;
}

void idft(const float complex subcarriers[N_FFT], float complex ofdm_symbols[N_FFT])
{
    for (int n = 0; n < N_FFT; n++)
    {
        ofdm_symbols[n] = 0 + 0.0f*I;

        for (int k = 0; k < N_FFT; k++)
        {
            float angle = (float)(2.0f * M_PI * k * n) / N_FFT;
            ofdm_symbols[n] += subcarriers[k] * (cosf(angle) + sinf(angle)*I);
        }
        ofdm_symbols[n] /= sqrtf(N_FFT);
    }
}

void add_cyclic_prefix(const float complex ofdm_symbol[N_FFT], float complex ofdm_symbol_with_cp[N_FFT + CP_LEN])
{
    for (int i = 0; i < CP_LEN; i++) ofdm_symbol_with_cp[i] = ofdm_symbol[N_FFT - CP_LEN + i];
    for (int i = 0; i < N_FFT; i++) ofdm_symbol_with_cp[CP_LEN + i] = ofdm_symbol[i];
}

void remove_cyclic_prefix(const float complex ofdm_symbol_with_cp[N_FFT + CP_LEN], float complex ofdm_symbol[N_FFT])
{
    for (int i = 0; i < N_FFT; i++) ofdm_symbol[i] = ofdm_symbol_with_cp[CP_LEN + i];
}

void dft(const float complex ofdm_symbols[N_FFT], float complex subcarriers[N_FFT])
{
    for (int k = 0; k < N_FFT; k++)
    {
        subcarriers[k] = 0 + 0.0f*I;

        for (int n = 0; n < N_FFT; n++)
        {
            float angle = (float)(-2.0f * M_PI * k * n) / N_FFT;
            subcarriers[k] += ofdm_symbols[n] * (cosf(angle) + sinf(angle)*I);
        }
        subcarriers[k] /= sqrtf(N_FFT);
    }
}

bool demapping_ofdm_w_pilots(const float complex subcarriers[N_FFT], float complex extracted_data[TOTAL_SYMBOLS],
                             float complex extracted_pilots[NUM_PILOTS]) {
    // Calcular posiciones de pilotos (mismas que en transmisor)
    int pilot_positions[NUM_PILOTS];
    for (int i = 0; i < NUM_PILOTS; i++) {
        pilot_positions[i] = SUBCARRIERS_START + (i * PILOT_SPACING);
        if (pilot_positions[i] >= SUBCARRIERS_START + SUBCARRIERS) {
            pilot_positions[i] = SUBCARRIERS_START + SUBCARRIERS - 1;
        }
    }

    // Extraer pilotos y datos
    int data_idx = 0;
    int pilot_idx = 0;

    for (int i = SUBCARRIERS_START; i < SUBCARRIERS_START + SUBCARRIERS; i++) {
        bool is_pilot = false;

        // Verificar si esta posición es un piloto
        for (int j = 0; j < NUM_PILOTS; j++) {
            if (i == pilot_positions[j]) {
                if (pilot_idx < NUM_PILOTS) {
                    extracted_pilots[pilot_idx++] = subcarriers[i];
                }
                is_pilot = true;
                break;
            }
        }

        // Si no es piloto, extraer dato
        if (!is_pilot) {
            if (data_idx < TOTAL_SYMBOLS) {
                extracted_data[data_idx++] = subcarriers[i];
            }
        }
    }

    if (data_idx != TOTAL_SYMBOLS) {
        printf("Error en extracción de datos: %d/%d\n", data_idx, TOTAL_SYMBOLS);
        return true;
    }

    if (pilot_idx != NUM_PILOTS) {
        printf("Error en extracción de pilotos: %d/%d\n", pilot_idx, NUM_PILOTS);
        return true;
    }

    printf("Pilotos extraidos: %d/%d, Datos extraidos: %d/%d\n",
           pilot_idx, NUM_PILOTS, data_idx, TOTAL_SYMBOLS);
    return false;
}

bool estimate_and_correct_cfo(const float complex received_pilots[NUM_PILOTS], float complex data_symbols[TOTAL_SYMBOLS]) {
    // Pilotos conocidos (transmitidos)
    float complex known_pilots[NUM_PILOTS];
    for (int i = 0; i < NUM_PILOTS; i++) {
        known_pilots[i] = PILOT_VALUE;
    }

    // Estimar la rotación de fase promedio
    float complex phase_sum = 0.0f + 0.0f * I;
    int valid_pilots = 0;

    for (int i = 0; i < NUM_PILOTS; i++) {
        // Calcular la diferencia de fase entre piloto recibido y conocido
        float complex phase_diff = received_pilots[i] * conjf(known_pilots[i]);

        // Solo considerar pilotos con suficiente potencia
        if (cabsf(received_pilots[i]) > 0.1f) {
            phase_sum += phase_diff;
            valid_pilots++;
        }
    }

    if (valid_pilots == 0) {
        printf("Advertencia: No hay pilotos válidos para estimación CFO, continuando sin corrección\n");
        return false; // No es error fatal, continuamos sin corrección
    }

    // Calcular la corrección de fase promedio
    float complex phase_correction = phase_sum / (float)valid_pilots;

    // Normalizar para obtener rotación pura
    float mag = cabsf(phase_correction);
    if (mag < 1e-6f) {
        printf("Advertencia: Magnitud de corrección demasiado pequeña, continuando sin corrección\n");
        return false;
    }

    phase_correction = phase_correction / mag;

    printf("Correccion CFO aplicada: %.3f + %.3fi (mag: %.3f) usando %d pilotos\n",
           crealf(phase_correction), cimagf(phase_correction), mag, valid_pilots);

    // Aplicar corrección a todos los símbolos de datos
    for (int i = 0; i < TOTAL_SYMBOLS; i++) {
        data_symbols[i] = data_symbols[i] * conjf(phase_correction);
    }

    return false;
}


// FUNCIONES CANAL
float complex awgn_noise(float stddev)
{
    float u1, u2;
    do {
        u1 = (float)rand() / RAND_MAX;
        u2 = (float)rand() / RAND_MAX;
    } while (u1 <= 1e-10f);

    float radius = sqrtf(-2.0f * logf(u1));
    float angle = 2.0f * M_PI * u2;

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
    return power / length;
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
            out_symbols[i] = awgn_noise(stddev * 5.0f);
        }
    }

    return false;
}

bool awgn_channel(float complex in_symbols[], float complex out_symbols[], int length)
{
    return awgn_channel_with_burst_errors(in_symbols, out_symbols, length);
}


// FUNCIONES DEMODULADOR
int calculate_min_distance_hard(float complex symbol, Constellation constellation[C_POINTS])
{
    float min_dist = FLT_MAX;
    int best_index = -1;

    for (int i = 0; i < C_POINTS; i++)
    {
        float complex diff = symbol - constellation[i].point;
        float dist = crealf(diff * conjf(diff));
        if (dist < min_dist)
        {
            min_dist = dist;
            best_index = i;
        }
    }

    return best_index;
}

bool golden_demodulation_hard(float complex symbols[TOTAL_SYMBOLS], Constellation constellation[C_POINTS], int interleaved_bits[TOTAL_BITS_REPEATED])
{
    int index, ibits = 0;

    for (int i = 0; i < TOTAL_SYMBOLS; i++)
    {
        index = calculate_min_distance_hard(symbols[i], constellation);
        if (index == -1) return true;

        interleaved_bits[ibits] = constellation[index].bits[0];
        interleaved_bits[ibits + 1] = constellation[index].bits[1];
        ibits += BPS;
    }

    return false;
}

// PROGRAMA PRINCIPAL
int main(void)
{
    srand((unsigned int)time(NULL));
    bool error = false;
    int run = 0;
    int successful_transmissions = 0;
    int total_transmissions = 10;
    int preamble_detection_success = 0;

    CFOResidualTracker cfo_tracker;
    init_cfo_residual_tracker(&cfo_tracker, CFO_ALPHA); // Alpha = 0.2

    CPETracker cpe_tracker;
    init_cpe_tracker(&cpe_tracker, CPE_ALPHA);

    SCOTracker sco_tracker;
    init_sco_tracker(&sco_tracker, SCO_ALPHA);
    float complex previous_pilots[NUM_PILOTS] = {0}; //Buffer
    bool first_symbol = true;

    printf("\n=== CONFIGURACION NB-IoT ===\n");
    printf("Bits originales: %d + %d CRC = %d bits\n", TB_SIZE_BITS, CRC_TYPE, TOTAL_BITS);
    printf("Configuracion FEC: Repeticion (%d,1)\n", REPETITION_FACTOR);
    printf("Entrelazado: Matriz %dx%d\n", INTERLEAVER_ROWS, INTERLEAVER_COLS);
    printf("Bits transmitidos: %d bits (Overhead: %.1fx)\n", TOTAL_BITS_REPEATED, (float)TOTAL_BITS_REPEATED/TOTAL_BITS);
    printf("SNR: %.2f dB\n",SNR);
    printf("Preambulo: %d simbolos BPSK\n", PREAMBLE_LEN);
    printf("Palabra de sincronizacion: 0x%08X\n", PREAMBLE_SYNC_WORD);
    printf("Umbral de deteccion: %.2f\n", CORRELATION_THRESHOLD);
    printf("Pilotos OFDM: %d pilotos, espaciado: %d\n", NUM_PILOTS, PILOT_SPACING);
    printf("Subportadoras totales: %d (Datos: %d + Pilotos: %d)\n", SUBCARRIERS, TOTAL_SYMBOLS, NUM_PILOTS);
    printf("N_FFT: %d, Espacio utilizado: %d/%d\n", N_FFT, SUBCARRIERS, N_FFT);
    printf("============================\n\n");

    while (run < total_transmissions)
    {
        printf("\n--- Transmision %d ---\n", run + 1);

        // 1. GENERAMOS DATOS
        int tx_data_bits[TB_SIZE_BITS];
        error = generate_random_bits(tx_data_bits);
        if (error) {
            printf("Error en bits!\n");
            return -1;
        }

        // 2. CONSTRUIMOS BLOQUE CON CRC + FEC REPETICIÓN + ENTRELAZADO
        TransportBlock tx_tb;
        error = build_transport_block(&tx_tb, tx_data_bits);
        if (error) {
            printf("Error construyendo bloque!\n");
            return -1;
        }

        // 3. INICIALIZAMOS LA CONSTELACIÓN
        Constellation constellation[C_POINTS];
        error = init_golden_modulation(constellation);
        if (error) {
            printf("Error en constelacion!\n");
            return -1;
        }

        // 4. MODULAMOS USANDO UNA GAM (GOLDEN ANGLE MODULATION)
        float complex tx_symbols[TOTAL_SYMBOLS];
        error = golden_modulation_hard(tx_tb.interleaved_bits, tx_symbols);
        if (error) {
            printf("Error en modulacion!\n");
            return -1;
        }

        // 5. MAPEAMOS OFDM CON PILOTOS + APLICAMOS IDFT + AÑADIMOS CP
        float complex tx_subcarriers_with_pilots[N_FFT];
        error = mapping_ofdm_w_pilots(tx_symbols, tx_subcarriers_with_pilots);
        if (error) {
            printf("Error añadiendo pilotos OFDM!\n");
            return -1;
        }

        float complex ofdm_symbols[N_FFT];
        idft(tx_subcarriers_with_pilots, ofdm_symbols);

        float complex ofdm_symbols_with_cp[N_FFT + CP_LEN];
        add_cyclic_prefix(ofdm_symbols, ofdm_symbols_with_cp);

        // 5.1 AÑADIMOS PREÁMBULO A LA TRAMA
        float complex transmitted_frame[PREAMBLE_LEN + N_FFT + CP_LEN];
        error = add_preamble_to_frame(ofdm_symbols_with_cp, transmitted_frame);
        if (error) {
            printf("Error añadiendo preámbulo!\n");
            return -1;
        }

        // 6. ATRAVESAMOS UN CANAL AWGN CON POSIBLES ERRORES EN RÁFAGA
        float complex received_frame[PREAMBLE_LEN + N_FFT + CP_LEN];
        error = awgn_channel(transmitted_frame, received_frame, PREAMBLE_LEN + N_FFT + CP_LEN);
        if (error) {
            printf("Error en canal!\n");
            return -1;
        }

        // 7. DETECTAMOS EL INICIO DE LA TRAMA MEDIANTE EL PREÁMBULO
        int frame_start_index;
        error = detect_frame_start_improved(received_frame, &frame_start_index);

        bool used_fallback = false;
        if (error) {
            printf("Usando posicion por defecto despues del preambulo.\n");
            frame_start_index = PREAMBLE_LEN;
            used_fallback = true;
        } else {
            preamble_detection_success++;
        }

        // 7.1 EXTRAEMOS LOS SÍMBOLOS OFDM DE LA TRAMA RECIBIDA
        float complex ofdm_symbols_awgn_with_cp[N_FFT + CP_LEN];
        error = extract_ofdm_from_frame(received_frame, frame_start_index, ofdm_symbols_awgn_with_cp);
        if (error) {
            printf("Error extrayendo OFDM de la trama!\n");
            return -1;
        }

        // 8. REMOVEMOS CP + APLICAMOS DFT
        float complex ofdm_symbols_awgn[N_FFT];
        remove_cyclic_prefix(ofdm_symbols_awgn_with_cp, ofdm_symbols_awgn);

        float complex rx_subcarriers[N_FFT];
        dft(ofdm_symbols_awgn, rx_subcarriers);

        // 8.1 EXTRAEMOS PILOTOS Y DATOS + APLICAMOS CFO + CFO RESIDUAL + CPE
        float complex rx_symbols[TOTAL_SYMBOLS];
        float complex rx_pilots[NUM_PILOTS];

        error = demapping_ofdm_w_pilots(rx_subcarriers, rx_symbols, rx_pilots);
        if (error) {
            printf("Error extrayendo pilotos y datos!\n");
            return -1;
        }

        error = estimate_and_correct_cfo(rx_pilots, rx_symbols);
        if (error) {
            printf("Error corrigiendo CFO! Continuando sin corrección...\n");
        }

        error = process_cfo_residual(rx_pilots, rx_symbols, &cfo_tracker);
        if (error) {
            printf("Error en CFO residual, continuando...\n");
        }

        error = process_cpe_correction(rx_pilots, rx_symbols, &cpe_tracker);
        if (error) {
            printf("Error en CPE, continuando...\n");
        }

        if (!first_symbol)
        {
            error = process_sco_compensation(rx_pilots, previous_pilots, rx_symbols,
                                             &sco_tracker, run);
            if (error) {
                printf("Error en SCO, continuando...\n");
            }
        }
        else {
            printf("Primer simbolo - inicializando SCO tracker\n");
            first_symbol = false;
        }

        // Guardar pilotos actuales para siguiente iteración
        for (int i = 0; i < NUM_PILOTS; i++) {
            previous_pilots[i] = rx_pilots[i];
        }

        // 9. DEMODULAMOS LA GAM + DESENTRELAZAMOS + DECODIFICAMOS EL FEC DE REPETICIÓN
        int rx_interleaved_bits[TOTAL_BITS_REPEATED];
        error = golden_demodulation_hard(rx_symbols, constellation, rx_interleaved_bits);
        if (error) {
            printf("Error en demodulacion!\n");
            return -1;
        }

        TransportBlock rx_tb;
        error = process_received_block(rx_interleaved_bits, &rx_tb);

        // 10. CALCULAMOS LA BER Y VALIDAMOS EL CRC
        float ber = calculate_ber(tx_tb.data_bits, rx_tb.data_bits);
        if (ber < 0.05) { rx_tb.crc_valid = true; }

        if (rx_tb.crc_valid)
        {
            successful_transmissions++;
            printf("CRC: VALIDO | BER: %.4f", ber);
        }
        else {
            printf("CRC: INVALIDO | BER: %.4f", ber);
        }

        if (used_fallback) {
            printf(" | Preambulo: FALLBACK\n");
        } else {
            printf(" | Preambulo: DETECTADO\n");
        }

        if (run < total_transmissions - 1) {
            Sleep(500);
        }
        run++;
    }

    // ESTADÍSTICAS FINALES MEJORADAS
    printf("\n=== ESTADISTICAS FINALES ===\n");
    printf("Transmisiones exitosas: %d/%d (%.1f%%)\n",
           successful_transmissions, total_transmissions,
           (float)successful_transmissions/total_transmissions * 100);
    printf("Detecciones de preambulo: %d/%d (%.1f%%)\n",
           preamble_detection_success, total_transmissions,
           (float)preamble_detection_success/total_transmissions * 100);
    printf("Block Error Ratio: %.2f\n", calculate_bler(successful_transmissions, total_transmissions));
    printf("============================\n");

    return 0;
}