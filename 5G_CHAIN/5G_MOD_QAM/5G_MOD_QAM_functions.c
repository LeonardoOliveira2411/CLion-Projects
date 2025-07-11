#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>

//4QAM
// Modulador 4QAM
int modulate_4QAM(int *bits, float complex *symbols, int numbits)
{
    int numsymb = 0;
    for (int i = 0; i < numbits; i += 2)
    {
        int bit1 = bits[i];
        int bit2 = bits[i+1];

        // Mapeo Gray para 4QAM:
        // 00 -> -1-1j 01 -> -1+1j
        // 11 -> +1+1j 10 -> +1-1j
        float InPhase = (bit1 == 0) ? -1.0 : 1.0;
        float Quadrature = (bit2 == 0) ? -1.0 : 1.0;

        symbols[numsymb] = InPhase + Quadrature * I;
        numsymb++;
    }

    return numsymb;
}

// Demodulador 4QAM
int demodulate_4QAM(float complex *symbols, int *bits, int numsymb)
{
    int numbits;
    for (int i = 0; i < numsymb; i++)
    {
        float InPhase = creal(symbols[i]);
        float Quadrature = cimag(symbols[i]);

        // Mapeo Gray para 4QAM:
        // 00 -> -1-1j 01 -> -1+1j
        // 11 -> +1+1j 10 -> +1-1j
        bits[2*i] = (InPhase > 0) ? 1 : 0;
        bits[2*i + 1] = (Quadrature > 0) ? 1 : 0;
        numbits += 2;
    }

    return numbits;
}
//END 4QAM

//16QAM
// Modulador 16QAM
int modulate_16QAM(int *bits, float complex *symbols, int numbits)
{
    // Verificar que el número de bits es múltiplo de 4
    if (numbits % 4 != 0)
    {
        fprintf(stderr, "Error: El número de bits debe ser múltiplo de 4 para 16QAM\n");
        return -1;
    }

    int numsymb = 0;
    for (int i = 0; i < numbits; i += 4)
    {
        int bit0 = bits[i];
        int bit1 = bits[i+1];
        int bit2 = bits[i+2];
        int bit3 = bits[i+3];

        // Mapeo Gray para 16QAM (constelación rectangular)
        float InPhase, Quadrature;

        // Bits 0 y 1 determinan la amplitud en I
        switch((bit0 << 1) | bit1)
        {
            case 0: InPhase = -3.0; break; // 00
            case 1: InPhase = -1.0; break; // 01
            case 3: InPhase = 1.0; break;  // 11
            case 2: InPhase = 3.0; break;  // 10
            default: InPhase = 0.0; break; // Nunca debería ocurrir
        }

        // Bits 2 y 3 determinan la amplitud en Q
        switch((bit2 << 1) | bit3)
        {
            case 0: Quadrature = -3.0; break; // 00
            case 1: Quadrature = -1.0; break; // 01
            case 3: Quadrature = 1.0; break;  // 11
            case 2: Quadrature = 3.0; break;  // 10
            default: Quadrature = 0.0; break; // Nunca debería ocurrir
        }

        // Aplicar factor de escala y formar símbolo complejo
        symbols[numsymb] = (InPhase + Quadrature * I);
        numsymb++;
    }

    return numsymb;
}

// Demodulador 16QAM
int demodulate_16QAM(float complex *symbols, int *bits, int numsymb)
{
    int numbits = 0;
    
    for (int i = 0; i < numsymb; i++)
    {
        // Escalar y obtener componentes
        float InPhase = creal(symbols[i]);
        float Quadrature = cimag(symbols[i]);
        
        // Decodificación de la componente en fase (bits 0 y 1)
        if (InPhase < -2.0)
        {
            bits[numbits++] = 0;
            bits[numbits++] = 0;
        }
        else if (InPhase < 0.0)
        {
            bits[numbits++] = 0;
            bits[numbits++] = 1;
        }
        else if (InPhase < 2.0)
        {
            bits[numbits++] = 1;
            bits[numbits++] = 1;
        }
        else
        {
            bits[numbits++] = 1;
            bits[numbits++] = 0;
        }
        
        // Decodificación de la componente en cuadratura (bits 2 y 3)
        if (Quadrature < -2.0)
        {
            bits[numbits++] = 0;
            bits[numbits++] = 0;
        }
        else if (Quadrature < 0.0)
        {
            bits[numbits++] = 0;
            bits[numbits++] = 1;
        }
        else if (Quadrature < 2.0)
        {
            bits[numbits++] = 1;
            bits[numbits++] = 1;
        }
        else
        {
            bits[numbits++] = 1;
            bits[numbits++] = 0;
        }
    }

    return numbits;
}
//END 16QAM

