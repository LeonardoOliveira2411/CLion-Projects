#include "Frame.h"

// PREÁMBULO Y DETECCIÓN
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


// FORMACIÓN DE TRAMA
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