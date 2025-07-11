#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>
#include <complex.h>
#include <float.h>
#define GAM_ANGLE_STEP 2.399963229728653f  // 2*π*(2-φ), φ = razón áurea
#include "5G_GAM_MAPPING.h"

//ESTRUCTURA PARA CADA PUNTO DE LA CONSTELACIÓN PARA PODER REALIZAR LA RECEPCIÓN DE LOS SÍMBOLOS CALCULANDO EL LLR
int BITS_PER_SYMBOL = 6;
int N_SYMBOLS = 64;

//GAM Constellation Structure
typedef struct
{
    float complex point; //Punto de la constelación
    int bits[6]; //Bits asignados a dicho punto
} Constellation;

//GAM Constellation Initiation
void init_GAM_constellation(Constellation constellation[N_SYMBOLS], int BPS)
{
    // Verificar que BPS esté en un rango válido
    if (BPS < 2 || BPS > 6) {
        fprintf(stderr, "Error: bits_per_symbol debe ser 2, 3 o 4\n");
        return;
    }

    // Determinar el mapeo correcto según BPS
    const uint8_t *bit_mapping;
    int mapping_size;

    switch(BPS) {
        case 2:
            bit_mapping = BIT_MAPPING_4;
            mapping_size = 4;
            break;
        case 3:
            bit_mapping = BIT_MAPPING_8;
            mapping_size = 8;
            break;
        case 4:
            bit_mapping = BIT_MAPPING_16;
            mapping_size = 16;
            break;
        case 5:
            bit_mapping = BIT_MAPPING_32;
            mapping_size = 32;
            break;
        case 6:
            bit_mapping = BIT_MAPPING_64;
            mapping_size = 64;
            break;
        default:
            return; // Ya verificamos arriba, pero por seguridad
    }

    for (int i = 0; i < N_SYMBOLS; i++) {
        // 1. Generar punto en la espiral GAM
        float angle = (i + 1) * GAM_ANGLE_STEP;
        float radius = sqrtf(i + 1);
        constellation[i].point = radius * (cosf(angle) + I * sinf(angle));

        // 2. Asignar bits desde el mapeo correspondiente
        uint8_t mapped_bits = bit_mapping[i % mapping_size];

        for (int b = 0; b < BPS; b++) {  // Usar BPS en lugar de BITS_PER_SYMBOL
            constellation[i].bits[b] = (mapped_bits >> (BPS - 1 - b)) & 0x1;
        }
    }
}

int find_symbol_index(int *bits, int start_bit, int numbits, int BPS) {
    // Verificar que BPS esté en un rango válido
    if (BPS < 2 || BPS > 4) {
        fprintf(stderr, "Error: bits_per_symbol debe ser 2, 3 o 4\n");
        return -1;
    }

    // Determinar el mapeo correcto según BPS
    const uint8_t *bit_mapping;
    int mapping_size;

    switch(BPS) {
        case 2:
            bit_mapping = BIT_MAPPING_4;
            mapping_size = 4;
            break;
        case 3:
            bit_mapping = BIT_MAPPING_8;
            mapping_size = 8;
            break;
        case 4:
            bit_mapping = BIT_MAPPING_16;
            mapping_size = 16;
            break;

        default:
            return -1;
    }

    // Leer BPS bits desde bits[start_bit]
    uint8_t read_bits = 0;
    for (int k = 0; k < BPS && (start_bit + k) < numbits; k++)
    {
        read_bits = (read_bits << 1) | (bits[start_bit + k] == 1 ? 1 : 0);
    }

    // Buscar el índice en BIT_MAPPING que coincida con read_bits
    for (int i = 0; i < mapping_size; i++)
    {
        if (bit_mapping[i] == read_bits) { return i; }
    }

    return -1; // No encontrado
}

// GAM Modulation
int modulate_GAM(int *bits, float complex *symbols, int numbits, int BPS)
{
    // Verificar que el número de bits es múltiplo de 6
    if (numbits % BPS != 0)
    {
        fprintf(stderr, "Error: El número de bits debe ser múltiplo de %d para 64QAM\n", BPS);
        return -1;
    }
    // Verificar que el número de bits por símbolo
    if (BPS < 2 || BPS > 4)
    {
        fprintf(stderr, "Error: El número de bits por símbolo debe ser 2, 3, 4, 5 o 6\n");
        return -1;
    }

	int i, j= 0;
	BITS_PER_SYMBOL = BPS;
	N_SYMBOLS = pow(2, BPS);
    Constellation constellation[N_SYMBOLS];
    init_GAM_constellation(constellation, BPS);

	if(numbits == 0) return(0);

	for(i = 0; i < numbits; i += BITS_PER_SYMBOL)
	{
	    int symbol_index = find_symbol_index(bits, i, numbits, BITS_PER_SYMBOL);
        symbols[j] = constellation[symbol_index].point;
		//printf("Index: %d, SymbolsR [%d]: %f, SymbolsI [%d]: %f \n", symbol_index, j, crealf(symbols[j]), j, cimagf(symbols[j]));
		j++;
	}

	return j;
}

//GAM Normalization
int normGAM(float complex *inout, int length)
{

    int i;
    float auxR, auxI, avg_power = 0.0;
    static float ratio = 0.0;

    for(i = 0; i < length; i++)
    {
        auxR = fabs(creal(inout[i]));
        auxI = fabs(cimag(inout[i]));
        avg_power += (auxR * auxR) + (auxI * auxI);
    }

    avg_power = avg_power / (length + 0.00000001);  // Evitar división por cero
    ratio = 1 / (sqrtf(avg_power) + 0.00000001);

    for(i = 0; i < length; i++)
    {
        inout[i] = inout[i] * ratio;
    }

    return (1);
}

// Función auxiliar: Encuentra el índice del símbolo más cercano en la constelación
int find_nearest_symbol(float complex received, Constellation *constellation)
{
    int best_index = 0;
    float min_distance = FLT_MAX;

    for (int i = 0; i < N_SYMBOLS; i++)
    {
        float complex diff = received - constellation[i].point;
        float distance = crealf(diff * conjf(diff)); // |diff|^2 = (I² + Q²)

        if (distance < min_distance)
        {
            min_distance = distance;
            best_index = i;
        }
    }

    return best_index;
}

// GAM Demodulation
int demodulate_GAM(float complex *symbols, int *bits, int numsymb, int BPS)
{
    BITS_PER_SYMBOL = BPS;
    N_SYMBOLS = pow(2, BPS);
    Constellation constellation[N_SYMBOLS];
    init_GAM_constellation(constellation, BITS_PER_SYMBOL);
    //normGAM(symbols, numsymb);

    int bit_counter = 0;

    for (int j = 0; j < numsymb; j++)
    {
        int best_index = find_nearest_symbol(symbols[j], constellation);

        // Copiar los bits del símbolo detectado al array de salida
        for (int bit_pos = 0; bit_pos < BITS_PER_SYMBOL; bit_pos++)
        {
            bits[bit_counter++] = constellation[best_index].bits[bit_pos];
        }
    }

    return bit_counter; // Número total de bits demodulados
}
//END GOLDEN MODULATION