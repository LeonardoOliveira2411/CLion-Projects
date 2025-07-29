#ifndef GNU_PLOT_H
#define GNU_PLOT_H

#include <stdio.h>
#include "Enlace_WDM_functions.h"

void plot_wdm_signal(float *wdm_signal, int first_call)
{
    static FILE *pipe = NULL;

    // Inicializar GNUplot en la primera llamada
    if (first_call) {
        pipe = _popen("gnuplot -persistent", "w");
        if (!pipe) {
            fprintf(stderr, "Error al abrir GNUplot.\n");
            return;
        }

        // Configuración inicial del gráfico
        fprintf(pipe, "reset\n");
        fprintf(pipe, "set terminal qt size 800,600 title 'WDM Signal'\n");
        fprintf(pipe, "set title 'Multiplexed WDM Signal'\n");
        fprintf(pipe, "set xlabel 'Tiempo (muestras)'\n");
        fprintf(pipe, "set ylabel 'Amplitud'\n");
        fprintf(pipe, "set grid\n");
        fprintf(pipe, "set key right top\n");
        fprintf(pipe, "set xrange [0:%d]\n", NUM_BITS-1);
    }

    // Calcular rango Y automático
    float ymin = 0.0f, ymax = 0.0f;
    for (int i = 0; i < NUM_BITS; i++) {
        if (wdm_signal[i] < ymin) ymin = wdm_signal[i];
        if (wdm_signal[i] > ymax) ymax = wdm_signal[i];
    }
    float margin = (ymax - ymin) * 0.1;
    fprintf(pipe, "set yrange [%f:%f]\n", ymin - margin, ymax + margin);

    // Enviar comando de plot con datos inline
    fprintf(pipe, "plot '-' with lines title 'WDM Signal'\n");
    for (int i = 0; i < NUM_BITS; i++) {
        fprintf(pipe, "%d %f\n", i, wdm_signal[i]);
    }
    fprintf(pipe, "e\n");  // Esto debe estar en una línea separada

    fflush(pipe);
}

#endif