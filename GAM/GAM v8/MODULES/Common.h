#ifndef GAM_COMMON_H
#define GAM_COMMON_H

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


// PARÁMETROS GLOBALES
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
#define BPS 4                     // 2 bits por símbolo (QPSK-like)
#define TOTAL_SYMBOLS (TOTAL_BITS_REPEATED / BPS) // 108 símbolos de datos


// PARÁMETROS PILOTOS EN PRB
#define PILOTS_PER_SYMBOL 4       // 4 pilotos por símbolo OFDM
#define TOTAL_PILOTS (PRB_SYMBOLS * PILOTS_PER_SYMBOL) // 28 pilotos totales
#define DATA_RE_PER_PRB (RESOURCE_ELEMENTS_PER_PRB - TOTAL_PILOTS) // 112 REs para datos


// PARÁMETROS ENTRELAZADO
#define INTERLEAVER_ROWS 18     //18
#define INTERLEAVER_COLS 12    //12
#define INTERLEAVER_SIZE (INTERLEAVER_ROWS * INTERLEAVER_COLS) // 216 bits


// PARÁMETROS MODULACIÓN
#define ANGLE_STEP 2.3997569
#define C_POINTS (1 << BPS)


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


// ESTRUCTURAS GLOBALES
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

typedef struct {
    float complex data_symbols[PRB_SYMBOLS][PRB_SUBCARRIERS];
    float complex pilot_symbols[PRB_SYMBOLS][PILOTS_PER_SYMBOL];
    int pilot_positions[PILOTS_PER_SYMBOL];
    bool initialized;
} PRB_Grid;

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


#endif //GAM_COMMON_H
