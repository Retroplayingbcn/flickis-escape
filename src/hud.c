// Marcador superior: fuente 8x16 con el fondo integrado (plano B) e iconos de objetos (plano A)
#include "juego.h"

static u32 cache_puntos = 0xFFFFFFFF;
static u16 cache_tiempo = 0xFFFF, cache_huevos = 0xFFFF, cache_vidas = 0xFFFF, cache_municion = 0xFFFF, cache_jefe = 0xFFFF;
static u8  cache_parpadeo = 0xFF;

static void hud_texto(const char* txt, u16 col) {
    for (u16 i = 0; txt[i] && col + i < 40; i++) {
        const char* p = HUD_CHARS; u16 idx = 0;
        char c = txt[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        for (u16 k = 0; p[k]; k++) if (p[k] == c) { idx = k; break; }
        VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(PAL0, 0, 0, 0, base_hud + idx * 2), col + i, 0);
        VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(PAL0, 0, 0, 0, base_hud + idx * 2 + 1), col + i, 1);
    }
}

static void hud_icono(u16 item, u16 col) {
    u16 t = base_items + item * 4;
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL2, 0, 0, 0, t),     col,     0);
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL2, 0, 0, 0, t + 1), col + 1, 0);
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL2, 0, 0, 0, t + 2), col,     1);
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL2, 0, 0, 0, t + 3), col + 1, 1);
}

static void hud_limpiar() {
    hud_texto("                                        ", 0);
    for (u16 x = 0; x < 40; x++) { VDP_setTileMapXY(BG_A, 0, x, 0); VDP_setTileMapXY(BG_A, 0, x, 1); }
}

void hud_forzar_redibujo() {
    cache_puntos = 0xFFFFFFFF; cache_tiempo = 0xFFFF; cache_huevos = 0xFFFF;
    cache_vidas = 0xFFFF; cache_parpadeo = 0xFF; cache_municion = 0xFFFF; cache_jefe = 0xFFFF;
    hud_limpiar();
}

void hud_mensaje(const char* txt) {
    hud_limpiar();
    hud_texto(txt, (40 - strlen(txt)) / 2);
}

// Distribución (40 columnas):
//   un jugador: [1-1] [huevo x3] [puntos] [reloj 3:00] [polluelo 00/10 | barra del jefe] [cara x1]
//   jefe:       [JEFE] [huevo x3] [reloj 2:30] [barra del jefe] [polluelo x3 = corazones] [cara x1]
//   versus:     [J1 n] [J2 n] [huevo x2] [reloj 3:00] [polluelo 00/10]
void hud_actualizar() {
    char buf[20];
    bool total = (cache_tiempo == 0xFFFF);
    u16 col_huevo = (modo_jugadores == 1) ? 12 : 4;
    u16 col_reloj = en_jefe ? 9 : 16;

    if (total) {
        hud_icono(ITEM_HUEVO, col_huevo);
        hud_icono(ITEM_RELOJ, col_reloj);
    }
    if (modo_jugadores == 1) {
        if (total) { hud_texto(T("J1", "P1"), 0); hud_texto(T("J2", "P2"), 5); hud_icono(ITEM_POLLUELO, 23); }
        if (cache_puntos != (u32)(victorias_j1 * 100 + victorias_j2)) {
            cache_puntos = victorias_j1 * 100 + victorias_j2;
            sprintf(buf, "%d", (u16)victorias_j1);  hud_texto(buf, 3);
            sprintf(buf, "%d", (u16)victorias_j2); hud_texto(buf, 8);
        }
    } else {
        if (total) {
            if (modo_juego_seleccionada == 1) hud_texto(T("LIB", "FRE"), 0);
            else if (en_jefe) hud_texto(T("JEFE", "BOSS"), 0);
            else { sprintf(buf, "%d-%d", mundo_actual + 1, fase_actual); hud_texto(buf, 0); }
            hud_icono(ITEM_POLLUELO, en_jefe ? 27 : 23);
            hud_icono(ITEM_CARA, 35);
        }
        if (!en_jefe && cache_puntos != flicky.puntuacion) {
            // el marcador "cuenta" hasta la puntuación real
            if (cache_puntos > flicky.puntuacion) cache_puntos = flicky.puntuacion;
            else {
                u32 dif = flicky.puntuacion - cache_puntos;
                cache_puntos += (dif > 2000) ? 100 : (dif > 400) ? 50 : (dif > 40) ? 10 : dif;
            }
            sprintf(buf, "%06ld", cache_puntos); hud_texto(buf, 9);
        }
        if (en_jefe) {
            s16 v = jefe_vida(), vm = jefe_vida_max();
            if (cache_jefe != (u16)(v * 16 + corazones)) {
                cache_jefe = v * 16 + corazones;
                for (s16 i = 0; i < vm && i < 10; i++) hud_texto(i < v ? "#" : "_", 16 + i);
                sprintf(buf, "x%d", corazones); hud_texto(buf, 29);
            }
        }
        if (cache_vidas != (u16)vidas) {
            cache_vidas = vidas;
            sprintf(buf, "x%-2d", vidas); hud_texto(buf, 37);
        }
    }
    if (!en_jefe) {
        u8 parpadeo = salida_abierta ? ((frame_counter >> 4) & 1) : 0;
        u16 huevos = total_salvados * 100 + max_polluelos_nivel;
        if (cache_huevos != huevos || cache_parpadeo != parpadeo) {
            cache_huevos = huevos; cache_parpadeo = parpadeo;
            if (salida_abierta && parpadeo) hud_texto(T("META!", "GOAL!"), 25);
            else { sprintf(buf, "%02d/%02d", total_salvados, max_polluelos_nivel); hud_texto(buf, 25); }
        }
    }
    if (cache_municion != municion) {
        cache_municion = municion;
        sprintf(buf, "x%d", municion); hud_texto(buf, col_huevo + 2);
    }
    // con poco tiempo el reloj parpadea
    u16 min = tiempo_restante_segundos / 60, seg = tiempo_restante_segundos - min * 60;
    u16 clave = tiempo_restante_segundos;
    bool oculto = (tiempo_restante_segundos <= 15) && ((frame_counter & 16) != 0);
    if (oculto) clave |= 0x8000;
    if (cache_tiempo != clave) {
        cache_tiempo = clave;
        if (oculto) strcpy(buf, "    ");
        else sprintf(buf, "%d:%02d", min, seg);
        hud_texto(buf, col_reloj + 2);
    }
}
