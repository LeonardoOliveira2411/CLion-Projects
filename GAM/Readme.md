En esta carpeta se encuentra las diferentes versiones de mi proyecto de una cadena NB-IoT utilizando una modulación GAM (Golden Angle Modulation).
Las versiones estan formadas por los siguientes módulos:
- v1: DATASOURCE + MODULACION GAM + CANAL AWGN SIMULADO + DEMODULACION GAM + DATASINK (CÁLCULO BER)
- v2: v1 + DFT/IFT + OFDM
- v3: Nueva implementación de toda la cadena. Dispone de los siguientes módulos:
  - DATASOURCE + CRC + CODIFICACIÓN FEC REPETICIÓN + INTERLEAVER + MODULACIÓN GAM + OFDM TX + CANAL AWGN CON ERRORES DE RÁFAGA ALEATORIOS + OFDM RX + DEMODULACIÓN GAM + DEINTERLEAVER + DECODIFICACIÓN FEC REPETICIÓN + DATASINK (CÁLCULO BER + BLER)
