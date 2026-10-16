// Configuración en la SRAM del cartucho: idioma y récord
#include "juego.h"

#define SRAM_MAGIA 0xF1

void cargar_config() {
    SRAM_enableRO();
    if (SRAM_readByte(0) == SRAM_MAGIA) {
        idioma = SRAM_readByte(1) & 1;
        puntuacion_maxima = SRAM_readLong(2);
        if (puntuacion_maxima > 9999999) puntuacion_maxima = 0;
    }
    SRAM_disable();
}

void guardar_config() {
    SRAM_enable();
    SRAM_writeByte(0, SRAM_MAGIA);
    SRAM_writeByte(1, (u8)idioma);
    SRAM_writeLong(2, puntuacion_maxima);
    SRAM_disable();
}
