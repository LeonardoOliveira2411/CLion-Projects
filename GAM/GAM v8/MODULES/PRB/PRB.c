#include "PRB.h"

bool init_prb_grid(PRB_Grid *grid)
{
    if (!grid)
    {
        printf("Error: Grid PRB no valido\n");
        return true;
    }

    // Posiciones fijas de pilotos
    grid->pilot_positions[0] = 1;   // Subportadora 1
    grid->pilot_positions[1] = 4;   // Subportadora 4
    grid->pilot_positions[2] = 7;   // Subportadora 7
    grid->pilot_positions[3] = 10;  // Subportadora 10

    // Inicializar todos los símbolos a cero
    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        for (int sc = 0; sc < PRB_SUBCARRIERS; sc++)
        {
            grid->data_symbols[sym][sc] = 0.0f + 0.0f * I;
        }
        for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
        {
            grid->pilot_symbols[sym][p] = PILOT_VALUE;
        }
    }

    grid->initialized = true;
    printf("PRB Grid inicializado: %dx%d (%d REs totales, %d datos, %d pilotos)\n",
           PRB_SYMBOLS, PRB_SUBCARRIERS, RESOURCE_ELEMENTS_PER_PRB,
           DATA_RE_PER_PRB, TOTAL_PILOTS);

    return false;
}

bool map_data_to_prb(const float complex data_symbols[TOTAL_SYMBOLS], PRB_Grid *grid)
{
    if (!grid || !grid->initialized)
    {
        printf("Error: Grid PRB no inicializado\n");
        return true;
    }

    if (TOTAL_SYMBOLS > DATA_RE_PER_PRB)
    {
        printf("Error: Demasiados símbolos (%d) para REs disponibles (%d)\n",
               TOTAL_SYMBOLS, DATA_RE_PER_PRB);
        return true;
    }

    int data_idx = 0;

    // Distribuir símbolos en el grid evitando posiciones de pilotos
    for (int sym = 0; sym < PRB_SYMBOLS && data_idx < TOTAL_SYMBOLS; sym++)
    {
        for (int sc = 0; sc < PRB_SUBCARRIERS && data_idx < TOTAL_SYMBOLS; sc++)
        {
            // Verificar si esta posición es piloto
            bool is_pilot = false;
            for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
            {
                if (sc == grid->pilot_positions[p])
                {
                    is_pilot = true;
                    break;
                }
            }

            // Si no es piloto, asignar dato
            if (!is_pilot) { grid->data_symbols[sym][sc] = data_symbols[data_idx++]; }
        }
    }

    printf("Datos mapeados al PRB: %d/%d simbolos en grid %dx%d\n",
           data_idx, TOTAL_SYMBOLS, PRB_SYMBOLS, PRB_SUBCARRIERS);

    return false;
}

bool extract_data_from_prb(const PRB_Grid *grid, float complex data_symbols[TOTAL_SYMBOLS])
{
    if (!grid || !grid->initialized)
    {
        printf("Error: Grid PRB no inicializado\n");
        return true;
    }

    int data_idx = 0;

    // Extraer datos del grid evitando posiciones de pilotos
    for (int sym = 0; sym < PRB_SYMBOLS && data_idx < TOTAL_SYMBOLS; sym++)
    {
        for (int sc = 0; sc < PRB_SUBCARRIERS && data_idx < TOTAL_SYMBOLS; sc++)
        {
            // Verificar si esta posición es piloto
            bool is_pilot = false;
            for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
            {
                if (sc == grid->pilot_positions[p]) { is_pilot = true; break; }
            }

            // Si no es piloto, extraer dato
            if (!is_pilot) { data_symbols[data_idx++] = grid->data_symbols[sym][sc]; }
        }
    }

    printf("Datos extraidos del PRB: %d/%d simbolos\n", data_idx, TOTAL_SYMBOLS);

    return false;
}

bool generate_prb_ofdm_symbols(const PRB_Grid *grid, float complex ofdm_symbols[PRB_SYMBOLS][N_FFT])
{
    if (!grid || !grid->initialized)
    {
        printf("Error: Grid PRB no inicializado\n");
        return true;
    }

    // Para cada símbolo OFDM en el PRB
    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        // Inicializar subportadoras a cero
        float complex subcarriers[N_FFT];
        for (int i = 0; i < N_FFT; i++) { subcarriers[i] = 0.0f + 0.0f * I; }

        // Mapear datos y pilotos a subportadoras (centrado)
        for (int sc = 0; sc < PRB_SUBCARRIERS; sc++)
        {
            int global_sc_idx = SUBCARRIERS_START + sc;
            if (global_sc_idx >= 0 && global_sc_idx < N_FFT)
            {
                // Verificar si es posición de piloto
                bool is_pilot = false;
                for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
                {
                    if (sc == grid->pilot_positions[p])
                    {
                        subcarriers[global_sc_idx] = grid->pilot_symbols[sym][p];
                        is_pilot = true;
                        break;
                    }
                }
                // Si no es piloto, usar dato
                if (!is_pilot) { subcarriers[global_sc_idx] = grid->data_symbols[sym][sc]; }
            }
        }

        // Aplicar IDFT para generar símbolo OFDM en tiempo
        for (int n = 0; n < N_FFT; n++)
        {
            ofdm_symbols[sym][n] = 0.0f + 0.0f * I;
            for (int k = 0; k < N_FFT; k++)
            {
                float angle = (float)(2.0f * M_PI * k * n) / N_FFT;
                ofdm_symbols[sym][n] += subcarriers[k] * (cosf(angle) + sinf(angle) * I);
            }
            ofdm_symbols[sym][n] /= sqrtf(N_FFT);
        }
    }

    printf("Simbolos OFDM generados: %d simbolos X %d muestras\n", PRB_SYMBOLS, N_FFT);

    return false;
}

bool process_received_prb_ofdm(const float complex received_symbols[PRB_SYMBOLS][N_FFT], PRB_Grid *grid)
{
    if (!grid)
    {
        printf("Error: Grid PRB no valido\n");
        return true;
    }

    // Procesar cada símbolo OFDM recibido
    for (int sym = 0; sym < PRB_SYMBOLS; sym++)
    {
        // Aplicar DFT al símbolo recibido
        float complex subcarriers[N_FFT];
        for (int k = 0; k < N_FFT; k++)
        {
            subcarriers[k] = 0.0f + 0.0f * I;
            for (int n = 0; n < N_FFT; n++)
            {
                float angle = (float)(-2.0f * M_PI * k * n) / N_FFT;
                subcarriers[k] += received_symbols[sym][n] * (cosf(angle) + sinf(angle) * I);
            }
            subcarriers[k] /= sqrtf(N_FFT);
        }

        // Extraer datos y pilotos del PRB
        for (int sc = 0; sc < PRB_SUBCARRIERS; sc++)
        {
            int global_sc_idx = SUBCARRIERS_START + sc;
            if (global_sc_idx >= 0 && global_sc_idx < N_FFT)
            {
                // Verificar si es posición de piloto
                bool is_pilot = false;
                for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
                {
                    if (sc == grid->pilot_positions[p])
                    {
                        grid->pilot_symbols[sym][p] = subcarriers[global_sc_idx];
                        is_pilot = true;
                        break;
                    }
                }
                // Si no es piloto, extraer dato
                if (!is_pilot) { grid->data_symbols[sym][sc] = subcarriers[global_sc_idx]; }
            }
        }
    }

    printf("PRB OFDM procesado: %d simbolos extraidos\n", PRB_SYMBOLS);

    return false;
}
