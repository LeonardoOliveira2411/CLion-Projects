#include <stdint.h>

//BIT MAPPING
const uint8_t BIT_MAPPING_4[4] =
{
    0b00, 0b11,
    0b10, 0b01
};

const uint8_t BIT_MAPPING_8[8] =
{
    0b000, 0b110, 0b101, 0b011,
    0b111, 0b001, 0b010, 0b100
};

const uint8_t BIT_MAPPING_16[16] =
{
    0b0000, 0b1100, 0b1111, 0b0011,
    0b0110, 0b1001, 0b0101, 0b1010,
    0b0001, 0b1110, 0b0010, 0b1101,
    0b0111, 0b1000, 0b0100, 0b1011
};

const uint8_t BIT_MAPPING_32[32] =
{
    // Primera mitad (16 símbolos)
    0b00000,  // 0
    0b00111,  // 1 (distancia 3 con 0b00000)
    0b01011,  // 2 (distancia 3 con 0b00000 y 0b00111)
    0b01100,  // 3 (distancia 3 con los anteriores)
    0b10011,  // 4
    0b10100,  // 5
    0b11000,  // 6
    0b11111,  // 7

    0b00011,  // 8
    0b00100,  // 9
    0b01000,  // 10
    0b01111,  // 11
    0b10000,  // 12
    0b10111,  // 13
    0b11011,  // 14
    0b11100,  // 15

    // Segunda mitad (16 símbolos)
    0b00001,  // 16
    0b00110,  // 17
    0b01010,  // 18
    0b01101,  // 19
    0b10010,  // 20
    0b10101,  // 21
    0b11001,  // 22
    0b11110,  // 23

    0b00010,  // 24
    0b00101,  // 25
    0b01001,  // 26
    0b01110,  // 27
    0b10001,  // 28
    0b10110,  // 29
    0b11010,  // 30
    0b11101   // 31
};

const uint8_t BIT_MAPPING_64[64] =
{
    // Primera mitad (32 símbolos)
    0b000000,  // 0
    0b110000,  // 1 (distancia 2 con 0b000000)
    0b101000,  // 2 (distancia 2 con 0b110000)
    0b011000,  // 3 (distancia 2 con 0b101000)
    0b111100,  // 4 (distancia 2 con 0b011000)
    0b001100,  // 5 (distancia 2 con 0b111100)
    0b010100,  // 6 (distancia 2 con 0b001100)
    0b100100,  // 7 (distancia 2 con 0b010100)
    0b110110,  // 8 (distancia 2 con 0b100100)
    0b000110,  // 9 (distancia 2 con 0b110110)
    0b011110,  // 10 (distancia 2 con 0b000110)
    0b101110,  // 11 (distancia 2 con 0b011110)
    0b111010,  // 12 (distancia 2 con 0b101110)
    0b001010,  // 13 (distancia 2 con 0b111010)
    0b010010,  // 14 (distancia 2 con 0b001010)
    0b100010,  // 15 (distancia 2 con 0b010010)
    0b110011,  // 16 (distancia 2 con 0b100010)
    0b000011,  // 17 (distancia 2 con 0b110011)
    0b011011,  // 18 (distancia 2 con 0b000011)
    0b101011,  // 19 (distancia 2 con 0b011011)
    0b111111,  // 20 (distancia 2 con 0b101011)
    0b001111,  // 21 (distancia 2 con 0b111111)
    0b010111,  // 22 (distancia 2 con 0b001111)
    0b100111,  // 23 (distancia 2 con 0b010111)
    0b110101,  // 24 (distancia 2 con 0b100111)
    0b000101,  // 25 (distancia 2 con 0b110101)
    0b011101,  // 26 (distancia 2 con 0b000101)
    0b101101,  // 27 (distancia 2 con 0b011101)
    0b111001,  // 28 (distancia 2 con 0b101101)
    0b001001,  // 29 (distancia 2 con 0b111001)
    0b010001,  // 30 (distancia 2 con 0b001001)
    0b100001,  // 31 (distancia 2 con 0b010001)

    // Segunda mitad (32 símbolos)
    0b111000,  // 32 (distancia 2 con 0b100001)
    0b001000,  // 33 (distancia 2 con 0b111000)
    0b010000,  // 34 (distancia 2 con 0b001000)
    0b100000,  // 35 (distancia 2 con 0b010000)
    0b110100,  // 36 (distancia 2 con 0b100000)
    0b000100,  // 37 (distancia 2 con 0b110100)
    0b011100,  // 38 (distancia 2 con 0b000100)
    0b101100,  // 39 (distancia 2 con 0b011100)
    0b111110,  // 40 (distancia 2 con 0b101100)
    0b001110,  // 41 (distancia 2 con 0b111110)
    0b010110,  // 42 (distancia 2 con 0b001110)
    0b100110,  // 43 (distancia 2 con 0b010110)
    0b110010,  // 44 (distancia 2 con 0b100110)
    0b000010,  // 45 (distancia 2 con 0b110010)
    0b011010,  // 46 (distancia 2 con 0b000010)
    0b101010,  // 47 (distancia 2 con 0b011010)
    0b111011,  // 48 (distancia 2 con 0b101010)
    0b001011,  // 49 (distancia 2 con 0b111011)
    0b010011,  // 50 (distancia 2 con 0b001011)
    0b100011,  // 51 (distancia 2 con 0b010011)
    0b110111,  // 52 (distancia 2 con 0b100011)
    0b000111,  // 53 (distancia 2 con 0b110111)
    0b011111,  // 54 (distancia 2 con 0b000111)
    0b101111,  // 55 (distancia 2 con 0b011111)
    0b111101,  // 56 (distancia 2 con 0b101111)
    0b001101,  // 57 (distancia 2 con 0b111101)
    0b010101,  // 58 (distancia 2 con 0b001101)
    0b100101,  // 59 (distancia 2 con 0b010101)
    0b110001,  // 60 (distancia 2 con 0b100101)
    0b000001,  // 61 (distancia 2 con 0b110001)
    0b011001,  // 62 (distancia 2 con 0b000001)
    0b101001   // 63 (distancia 2 con 0b011001)
};
