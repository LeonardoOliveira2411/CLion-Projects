#ifndef ENLACE_WDM_FUNCTIONS_H
#define ENLACE_WDM_FUNCTIONS_H
#define FFTW_DLL_IMPORT

#include <stdlib.h>
#include <math.h>
#include <tgmath.h>

#define NUM_CHANNELS 2
#define NUM_BITS 128
#define NUM_SYMBOLS (NUM_BITS/2) //PARA 4QAM
#define SAMPLE_RATE 1e9 // Fs en Hz
#define CHANNEL_SPACING 0.5e9 // DeltaFreq en Hz

void gen_random_bits (int *bits, int len)
{
    for (int i = 0; i < len; i++ )
    {
        bits[i] = rand() % 2;
    }
}

void gen_bpsk_signal (int *bits, float *signal, int len)
{
    for (int i = 0; i < len; i++ )
    {
        signal[i] = (bits[i] == 1) ? 1.0 : -1.0;
    }
}

void gen_4QAM_signal(int *bits, float complex *signal)
{
    // Verificar que el número de bits es múltiplo de 2
    if (NUM_BITS % 2 != 0)
    {
        fprintf(stderr, "Error: El número de bits debe ser múltiplo de 4 para 16QAM\n");
        return;
    }

    int numsymb = 0;
    for (int i = 0; i < NUM_BITS; i += 2)
    {
        int bit1 = bits[i];
        int bit2 = bits[i+1];

        // Mapeo Gray para 4QAM:
        // 00 -> -1-1j 01 -> -1+1j
        // 11 -> +1+1j 10 -> +1-1j
        float InPhase = (bit1 == 0) ? -1.0 : 1.0;
        float Quadrature = (bit2 == 0) ? -1.0 : 1.0;

        signal[numsymb] = InPhase + Quadrature * I;
        numsymb++;
    }
}

void wdm_mux(float complex tx_signal[NUM_CHANNELS][NUM_SYMBOLS], float *wdm_output)
{
    // Frecuencia central del primer canal
    //float base_freq = (NUM_CHANNELS - 1) * CHANNEL_SPACING / 2.0f;
    float base_freq = 192.8e6; //Teniendo en cuenta c/lambda (1550 nm)
    
    // Inicializar la salida a cero
    for (int i = 0; i < NUM_SYMBOLS; i++)
    {
        wdm_output[i] = 0.0f;
    }
    
    // Para cada canal
    for (int ch = 0; ch < NUM_CHANNELS; ch++)
    {
        float carrier_freq = base_freq - ch * CHANNEL_SPACING;
        
        // Generar y sumar la señal modulada
        for (int i = 0; i < NUM_SYMBOLS; i++)
        {
            float t = (float)i * 2/ SAMPLE_RATE;  // Tiempo en segundos

            wdm_output[2*i] += crealf(tx_signal[ch][i]) * cos(2 * M_PI * carrier_freq * t);
            wdm_output[2*i] -= cimagf(tx_signal[ch][i]) * sin(2 * M_PI * carrier_freq * t);

            // Segunda muestra del símbolo (sincronización)
            wdm_output[2*i+1] += crealf(tx_signal[ch][i]) * cos(2 * M_PI * carrier_freq * (t + 1/SAMPLE_RATE));
            wdm_output[2*i+1] -= cimagf(tx_signal[ch][i]) * sin(2 * M_PI * carrier_freq * (t + 1/SAMPLE_RATE));
        }
    }
}

void wdm_demux(float *wdm_input, float complex rx_signals[NUM_CHANNELS][NUM_SYMBOLS])
{
    // Frecuencia central del primer canal
    //float base_freq = (NUM_CHANNELS - 1) * CHANNEL_SPACING / 2.0f;
    float base_freq = 192.8e6; //Calcular según los canales en el sistema - (1550 nm)

    // Para cada canal
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        float carrier_freq = base_freq - ch * CHANNEL_SPACING;

        // Demodulación coherente (producto con portadora)
        for (int i = 0; i < NUM_SYMBOLS; i++)
        {
            float t = (float)i * 2 / SAMPLE_RATE;
            float I_component = 0.0f;
            float Q_component = 0.0f;

            for (int s = 0; s < 2; s++)
            {
                float current_t = t + (float)s/SAMPLE_RATE;
                I_component += wdm_input[2*i + s] * cos(2 * M_PI * carrier_freq * current_t);
                Q_component -= wdm_input[2*i + s] * sin(2 * M_PI * carrier_freq * current_t);
            }

            rx_signals[ch][i] = (I_component / 2.0f) + (Q_component / 2.0f)*I;
        }

        // Filtrado pasa-bajos (promedio móvil)
        for (int i = 1; i < NUM_BITS-1; i++) {
            rx_signals[ch][i] = (rx_signals[ch][i-1] + rx_signals[ch][i] + rx_signals[ch][i+1]) / 3.0f;
        }
    }
}

void demod_bpsk_signal(float *signal, int *bits, int len, float threshold)
{
    for (int i = 0; i < len; i++) {
        bits[i] = (signal[i] > threshold) ? 1 : 0;
    }
}

void demod_4QAM_signal(float complex *signal, int *bits)
{
    for (int i = 0; i < NUM_SYMBOLS; i++)
    {
        float InPhase = creal(signal[i]);
        float Quadrature = cimag(signal[i]);

        // Mapeo Gray para 4QAM:
        // 00 -> -1-1j 01 -> -1+1j
        // 11 -> +1+1j 10 -> +1-1j
        bits[2*i] = (InPhase > 0) ? 1 : 0;
        bits[2*i + 1] = (Quadrature > 0) ? 1 : 0;
    }
}

void random_bits_sink(int *tx_bits, int *rx_bits, int len, float *ber)
{
    int error_count = 0;
    
    for (int i = 0; i < len; i++) {
        if (tx_bits[i] != rx_bits[i]) {
            error_count++;
        }
    }
    
    *ber = (float)error_count / len;
}

#endif //ENLACE_WDM_FUNCTIONS_H
