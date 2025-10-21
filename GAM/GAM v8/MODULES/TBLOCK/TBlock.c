#include "TBlock.h"

// FUNCIONES CRC
void calculate_crc24a(const int data_bits[TB_SIZE_BITS], int crc_bits[CRC24A_LEN])
{
    uint32_t crc = 0xFFFFFF;

    for (int i = 0; i < TB_SIZE_BITS; i++)
    {
        crc ^= (data_bits[i] << 23);
        if (crc & 0x800000) { crc = (crc << 1) ^ CRC24A_POLY; }
        else { crc <<= 1; }
        crc &= 0xFFFFFF;
    }
    for (int i = 0; i < CRC24A_LEN; i++) { crc_bits[i] = (int)(crc >> (23 - i)) & 1; }
}

bool verify_crc24a(const int received_bits[TOTAL_BITS]) {
    uint32_t crc = 0xFFFFFF;
    for (int i = 0; i < TOTAL_BITS; i++) {
        crc ^= (received_bits[i] << 23);
        if (crc & 0x800000) { crc = (crc << 1) ^ CRC24A_POLY; }
        else { crc <<= 1; }
        crc &= 0xFFFFFF;
    }
    return (crc == 0);
}


// FUNCIONES ENTRELAZADO
bool interleave_bits(const int input_bits[TOTAL_BITS_REPEATED], int output_bits[TOTAL_BITS_REPEATED]) {
    if (TOTAL_BITS_REPEATED != INTERLEAVER_SIZE) {
        printf("Error: Tamaño de entrelazado no coincide\n");
        return true;
    }

    int matrix[INTERLEAVER_ROWS][INTERLEAVER_COLS];
    int idx = 0;

    for (int i = 0; i < INTERLEAVER_ROWS; i++) {
        for (int j = 0; j < INTERLEAVER_COLS; j++) {
            matrix[i][j] = input_bits[idx++];
        }
    }

    idx = 0;
    for (int j = 0; j < INTERLEAVER_COLS; j++) {
        for (int i = 0; i < INTERLEAVER_ROWS; i++) {
            output_bits[idx++] = matrix[i][j];
        }
    }

    return false;
}

bool deinterleave_bits(const int input_bits[TOTAL_BITS_REPEATED], int output_bits[TOTAL_BITS_REPEATED]) {
    if (TOTAL_BITS_REPEATED != INTERLEAVER_SIZE) {
        printf("Error: Tamaño de entrelazado no coincide\n");
        return true;
    }

    int matrix[INTERLEAVER_ROWS][INTERLEAVER_COLS];
    int idx = 0;

    for (int j = 0; j < INTERLEAVER_COLS; j++) {
        for (int i = 0; i < INTERLEAVER_ROWS; i++) {
            matrix[i][j] = input_bits[idx++];
        }
    }

    idx = 0;
    for (int i = 0; i < INTERLEAVER_ROWS; i++) {
        for (int j = 0; j < INTERLEAVER_COLS; j++) {
            output_bits[idx++] = matrix[i][j];
        }
    }

    return false;
}


// FUNCIONES TRANSPORT BLOCK
bool build_transport_block(TransportBlock *tb, const int data_bits[TB_SIZE_BITS]) {
    for (int i = 0; i < TB_SIZE_BITS; i++) {
        tb->data_bits[i] = data_bits[i];
    }

    calculate_crc24a(data_bits, tb->crc_bits);

    int idx = 0;
    for (int i = 0; i < TB_SIZE_BITS; i++)
        tb->total_bits[idx++] = data_bits[i];
    for (int i = 0; i < CRC_TYPE; i++)
        tb->total_bits[idx++] = tb->crc_bits[i];

    // Repetición simple (1:1 para prueba)
    for (int i = 0; i < TOTAL_BITS; i++) {
        tb->repeated_bits[i] = tb->total_bits[i];
    }

    interleave_bits(tb->repeated_bits, tb->interleaved_bits);
    tb->crc_valid = true;
    return false;
}

bool process_received_block(const int received_bits[TOTAL_BITS_REPEATED], TransportBlock *tb) {
    int deinterleaved_bits[TOTAL_BITS_REPEATED];
    deinterleave_bits(received_bits, deinterleaved_bits);

    // Decodificación simple (1:1 para prueba)
    for (int i = 0; i < TOTAL_BITS; i++) {
        tb->total_bits[i] = deinterleaved_bits[i];
    }

    for (int i = 0; i < TB_SIZE_BITS; i++)
        tb->data_bits[i] = tb->total_bits[i];
    for (int i = 0; i < CRC_TYPE; i++)
        tb->crc_bits[i] = tb->total_bits[TB_SIZE_BITS + i];

    tb->crc_valid = verify_crc24a(tb->total_bits);

    return !tb->crc_valid;
}
