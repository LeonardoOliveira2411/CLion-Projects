//DECLARAMOS LAS LIBRERÍAS NECESARIAS
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <complex.h>
#include <math.h>
#include <float.h>


//VARIABLES GENERALES
#define NUM_BITS 64
#define BPS 2
#define NUM_SYMBOLS (NUM_BITS/BPS)
#define C_POINTS (int)pow(BPS, 2)
#define SNR 30.0


//VARIABLES OFDM
#define N_FFT (2*NUM_SYMBOLS)
#define DATA_SUBCARRIERS 0.75*N_FFT
#define DC_SUBCARRIER (N_FFT/2)


//VARIABLES MODULACIÓN
#define ANGLE_STEP 2.3997569 //137.5º


//ESTRUCTURA PARA LA CONSTELACIÓN GAM
typedef struct
{
    float complex point; //Punto de la constelación
    int bits[BPS]; //Bits asignados a dicho punto
} Constellation;


//FUNCIONES DATASOURCE
bool generate_random_bits(int bits[NUM_BITS])
{
    for (int i = 0; i < NUM_BITS; i++)
    {   bits[i] = rand() % 2 ? 1 : 0;   }

    for (int i = 0; i < NUM_BITS; i++)
    {   if ( bits[i] > 1) return true;    }

    return false;
}

float calculate_ber(const int tx_bits[NUM_BITS], const int rx_bits[NUM_BITS])
{
    int errors = 0;
    float ber;

    for (int i = 0; i < NUM_BITS; i ++)
    {
        //if (tx_bits[i] != rx_bits[i]) {     errors++;    }
        if (tx_bits[i] != rx_bits[i]) {     errors++;    }
    }

    if (errors == 0) {  ber = 0;  }
    else {  ber = (float)errors / NUM_BITS;   }

    return ber;
}


//FUNCIONES MODULADOR
bool init_golden_modulation(Constellation constellation[C_POINTS])
{
    int ibits = 0;

    for (int i = 0; i < C_POINTS; i++)
    {
        float angle = (float)(i * ANGLE_STEP);
        float radius = (float)sqrt(i);
        constellation[i].point = radius * (cosf(angle) + sinf(angle)*I);

        if (i == 0) {   constellation[i].bits[ibits] = 0; constellation[i].bits[ibits + 1] = 0;     }
        else if (i == 1) {   constellation[i].bits[ibits] = 0; constellation[i].bits[ibits + 1] = 1;     }
        else if (i == 2) {   constellation[i].bits[ibits] = 1; constellation[i].bits[ibits + 1] = 0;     }
        else if (i == 3) {   constellation[i].bits[ibits] = 1; constellation[i].bits[ibits + 1] = 1;     }
        else return true;
        //ibits += BPS;
    }

    return false;
}

bool golden_modulation(const int bits[NUM_BITS], float complex symbols[NUM_SYMBOLS])
{
    int num;
    int ibits = 0;

    for (int i = 0; i < NUM_SYMBOLS; i++)
    {
        if (bits[ibits] == 0 && bits[ibits + 1] == 0) { num = 0; }
        else if (bits[ibits] == 0 && bits[ibits + 1] == 1) { num = 1; }
        else if (bits[ibits] == 1 && bits[ibits + 1] == 0) { num = 2; }
        else if (bits[ibits] == 1 && bits[ibits + 1] == 1) { num = 3; }
        else return true;

        float angle = (float)(num * ANGLE_STEP);
        float radius = (float)sqrt(num);
        symbols[i] = radius * (cosf(angle) + sinf(angle)*I);
        ibits += BPS;
    }

    return false;
}

//FUNCIONES MAPPING OFDM
bool mapping_ofdm (const float complex tx_symbols[NUM_SYMBOLS], float complex subcarriers[N_FFT])
{
    int null_subcarriers = 0, start = 0, end = 0, data_idx = 0;

    if (NUM_SYMBOLS > DATA_SUBCARRIERS) {     return true;     }

    for (int i = 0; i < N_FFT; i++) {   subcarriers[i] = 0.0 + 0.0*I;  }

    null_subcarriers = N_FFT - DATA_SUBCARRIERS;
    start = null_subcarriers / 2;
    end = N_FFT - (null_subcarriers/2);

    for (int i = start; i < end && data_idx < NUM_SYMBOLS; i++)
    {
       if (i == DC_SUBCARRIER) {    continue;   }

       subcarriers[i] = tx_symbols[data_idx];
       data_idx++;
    }

    return false;
}

