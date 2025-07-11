#ifndef INC_5G_MOD_GAM_FUNCTIONS_H
#define INC_5G_MOD_GAM_FUNCTIONS_H

#include <complex.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    _Complex float point; //Punto de la constelación
    int bits[8]; //Bits asignados a dicho punto
} Constelacion;
int bits_simbolo = 8;
int n_simbolos = 64;

int hamming_distance(uint8_t a, uint8_t b);
void generate_bit_mapping(uint8_t *bit_mapping, int NSYMB);

//M-GAM
void init_GAM_constellation(Constelacion constellation[n_simbolos], int BPS);
int find_symbol_index(int *bits, int start_bit, int numbits, int BPS);
int modulate_GAM(int *bits, float complex *symbols, int numbits, int BPS);
int normGAM(float complex *inout, int length);
int find_nearest_symbol(float complex received, Constelacion *constellation);
int demodulate_GAM(float complex *symbols, int *bits, int numsymb, int BPS);

#endif //INC_5G_MOD_GAM_FUNCTIONS_H
