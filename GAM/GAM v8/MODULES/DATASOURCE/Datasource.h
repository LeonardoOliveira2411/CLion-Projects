#ifndef GAM_DATASOURCE_H
#define GAM_DATASOURCE_H

#include "../../MODULES/Common.h"


// GENERACIÓN DE BITS Y CÁLCULO DE ERRORES
bool generate_random_bits(int bits[TB_SIZE_BITS]);

float calculate_ber(const int tx_bits[TB_SIZE_BITS], const int rx_bits[TB_SIZE_BITS]);

float calculate_bler(int successful_transmissions, int total_transmissions);


#endif //GAM_DATASOURCE_H
