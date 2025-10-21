#ifndef GAM_TRANSPORT_BLOCK_H
#define GAM_TRANSPORT_BLOCK_H

#include "../../MODULES/Common.h"


// CRC
void calculate_crc24a(const int data_bits[TB_SIZE_BITS], int crc_bits[CRC24A_LEN]);
bool verify_crc24a(const int received_bits[TOTAL_BITS]);


// ENTRELAZADO
bool interleave_bits(const int input_bits[TOTAL_BITS_REPEATED], int output_bits[TOTAL_BITS_REPEATED]);
bool deinterleave_bits(const int input_bits[TOTAL_BITS_REPEATED], int output_bits[TOTAL_BITS_REPEATED]);


// GESTIÓN DEL T-BLOCK
bool build_transport_block(TransportBlock *tb, const int data_bits[TB_SIZE_BITS]);
bool process_received_block(const int received_bits[TOTAL_BITS_REPEATED], TransportBlock *tb);


#endif //GAM_TRANSPORT_BLOCK_H
