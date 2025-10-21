cmake_minimum_required(VERSION 3.31)
project(GAM C)

set(CMAKE_C_STANDARD 99)

add_executable (
        GAM main.c
        MODULES/Common.h
        MODULES/SYNC/Sync.h             MODULES/SYNC/Sync.c
        MODULES/PRB/PRB.h               MODULES/PRB/PRB.c
        MODULES/FRAME/Frame.h           MODULES/FRAME/Frame.c
        MODULES/TBLOCK/TBlock.h         MODULES/TBLOCK/TBlock.c
        MODULES/MOD/Mod.h               MODULES/MOD/Mod.c
        MODULES/CHANNEL/Channel.h       MODULES/CHANNEL/Channel.c
        MODULES/DATASOURCE/Datasource.h MODULES/DATASOURCE/Datasource.c
        MODULES/MOD/Bit_Mapping_GAM2.h
        MODULES/MOD/Bit_Mapping_GAM3.h
        MODULES/MOD/Bit_Mapping_GAM4.h
)
