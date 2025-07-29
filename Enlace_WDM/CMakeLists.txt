cmake_minimum_required(VERSION 3.31)
project(Enlace_WDM C)

set(CMAKE_C_STANDARD 99)

add_executable(Enlace_WDM Enlace_WDM.c
        Functions/Enlace_WDM_functions.h
        Functions/GNU_plot.h
        Functions/fftw3.h)

#CONFIGURACIÓN FFTW3
set(FFTW3_DIR "C:/fftw3")
include_directories(${FFTW3_DIR})
link_directories(${FFTW3_DIR})
target_link_libraries(Enlace_WDM fftw3)