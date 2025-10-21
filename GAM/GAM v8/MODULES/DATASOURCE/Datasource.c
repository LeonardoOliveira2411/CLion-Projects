#include "Datasource.h"

// FUNCIONES DATASOURCE
bool generate_random_bits(int bits[TB_SIZE_BITS])
{
    for (int i = 0; i < TB_SIZE_BITS; i++)
    {   bits[i] = rand() % 2;   }

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

float calculate_bler(int successful_transmissions, int total_transmissions)
{
    return ((float)(total_transmissions - successful_transmissions)/(float)total_transmissions);
}