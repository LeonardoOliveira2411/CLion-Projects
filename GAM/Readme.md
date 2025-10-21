En esta carpeta se encuentra las diferentes versiones de mi proyecto de una cadena NB-IoT, programada en C, utilizando una modulación GAM (Golden Angle Modulation).
En este repositorio se encuentran las siguientes versiones:
- v1: DATASOURCE + MODULACION GAM + CANAL AWGN SIMULADO + DEMODULACION GAM + DATASINK (CÁLCULO BER)
  
- v2: v1 + DFT/IFT + OFDM
  
- v3: Nueva implementación de toda la cadena. Dispone de los siguientes módulos:
  - DATASOURCE + CRC + CODIFICACIÓN FEC REPETICIÓN + INTERLEAVER + MODULACIÓN GAM + OFDM TX (SC) + CANAL AWGN CON ERRORES DE RÁFAGA ALEATORIOS + OFDM RX (SC) + DEMODULACIÓN GAM + DEINTERLEAVER + DECODIFICACIÓN FEC REPETICIÓN + DATASINK (CÁLCULO BER + BLER)

- v4: v3 + PREÁMBULO + DETECCIÓN DE TRAMA
  
- v5: v4 + PILOTOS OFDM + CORRECIÓN CFO
  
- v6: v5 + SINCRONIZACIÓN FINA (CFO RESIDUAL + CPE + SCO)

- v7: v6 + IMPLEMENTACIÓN DE PRBs (Physical Resource Blocks)

- v8: v7 ORGANIZADO (CÓDIGO SEPARADO EN main.c + ARCHIVOS .h y .c SEGÚN EL MÓDULO QUE REPRESENTAN EN LA CADENA)
