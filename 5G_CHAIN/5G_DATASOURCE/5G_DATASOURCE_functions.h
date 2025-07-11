#ifndef INC_5G_DATASOURCE_FUNCTIONS_H
#define INC_5G_DATASOURCE_FUNCTIONS_H

#include <complex.h>

// Generador de bits aleatorios
void generate_random_bits(int *bits, int length);

// Cálculo de BER
float calculate_ber(int *tx_bits, int *rx_bits, int length);

#endif //INC_5G_DATASOURCE_FUNCTIONS_H
