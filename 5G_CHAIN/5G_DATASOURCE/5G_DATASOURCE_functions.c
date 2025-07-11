#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <complex.h>

// Generador de bits aleatorios
void generate_random_bits(int *bits, int numbits)
{
    for (int i = 0; i < numbits; i++)
    {
        bits[i] = rand() % 2;
    }
}

// Cálculo de BER (Bit Error Rate)
float calculate_ber(int *tx_bits, int *rx_bits, int numbits)
{
    int errors = 0;
    for (int i = 0; i < numbits; i++)
    {
        if (tx_bits[i] != rx_bits[i]) {
            errors++;
        }
    }
    return (float)errors / numbits;
}