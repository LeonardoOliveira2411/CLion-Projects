#include "Mod.h"

// Selección automática del archivo de mapeo según BPS
#if BPS == 2
#include "Bit_Mapping_GAM2.h"
#elif BPS == 3
#include "Bit_Mapping_GAM3.h"
#elif BPS == 4
    #include "Bit_Mapping_GAM4.h"
#else
    #error "BPS no soportado. Use BPS = 2, 3 o 4"
#endif

// FUNCIONES MODULACIÓN
bool init_golden_modulation(Constellation constellation[C_POINTS])
{
    if (C_POINTS != (1 << BPS)) {
        return true; // Error: C_POINTS debe ser 2^BPS
    }

    float total_power = 0.0f;

    for (int i = 0; i < C_POINTS; i++)
    {
        // Patrón espiral dorado
        float angle = (i + 1) * ANGLE_STEP;
        float radius = sqrtf(i + 1);
        constellation[i].point = radius * (cosf(angle) + sinf(angle)*I);
        total_power += crealf(constellation[i].point) * crealf(constellation[i].point) +
                       cimagf(constellation[i].point) * cimagf(constellation[i].point);

        // Asignación de bits desde el mapping predefinido
        for (int bit_pos = 0; bit_pos < BPS; bit_pos++)
        {
            constellation[i].bits[bit_pos] = BIT_MAPPING[i][bit_pos];
        }
    }

    // Normalización de potencia
    float norm_factor = sqrtf(total_power / C_POINTS);

    for (int i = 0; i < C_POINTS; i++)
    {
        constellation[i].point /= norm_factor;
    }

    return false;
}

bool golden_modulation_hard(const int interleaved_bits[TOTAL_BITS_REPEATED], float complex symbols[TOTAL_SYMBOLS])
{
    Constellation temp_const[C_POINTS];

    if (init_golden_modulation(temp_const)) return true;

    for (int i = 0; i < TOTAL_SYMBOLS; i++)
    {
        int start_bit_index = i * BPS;
        bool found = false;

        // Buscar el símbolo que coincide con los bits
        for (int j = 0; j < C_POINTS; j++)
        {
            bool match = true;
            for (int k = 0; k < BPS; k++)
            {
                if (temp_const[j].bits[k] != interleaved_bits[start_bit_index + k])
                {
                    match = false;
                    break;
                }
            }

            if (match)
            {
                symbols[i] = temp_const[j].point;
                found = true;
                break;
            }
        }

        if (!found) return true;
    }

    return false;
}

// FUNCIONES DEMODULACIÓN
int calculate_min_distance_hard(float complex symbol, Constellation constellation[C_POINTS])
{
    float min_dist = FLT_MAX;
    int best_index = -1;

    for (int i = 0; i < C_POINTS; i++)
    {
        float complex diff = symbol - constellation[i].point;
        float dist = crealf(diff * conjf(diff));

        if (dist < min_dist)
        {
            min_dist = dist;
            best_index = i;
        }
    }

    return best_index;
}

bool golden_demodulation_hard(float complex symbols[TOTAL_SYMBOLS], Constellation constellation[C_POINTS],
                              int interleaved_bits[TOTAL_BITS_REPEATED])
{
    int index, bit_index = 0;

    for (int i = 0; i < TOTAL_SYMBOLS; i++)
    {
        index = calculate_min_distance_hard(symbols[i], constellation);

        if (index == -1) return true;

        // Copiar los bits del símbolo detectado
        for (int j = 0; j < BPS; j++)
        {
            interleaved_bits[bit_index + j] = constellation[index].bits[j];
        }
        bit_index += BPS;
    }

    return false;
}