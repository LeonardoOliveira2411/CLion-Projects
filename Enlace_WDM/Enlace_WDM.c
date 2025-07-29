#include <stdio.h>
#include "Functions/Enlace_WDM_functions.h"
#include "Functions/GNU_plot.h"
#include <windows.h>

#define EXEC_TIMES 1

int main(void)
{
    //Variables del sistema TX
    int tx_bits[NUM_CHANNELS][NUM_BITS] = {0};
    float complex tx_signal[NUM_CHANNELS][NUM_SYMBOLS] = {0.0f};
    float wdm_signal[NUM_SYMBOLS] = {0.0f};

    //Variables del sistema RX
    float complex rx_signal[NUM_CHANNELS][NUM_SYMBOLS] = {0.0f};
    int rx_bits[NUM_CHANNELS][NUM_BITS] = {0};
    float ber[NUM_CHANNELS] = {0.0f};
    int total_errors = 0;

    int times = 0;
    while (times < EXEC_TIMES)
    {
        //TX
        for (int ch = 0; ch < NUM_CHANNELS; ch++)
        {
            gen_random_bits(tx_bits[ch], NUM_BITS);
            gen_4QAM_signal(tx_bits[ch], tx_signal[ch]);
            //for (int i = 0; i < NUM_SYMBOLS; i++) printf("Signal Imag[%d][%d] = %f\n", ch, i, cimagf(tx_signal[ch][i]));
        }
        wdm_mux(tx_signal, wdm_signal);
        //for (int i = 0; i < NUM_BITS; i++) printf("WDM Signal[%d]: %f\n", i, wdm_signal[i]);
        plot_wdm_signal(wdm_signal, (times == 0));


        //RX
        wdm_demux(wdm_signal, rx_signal);

        for (int ch = 0; ch < NUM_CHANNELS; ch++)
        {
            //for (int i = 0; i < NUM_BITS; i++) printf("RX Signal[%d][%d] = %f\n", ch, i, rx_signal[ch][i]);
            demod_4QAM_signal(rx_signal[ch], rx_bits[ch]);

            random_bits_sink(tx_bits[ch], rx_bits[ch], NUM_BITS, &ber[ch]);
            printf("Canal %d - BER: %.4f\n", ch+1, ber[ch]);
        }
        Sleep(500);
        times++;
    }
    return 0;
}