void idft (const float complex subcarriers[N_FFT], float complex ofdm_symbols[N_FFT])
{
    float angle = 0;
    float complex twiddle = 0.0 + 0.0*I;

    for (int n = 0; n < N_FFT; n++)
    {
        ofdm_symbols[n] = 0 + 0.0*I;

        for (int k = 0; k < N_FFT; k++)
        {
            angle = (float)(2.0 * M_PI * k * n) / N_FFT;
            twiddle = cosf(angle) + sinf(angle)*I;

            ofdm_symbols[n] += subcarriers[k] * twiddle;
        }

        ofdm_symbols[n] /= N_FFT;
    }
}

void dft (const float complex ofdm_symbols[N_FFT], float complex subcarriers[N_FFT])
{
    float angle = 0;
    float complex twiddle = 0.0 + 0.0*I;

    for (int k = 0; k < N_FFT; k++)
    {
        subcarriers[k] = 0 + 0.0*I;

        for (int n = 0; n < N_FFT; n++)
        {
            angle = (float)(-2.0 * M_PI * k * n) / N_FFT;
            twiddle = cosf(angle) + sinf(angle)*I;

            subcarriers[k] += ofdm_symbols[n] * twiddle;
        }
    }
}

bool demapping_ofdm (const float complex subcarriers[N_FFT], float complex rx_symbols[NUM_SYMBOLS])
{
    int null_subcarriers = 0, start = 0, end = 0, data_idx = 0;

    if (NUM_SYMBOLS > DATA_SUBCARRIERS) {     return true;     }

    for (int i = 0; i < NUM_SYMBOLS; i++) {   rx_symbols[i] = 0.0 + 0.0*I;  }

    null_subcarriers = N_FFT - DATA_SUBCARRIERS;
    start = null_subcarriers / 2;
    end = N_FFT - (null_subcarriers/2);

    for (int i = start; i < end && data_idx < NUM_SYMBOLS; i++)
    {
        if (i == DC_SUBCARRIER) {    continue;   }

        rx_symbols[data_idx] = subcarriers[i];
        data_idx++;
    }

    return false;
}


//FUNCIONES CANAL SIMULADO
float complex awgn_noise (float stddev)
{
    float u1 = 0, u2 = 0, radius = 0, angle = 0;
    float noise_i = 0, noise_q = 0;

    while (u1 <= 1e-10)
    {
        u1 = (float)(rand() / (RAND_MAX + 1.0));
        u2 = (float)(rand() / (RAND_MAX + 1.0));
    }

    radius = (float)sqrt(-2.0*log((double)u1));
    angle = (float)(2.0 * M_PI * u2);

    noise_i = radius * cosf(angle) * stddev;
    noise_q = radius * sinf(angle) * stddev;

    return noise_i + I*noise_q;
}

float calculate_mean_power (const float complex symbols[N_FFT])
{
    float power = 0, mean_power = 0;

    for (int i = 0; i < N_FFT; i++)
    {
        power += crealf(symbols[i]) * crealf(symbols[i]) + cimagf(symbols[i]) * cimagf(symbols[i]);
    }

    mean_power = power / N_FFT;

    return mean_power;
}

bool awgn_channel(float complex in_symbols[N_FFT], float complex out_symbols[N_FFT])
{
    float mean_power = 0, SNR_lin = 0, noise_power = 0, stddev = 0;
    float complex noise;

    mean_power = calculate_mean_power(in_symbols);
    if (mean_power == 0) {      return true;    }

    SNR_lin = (float)pow(10.0, SNR/10.0);
    noise_power = mean_power / SNR_lin;

    stddev = (float)sqrt(noise_power / 2.0);

    for (int i = 0; i < N_FFT; i++)
    {
        noise = awgn_noise(stddev);
        out_symbols[i] = in_symbols[i] + noise;
    }

    return false;
}


//FUNCIONES DEMODULADOR
int calculate_distances(float complex symbol, Constellation constellation[C_POINTS])
{
    float min_dist = FLT_MAX;
    int best_index = -1;

    for (int i = 0; i < C_POINTS; i++)
    {
        //printf("Constellation Real[%d] = %f\n", i, crealf(constellation[i].point));
        float complex complex_dist = symbol - constellation[i].point;
        float dist = crealf(complex_dist * conjf(complex_dist));

        if (dist < min_dist)
        {
            min_dist = dist;
            best_index = i;
        }
    }

    return best_index;
}

