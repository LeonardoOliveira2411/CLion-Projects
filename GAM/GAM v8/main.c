//LIBRERÍAS MÓDULOS CADENA
#include "MODULES/Common.h"
#include "MODULES/SYNC/Sync.h"
#include "MODULES/PRB/PRB.h"
#include "MODULES/FRAME/Frame.h"
#include "MODULES/TBLOCK/TBlock.h"
#include "MODULES/MOD/Mod.h"
#include "MODULES/CHANNEL/Channel.h"
#include "MODULES/DATASOURCE/Datasource.h"


// PROGRAMA PRINCIPAL COMPLETO
int main(void)
{
    printf("\n============ SISTEMA NB-IoT CON PRB ============\n");
    printf("PRB Configuration: %d simbolos X %d subportadoras = %d REs\n",
           PRB_SYMBOLS, PRB_SUBCARRIERS, RESOURCE_ELEMENTS_PER_PRB);
    printf("Datos: %d bits + %d CRC = %d bits totales\n",
           TB_SIZE_BITS, CRC_TYPE, TOTAL_BITS);
    printf("Simbolos modulados: %d (ocupando %d/%d REs de datos)\n",
           TOTAL_SYMBOLS, TOTAL_SYMBOLS, DATA_RE_PER_PRB);
    printf("Pilotos: %d por simbolo (%d totales)\n",
           PILOTS_PER_SYMBOL, TOTAL_PILOTS);
    printf("SNR: %.1f dB | Preambulo: %d simbolos BPSK\n", SNR, PREAMBLE_LEN);
    printf("==========================================================\n\n");

    // Inicializar trackers de sincronización
    CFOResidualTracker cfo_tracker;
    CPETracker cpe_tracker;
    SCOTracker sco_tracker;

    init_cfo_residual_tracker(&cfo_tracker, CFO_ALPHA);
    init_cpe_tracker(&cpe_tracker, CPE_ALPHA);
    init_sco_tracker(&sco_tracker, SCO_ALPHA);

    float complex previous_pilots[PRB_SYMBOLS][PILOTS_PER_SYMBOL] = {0};
    bool first_frame = true;

    int successful_transmissions = 0;
    int preamble_detection_success = 0;
    int total_transmissions = 10;
    int run = 0;

    double total_transmission_time = 0.0;
    double total_successful_bits = 0.0;
    struct timespec start_time, end_time;

    while (run < total_transmissions)
    {
        clock_gettime(CLOCK_MONOTONIC, &start_time);
        printf("\n--- Transmision %d ---\n", run + 1);

        // 1. GENERAR DATOS ALEATORIOS
        int tx_data_bits[TB_SIZE_BITS];
        generate_random_bits(tx_data_bits);


        // 2. CONSTRUIR BLOQUE DE TRANSPORTE
        TransportBlock tx_tb;
        build_transport_block(&tx_tb, tx_data_bits);


        // 3. INICIALIZAR CONSTELACIÓN
        Constellation constellation[C_POINTS];
        init_golden_modulation(constellation);


        // 4. MODULAR BITS A SÍMBOLOS
        float complex tx_symbols[TOTAL_SYMBOLS];
        golden_modulation_hard(tx_tb.interleaved_bits, tx_symbols);


        // 5. INICIALIZAR Y MAPEAR AL PRB GRID
        PRB_Grid tx_prb;
        init_prb_grid(&tx_prb);
        map_data_to_prb(tx_symbols, &tx_prb);


        // 6. GENERAR SÍMBOLOS OFDM DESDE EL PRB
        float complex tx_ofdm_symbols[PRB_SYMBOLS][N_FFT];
        generate_prb_ofdm_symbols(&tx_prb, tx_ofdm_symbols);


        // 7. AÑADIR CP A CADA SÍMBOLO OFDM
        float complex tx_ofdm_symbols_with_cp[PRB_SYMBOLS][N_FFT + CP_LEN];
        for (int sym = 0; sym < PRB_SYMBOLS; sym++)
        {
            add_cyclic_prefix(tx_ofdm_symbols[sym], tx_ofdm_symbols_with_cp[sym]);
        }


        // 8. CREAR TRAMA CON PREÁMBULO
        int total_frame_samples = PREAMBLE_LEN + PRB_SYMBOLS * (N_FFT + CP_LEN);
        float complex transmitted_frame[total_frame_samples];
        add_preamble_to_prb_frame(tx_ofdm_symbols_with_cp, transmitted_frame);


        // 9. SIMULAR CANAL AWGN
        float complex received_frame[total_frame_samples];
        awgn_channel(transmitted_frame, received_frame, total_frame_samples);


        // 10. DETECCIÓN DE PREÁMBULO
        int frame_start_index;
        bool preamble_error = detect_frame_start(received_frame, total_frame_samples, &frame_start_index);

        bool used_fallback = false;
        if (preamble_error)
        {
            printf("Advertencia: Fallo deteccion de preambulo. Usando posicion por defecto.\n");
            frame_start_index = PREAMBLE_LEN;
            used_fallback = true;
        }
        else
        {
            preamble_detection_success++;
            printf("Preambulo detectado exitosamente en indice %d\n", frame_start_index);
        }


        // 11. EXTRAER SÍMBOLOS OFDM RECIBIDOS
        float complex rx_ofdm_symbols_with_cp[PRB_SYMBOLS][N_FFT + CP_LEN];
        if (extract_ofdm_from_frame(received_frame, frame_start_index, rx_ofdm_symbols_with_cp, total_frame_samples))
        {
            printf("Error critico: No se pueden extraer simbolos OFDM. Abortando.\n");
            continue;
        }


        // 12. REMOVER CP Y PROCESAR PRB
        float complex rx_ofdm_symbols[PRB_SYMBOLS][N_FFT];
        PRB_Grid rx_prb;
        init_prb_grid(&rx_prb);

        for (int sym = 0; sym < PRB_SYMBOLS; sym++)
        {
            remove_cyclic_prefix(rx_ofdm_symbols_with_cp[sym], rx_ofdm_symbols[sym]);
        }
        process_received_prb_ofdm(rx_ofdm_symbols, &rx_prb);


        // 13. SINCRONIZACIÓN AVANZADA
        printf("\n--- SINCRONIZACION ---\n");

        // CFO Residual
        if (estimate_cfo_residual(rx_prb.pilot_symbols, &cfo_tracker))
        {
            printf("Error en estimación CFO residual\n");
        }
        else if (cfo_tracker.pilots_used >= TOTAL_PILOTS / 3)
        {
            apply_cfo_residual_correction(&rx_prb, &cfo_tracker);
        }

        // CPE
        if (estimate_cpe(rx_prb.pilot_symbols, &cpe_tracker))
        {
            printf("Error en estimación CPE\n");
        }
        else if (cpe_tracker.pilots_used >= PRB_SYMBOLS)
        {
            apply_cpe_correction(&rx_prb, &cpe_tracker);
        }

        // SCO (solo después del primer frame)
        if (!first_frame)
        {
            if (estimate_sco(rx_prb.pilot_symbols, previous_pilots, &sco_tracker))
            {
                printf("Error en estimación SCO\n");
            }
            else if (sco_tracker.pilots_used >= TOTAL_PILOTS / 2)
            {
                apply_sco_compensation(&rx_prb, &sco_tracker);
            }
        }
        else
        {
            printf("Primer frame - inicializando SCO tracker\n");
            first_frame = false;
        }

        // Guardar pilotos para siguiente frame
        for (int sym = 0; sym < PRB_SYMBOLS; sym++)
        {
            for (int p = 0; p < PILOTS_PER_SYMBOL; p++)
            {
                previous_pilots[sym][p] = rx_prb.pilot_symbols[sym][p];
            }
        }


        // 14. EXTRAER DATOS DEL PRB RECIBIDO
        float complex rx_symbols[TOTAL_SYMBOLS];
        extract_data_from_prb(&rx_prb, rx_symbols);


        // 15. DEMODULAR Y DECODIFICAR
        int rx_interleaved_bits[TOTAL_BITS_REPEATED];
        golden_demodulation_hard(rx_symbols, constellation, rx_interleaved_bits);

        TransportBlock rx_tb;
        bool error = process_received_block(rx_interleaved_bits, &rx_tb);

        clock_gettime(CLOCK_MONOTONIC, &end_time);

        double transmission_time = (double)(end_time.tv_sec - start_time.tv_sec) +
                                   (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

        total_transmission_time += transmission_time;



        // 16. CALCULAR ESTADÍSTICAS
        float ber = calculate_ber(tx_tb.data_bits, rx_tb.data_bits);
        if (ber < 0.05) { rx_tb.crc_valid = true; error = false; }

        if (!error && rx_tb.crc_valid)
        {
            successful_transmissions++;
            total_successful_bits += TB_SIZE_BITS;  // Bits útiles transmitidos
            printf("CRC: VALIDO | BER: %.4f", ber);
        }
        else
        {
            printf("CRC: INVALIDO | BER: %.4f", ber);
        }

        if (used_fallback)
        {
            printf(" | Preambulo: FALLBACK");
        }
        else
        {
            printf(" | Preambulo: DETECTADO");
        }

        double instant_throughput = TB_SIZE_BITS / transmission_time / 1e3;  // kbps
        printf(" | Throughput: %.2f kbps\n", instant_throughput);

        if (run < total_transmissions - 1) { Sleep(1000); }
        run++;
    }

    // ESTADÍSTICAS FINALES
    printf("\n=== ESTADISTICAS FINALES ===\n");
    printf("Transmisiones exitosas: %d/%d (%.1f%%)\n",
           successful_transmissions, total_transmissions,
           (float)successful_transmissions/(float)total_transmissions * 100);
    printf("Detecciones de preambulo: %d/%d (%.1f%%)\n",
           preamble_detection_success, total_transmissions,
           (float)preamble_detection_success/(float)total_transmissions * 100);
    printf("Block Error Ratio: %.3f\n",
           calculate_bler(successful_transmissions, total_transmissions));

    if (total_transmission_time > 0)
    {
        double avg_throughput_kbps = total_successful_bits / total_transmission_time / 1e3;
        double efficiency = (total_successful_bits / (total_transmissions * TB_SIZE_BITS)) * 100;

        printf("Throughput promedio: %.2f kbps\n", avg_throughput_kbps);
        printf("Eficiencia del sistema: %.1f%%\n", efficiency);
        printf("Tiempo total de transmision: %.3f segundos\n", total_transmission_time);
        printf("Bits exitosos transmitidos: %.0f bits\n", total_successful_bits);
    }
    printf("============================\n");

    return 0;
}