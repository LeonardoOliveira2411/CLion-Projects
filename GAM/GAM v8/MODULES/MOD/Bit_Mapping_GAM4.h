#ifndef GAM_BIT_MAPPING_GAM4_H
#define GAM_BIT_MAPPING_GAM4_H

static const int BIT_MAPPING[16][4] = {
        {0, 0, 0, 0},  // Índice 0
        {0, 0, 0, 1},  // Índice 1
        {0, 0, 1, 1},  // Índice 2
        {0, 0, 1, 0},  // Índice 3
        {0, 1, 1, 0},  // Índice 4
        {0, 1, 1, 1},  // Índice 5
        {0, 1, 0, 1},  // Índice 6
        {0, 1, 0, 0},  // Índice 7
        {1, 1, 0, 0},  // Índice 8
        {1, 1, 0, 1},  // Índice 9
        {1, 1, 1, 1},  // Índice 10
        {1, 1, 1, 0},  // Índice 11
        {1, 0, 1, 0},  // Índice 12
        {1, 0, 1, 1},  // Índice 13
        {1, 0, 0, 1},  // Índice 14
        {1, 0, 0, 0}   // Índice 15
};

#endif //GAM_BIT_MAPPING_GAM4_H