bool golden_demodulation(float complex symbols[NUM_SYMBOLS], Constellation constellation[C_POINTS], int bits[NUM_BITS])
{
    int index;
    int ibits = 0;

    for (int i = 0; i < NUM_SYMBOLS; i++)
    {
        //printf("Symbol Real[%d] = %f\n", i, crealf(symbols[i]));
        index = calculate_distances(symbols[i], constellation);
        if (index == -1) {      return true;    }

        //printf("Constellation Bits[%d] = %d%d\n", index, constellation[index].bits[0], constellation[index].bits[1]);
        bits[ibits] = constellation[index].bits[0];
        bits[ibits + 1] = constellation[index].bits[1];
        ibits += BPS;
    }

    return false;
}


//PROGRAMA PRINCIPAL - MAIN
int main(void)
{
    bool error = false; //Variable para comprobaciones de error

    //GENERAMOS BITS ALEATORIOS
    int tx_bits[NUM_BITS];
    error = generate_random_bits(tx_bits);
    if (error) {    printf("Error en la generación de bits aleatorios!!\n");   }
    //else {  for (int i = 0; i < NUM_BITS; i++) {    printf("TX Bit[%d] = %d\n", i, tx_bits[i]);   }   }


    //GENERAMOS CONSTELACIÓN
    Constellation constellation[C_POINTS];
    error = init_golden_modulation(constellation);
    if (error) {    printf("Error en la generación de la constelación Golden!!\n");   }
    //else {  for (int i = 0; i < C_POINTS; i++) {    printf("Constellation Real[%d] = %f\n", i, crealf(constellation[i].point));   }   }


    //GENERAMOS SÍMBOLOS GAM A TX
    float complex tx_symbols[NUM_SYMBOLS];
    error = golden_modulation(tx_bits, tx_symbols);
    if (error) {    printf("Error en la modulación Golden!!\n");   }
    //else {  for (int i = 0; i < NUM_SYMBOLS; i++) {    printf("TX Symbol Real[%d] = %f\n", i, crealf(tx_symbols[i]));   }   }


    //APLICAMOS UN MAPEADO OFDM
    float complex tx_subcarriers[N_FFT];
    error = mapping_ofdm(tx_symbols, tx_subcarriers);
    if (error) {    printf("Número de símbolos a TX mayor que número de subportadoras de datos!!\n");   }
    //else {  for (int i = 0; i < N_FFT; i++) {    printf("TX Subcarriers Real[%d] = %f\n", i, crealf(tx_subcarriers[i]));   }   }

    float complex ofdm_symbols[N_FFT];
    idft(tx_subcarriers, ofdm_symbols);


    //SIMULAMOS UN CANAL AWGN
    float complex ofdm_symbols_awgn[N_FFT];
    error = awgn_channel(ofdm_symbols, ofdm_symbols_awgn);
    if (error) {    printf("Error en la transmisión a través del canal AWGN!!\n");   }
    //else {  for (int i = 0; i < NUM_SYMBOLS; i++) {    printf("RX Symbol Real[%d] = %f\n", i, crealf(rx_symbols[i]));   }   }


    //APLICAMOS UN DEMAPEADO OFDM
    float complex rx_subcarriers[N_FFT];
    dft(ofdm_symbols_awgn, rx_subcarriers);

    float complex rx_symbols[NUM_SYMBOLS];
    error = demapping_ofdm(rx_subcarriers, rx_symbols);
    if (error) {    printf("Número de símbolos a RX mayor que número de subportadoras de datos!!\n");   }
    //else {  for (int i = 0; i < NUM_SYMBOLS; i++) {    printf("RX Symbol Real[%d] = %f\n", i, crealf(rx_symbols[i]));   }   }


    //DEMODULAMOS SÍMBOLOS A BITS RX
    int rx_bits[NUM_BITS];
    error = golden_demodulation(rx_symbols, constellation, rx_bits);
    if (error) {    printf("Error en la demodulación Golden!!\n");   }
    //else {  for (int i = 0; i < NUM_BITS; i++) {    printf("RX Bit[%d] = %d\n", i, rx_bits[i]);   }   }


    //CALCULAMOS BER DE LA TX/RX
    float ber;
    ber = calculate_ber(tx_bits, rx_bits);
    if (ber == 0) {    printf("Demodulacion perfecta: BER = %f!!\n", ber);   }
    else if (ber < 0.1) {    printf("Demodulacion correcta: BER = %f!!\n", ber);   }
    else {    printf("Demodulacion incorrecta: BER = %f!!\n", ber);   }

    return 0;
}