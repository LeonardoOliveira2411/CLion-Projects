#ifndef INC_5G_MOD_QAM_FUNCTIONS_H
#define INC_5G_MOD_QAM_FUNCTIONS_H

#include <complex.h>

//4QAM
int modulate_4QAM(int *bits, float complex *symbols, int length);
int demodulate_4QAM(float complex *symbols, int *bits, int length);

//16QAM
int modulate_16QAM(int *bits, float complex *symbols, int numbits);
int demodulate_16QAM(float complex *symbols, int *bits, int numsymb);

//64QAM
int modulate_64QAM(int *bits, float complex *symbols, int numbits);
int demodulate_64QAM(float complex *symbols, int *bits, int numsymb);

#endif //INC_5G_MOD_QAM_FUNCTIONS_H
