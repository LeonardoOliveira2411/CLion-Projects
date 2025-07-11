cmake_minimum_required(VERSION 3.31)
project(5G_CHAIN C)

set(CMAKE_C_STANDARD 99)

add_executable(5G_CHAIN
               5G_CHAIN.c
               5G_DATASOURCE/5G_DATASOURCE_functions.h
               5G_DATASOURCE/5G_DATASOURCE_functions.c
               5G_MOD_QAM/5G_MOD_QAM_functions.h
               5G_MOD_QAM/5G_MOD_QAM_functions.c
               5G_MOD_GOLDEN/5G_MOD_GAM_functions.h
               5G_MOD_GOLDEN/5G_MOD_GAM_functions.c
               5G_CHANNEL_DIST_NOISE/5G_CHANNEL_DIST_NOISE_functions.h
               5G_CHANNEL_DIST_NOISE/5G_CHANNEL_DIST_NOISE_functions.c
               5G_MOD_GOLDEN/5G_GAM_MAPPING.h
               5G_MOD_GOLDEN/5G_GAM_MAPPING.c
        GNUPLOT/plot.h
        GNUPLOT/plot.c
)