//64QAM
// Modulador 64QAM
int modulate_64QAM(int *bits, float complex *symbols, int numbits)
{
    // Verificar que el número de bits es múltiplo de 6
    if (numbits % 6 != 0)
    {
        fprintf(stderr, "Error: El número de bits debe ser múltiplo de 6 para 64QAM\n");
        return -1;
    }

    int numsymb = 0;
    for (int i = 0; i < numbits; i += 6)
    {
        int bit0 = bits[i];
        int bit1 = bits[i+1];
        int bit2 = bits[i+2];
        int bit3 = bits[i+3];
        int bit4 = bits[i+4];
        int bit5 = bits[i+5];

        // Mapeo Gray para 64QAM (constelación rectangular)
        float InPhase, Quadrature;

        // Bits 0, 1 y 2 determinan la amplitud en I
        switch((bit0 << 2) | (bit1 << 1) | bit2)
        {
            case 0: InPhase = -7.0; break; // 000
            case 1: InPhase = -5.0; break; // 001
            case 3: InPhase = -3.0; break; // 011
            case 2: InPhase = -1.0; break; // 010
            case 6: InPhase = 1.0; break;  // 110
            case 7: InPhase = 3.0; break;  // 111
            case 5: InPhase = 5.0; break;  // 101
            case 4: InPhase = 7.0; break;  // 100
            default: InPhase = 0.0; break; // Nunca debería ocurrir
        }

        // Bits 3, 4 y 5 determinan la amplitud en Q
        switch((bit3 << 2) | (bit4 << 1) | bit5)
        {
            case 0: Quadrature = -7.0; break; // 000
            case 1: Quadrature = -5.0; break; // 001
            case 3: Quadrature = -3.0; break; // 011
            case 2: Quadrature = -1.0; break; // 010
            case 6: Quadrature = 1.0; break;  // 110
            case 7: Quadrature = 3.0; break;  // 111
            case 5: Quadrature = 5.0; break;  // 101
            case 4: Quadrature = 7.0; break;  // 100
            default: Quadrature = 0.0; break; // Nunca debería ocurrir
        }

        // Normalización para mantener potencia promedio igual a 1
        symbols[numsymb] = (InPhase + Quadrature * I) / sqrtf(42.0f);
        numsymb++;
    }

    return numsymb;
}

// Demodulador 64QAM
int demodulate_64QAM(float complex *symbols, int *bits, int numsymb)
{
    int numbits = 0;

    for (int i = 0; i < numsymb; i++)
    {
        // Escalar y obtener componentes
        float InPhase = creal(symbols[i]) * sqrtf(42.0f);
        float Quadrature = cimag(symbols[i]) * sqrtf(42.0f);

        // Decodificación de la componente en fase (bits 0, 1, 2)
        if (InPhase < -6.0)
        {
            bits[numbits++] = 0;
            bits[numbits++] = 0;
            bits[numbits++] = 0;
        }
        else if (InPhase < -4.0)
        {
            bits[numbits++] = 0;
            bits[numbits++] = 0;
            bits[numbits++] = 1;
        }
        else if (InPhase < -2.0)
        {
            bits[numbits++] = 0;
            bits[numbits++] = 1;
            bits[numbits++] = 1;
        }
        else if (InPhase < 0.0)
        {
            bits[numbits++] = 0;
            bits[numbits++] = 1;
            bits[numbits++] = 0;
        }
        else if (InPhase < 2.0)
        {
            bits[numbits++] = 1;
            bits[numbits++] = 1;
            bits[numbits++] = 0;
        }
        else if (InPhase < 4.0)
        {
            bits[numbits++] = 1;
            bits[numbits++] = 1;
            bits[numbits++] = 1;
        }
        else if (InPhase < 6.0)
        {
            bits[numbits++] = 1;
            bits[numbits++] = 0;
            bits[numbits++] = 1;
        }
        else
        {
            bits[numbits++] = 1;
            bits[numbits++] = 0;
            bits[numbits++] = 0;
        }

        // Decodificación de la componente en cuadratura (bits 3, 4, 5)
        if (Quadrature < -6.0)
        {
            bits[numbits++] = 0;
            bits[numbits++] = 0;
            bits[numbits++] = 0;
        }
        else if (Quadrature < -4.0)
        {
            bits[numbits++] = 0;
            bits[numbits++] = 0;
            bits[numbits++] = 1;
        }
        else if (Quadrature < -2.0)
        {
            bits[numbits++] = 0;
            bits[numbits++] = 1;
            bits[numbits++] = 1;
        }
        else if (Quadrature < 0.0)
        {
            bits[numbits++] = 0;
            bits[numbits++] = 1;
            bits[numbits++] = 0;
        }
        else if (Quadrature < 2.0)
        {
            bits[numbits++] = 1;
            bits[numbits++] = 1;
            bits[numbits++] = 0;
        }
        else if (Quadrature < 4.0)
        {
            bits[numbits++] = 1;
            bits[numbits++] = 1;
            bits[numbits++] = 1;
        }
        else if (Quadrature < 6.0)
        {
            bits[numbits++] = 1;
            bits[numbits++] = 0;
            bits[numbits++] = 1;
        }
        else
        {
            bits[numbits++] = 1;
            bits[numbits++] = 0;
            bits[numbits++] = 0;
        }
    }

    return numbits;
}
//END 64QAM
