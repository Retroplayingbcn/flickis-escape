// Jefes de final de mundo (fase 6): el enemigo del mundo a doble tamaño, con barra de vida
// y un ataque propio. Se le vence a huevazos; los huevos van apareciendo por la arena.
#include "juego.h"

enum { J_ENTRA, J_PERSIGUE, J_PREPARA, J_EMBISTE, J_RECUPERA, J_OCULTO, J_DERROTA };

typedef struct {
    Sprite* s;
    s16 fx, fy;          // posición en 1/16 px (esquina superior izquierda del sprite de 48x48)
    s16 vx, vy;          // velocidad en 1/16 px por frame
    s16 vida, vida_max;
    u16 estado;
    s16 t, parpadeo, huevos_t;
    u8 anim;
} Jefe;
static Jefe jefe;

#define JX (jefe.fx >> 4)
#define JY (jefe.fy >> 4)

const char* jefe_nombre() {
    switch (mundo_actual) {
        case MUNDO_VOLCAN:  return T("LA REINA LLAMA", "THE FLAME QUEEN");
        case MUNDO_MARINO:  return T("EL GRAN TIBURON", "THE GREAT SHARK");
        case MUNDO_CELESTE: return T("EL REY DIABLILLO", "THE IMP KING");
    }
    return T("EL GRAN CAZADOR", "THE GREAT HUNTER");
}

static const SpriteDefinition* jefe_sprite_def() {
    switch (mundo_actual) {
        case MUNDO_VOLCAN:  return &jefe_llama;
        case MUNDO_MARINO:  return &jefe_tiburon;
        case MUNDO_CELESTE: return &jefe_diablillo;
    }
    return &jefe_cazador;
}

s16 jefe_vida()     { return jefe.vida; }
s16 jefe_vida_max() { return jefe.vida_max; }

void jefe_reset() {
    if (jefe.s) SPR_releaseSprite(jefe.s);
    jefe.s = NULL;
}

void jefe_iniciar() {
    const SpriteDefinition* def = jefe_sprite_def();
    PAL_setPalette(PAL3, def->palette->data, CPU);
    jefe.fx = (14 * CELDA - 16) << 4; jefe.fy = (5 * CELDA - 8) << 4;
    jefe.vx = 0; jefe.vy = 0;
    jefe.vida_max = 6 + d_sel * 2; jefe.vida = jefe.vida_max;
    jefe.estado = J_ENTRA; jefe.t = 0; jefe.parpadeo = 0; jefe.huevos_t = 60; jefe.anim = 0xFF;
    jefe.s = SPR_addSprite(def, JX, JY + PLAY_Y, TILE_ATTR(PAL3, 0, FALSE, TRUE));
}

static bool furioso() { return jefe.vida * 2 <= jefe.vida_max; }

// caja de colisión del cuerpo (más pequeña que el sprite)
static bool choca_muro(s16 x, s16 y) {
    s16 x0 = x + 10, x1 = x + 37, y0 = y + 14, y1 = y + 45;
    return hay_muro(x0 >> CELDA_SHIFT, y0 >> CELDA_SHIFT) || hay_muro(x1 >> CELDA_SHIFT, y0 >> CELDA_SHIFT) ||
           hay_muro(x0 >> CELDA_SHIFT, y1 >> CELDA_SHIFT) || hay_muro(x1 >> CELDA_SHIFT, y1 >> CELDA_SHIFT);
}

bool jefe_toca(s16 x, s16 y, s16 radio) {
    if (!jefe.s || jefe.estado == J_DERROTA || jefe.estado == J_OCULTO) return FALSE;
    s16 cx = JX + 24, cy = JY + 30;
    return abs(x - cx) < 14 + radio && abs(y - cy) < 16 + radio;
}

bool jefe_impacto(s16 x, s16 y) {
    if (!jefe_toca(x, y, 2)) return FALSE;
    if (jefe.parpadeo > 0) return TRUE;                  // invulnerable un momento tras cada golpe
    jefe.vida--;
    jefe.parpadeo = 40;
    sonido(SFX_GOLPE);
    sumar_puntos(200);
    fx_popup(JX + 16, JY + 4, POP_200);
    fx_estallido(x - 8, y - 8);
    efecto_shake(10, 2);
    if (jefe.vida <= 0) {
        jefe.estado = J_DERROTA; jefe.t = 0;
        musica_detener();
        sonido(SFX_EXPLOSION);
        fx_destello();
    }
    return TRUE;
}

// velocidad hacia el jugador con módulo aproximado 'v' (1/16 px)
static void apuntar(s16 v, s16* ovx, s16* ovy) {
    s16 dx = (flicky.x + 8) - (JX + 24), dy = (flicky.y + 8) - (JY + 30);
    s16 m = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    if (m == 0) { *ovx = 0; *ovy = 0; return; }
    *ovx = (dx * v) / m; *ovy = (dy * v) / m;
}

static bool mover(s16 vx, s16 vy) {
    bool choque = FALSE;
    s16 nx = jefe.fx + vx;
    if (!choca_muro(nx >> 4, JY)) jefe.fx = nx; else choque = TRUE;
    s16 ny = jefe.fy + vy;
    if (!choca_muro(JX, ny >> 4)) jefe.fy = ny; else choque = TRUE;
    return choque;
}

static void disparar_abanico(u16 tipo, s16 v, u16 n) {
    s16 vx, vy;
    apuntar(v, &vx, &vy);
    s16 ox = JX + 16, oy = JY + 20;
    disparo_jefe(tipo, ox, oy, vx, vy, 240);
    if (n >= 3) { disparo_jefe(tipo, ox, oy, vx - vy / 3, vy + vx / 3, 240); disparo_jefe(tipo, ox, oy, vx + vy / 3, vy - vx / 3, 240); }
    if (n >= 5) { disparo_jefe(tipo, ox, oy, vx - (vy * 2) / 3, vy + (vx * 2) / 3, 240); disparo_jefe(tipo, ox, oy, vx + (vy * 2) / 3, vy - (vx * 2) / 3, 240); }
}

static void teletransportar() {
    for (u16 i = 0; i < 40; i++) {
        s16 x = 16 + (random() % (MAP_WIDTH * CELDA - 80)), y = 8 + (random() % (MAP_HEIGHT * CELDA - 72));
        if (choca_muro(x, y)) continue;
        if (abs(x + 24 - flicky.x) + abs(y + 30 - flicky.y) < 110) continue;
        jefe.fx = x << 4; jefe.fy = y << 4;
        return;
    }
}

static void animar() {
    u8 a = jefe.anim;
    if (abs(jefe.vx) > abs(jefe.vy)) a = ANIM_LADO;
    else if (jefe.vy > 0) a = ANIM_ABAJO;
    else if (jefe.vy < 0) a = ANIM_ARRIBA;
    else if (a == 0xFF) a = ANIM_ABAJO;
    if (a != jefe.anim) { SPR_setAnim(jefe.s, a); jefe.anim = a; }
    if (jefe.vx) SPR_setHFlip(jefe.s, jefe.vx < 0);
    if ((frame_counter % (jefe.estado == J_EMBISTE ? 3 : 7)) == 0) SPR_nextFrame(jefe.s);
    s16 temblor = (jefe.estado == J_PREPARA && (frame_counter & 2)) ? 2 : 0;
    SPR_setPosition(jefe.s, JX + shake_x + temblor, JY + PLAY_Y);
    bool oculto = (jefe.estado == J_OCULTO) || (jefe.parpadeo > 0 && (jefe.parpadeo & 4));
    SPR_setVisibility(jefe.s, oculto ? HIDDEN : VISIBLE);
}

void jefe_actualizar() {
    if (!jefe.s) return;
    if (jefe.parpadeo > 0) jefe.parpadeo--;
    jefe.t++;

    // van apareciendo huevos por la arena (como mucho dos en el suelo)
    if (jefe.estado != J_DERROTA && --jefe.huevos_t <= 0) {
        jefe.huevos_t = 140;
        u16 n = 0;
        for (s16 y = 1; y < MAP_HEIGHT - 1; y++) for (s16 x = 1; x < MAP_WIDTH - 1; x++) if (mapa[y][x] == TILE_HUEVO) n++;
        s16 hx, hy;
        if (n < 2 && buscar_celda_libre(&hx, &hy, 4)) objeto_colocar(hx, hy, TILE_HUEVO, FALSE);
    }

    bool rabia = furioso();
    switch (jefe.estado) {
        case J_ENTRA:
            jefe.vx = 0; jefe.vy = 0;
            if (jefe.t > 30) { jefe.estado = J_PERSIGUE; jefe.t = 0; }
            break;

        case J_PERSIGUE: {
            s16 v = (mundo_actual == MUNDO_VOLCAN) ? (rabia ? 22 : 16) : (mundo_actual == MUNDO_MARINO) ? 9 :
                    (mundo_actual == MUNDO_CELESTE) ? (rabia ? 16 : 12) : (rabia ? 18 : 14);
            apuntar(v, &jefe.vx, &jefe.vy);
            mover(jefe.vx, jefe.vy);
            if (mundo_actual == MUNDO_VOLCAN) {          // va dejando un rastro de fuego
                if ((jefe.t % (rabia ? 16 : 24)) == 0) disparo_jefe(DISPARO_FUEGO, JX + 16, JY + 30, 0, 0, 180);
            } else {
                s16 espera = (mundo_actual == MUNDO_MARINO) ? 120 : (mundo_actual == MUNDO_CELESTE) ? (rabia ? 80 : 120) : (rabia ? 100 : 150);
                if (jefe.t > espera) {
                    jefe.t = 0;
                    if (mundo_actual == MUNDO_CELESTE) { jefe.estado = J_OCULTO; fx_estallido(JX + 16, JY + 16); sonido(SFX_TELEPORT); }
                    else { jefe.estado = J_PREPARA; sonido(SFX_RED); }
                }
            }
            break;
        }

        case J_PREPARA:                                    // aviso: tiembla antes de atacar
            jefe.vx = 0; jefe.vy = 0;
            if (jefe.t > ((mundo_actual == MUNDO_MARINO) ? (rabia ? 25 : 40) : 30)) {
                jefe.t = 0;
                if (mundo_actual == MUNDO_MARINO) { apuntar(64, &jefe.vx, &jefe.vy); jefe.estado = J_EMBISTE; }
                else { disparar_abanico(DISPARO_RED, 40, rabia ? 3 : 1); sonido(SFX_LANZAR); jefe.estado = J_PERSIGUE; }
            }
            break;

        case J_EMBISTE:                                    // tiburón: a toda velocidad hasta chocar
            if (mover(jefe.vx, jefe.vy) || jefe.t > 90) {
                jefe.estado = J_RECUPERA; jefe.t = 0;
                efecto_shake(14, 3);
                sonido(SFX_GOLPE);
                fx_estallido(JX + 16, JY + 24);
            }
            break;

        case J_RECUPERA:                                   // mareado tras el choque: el mejor momento para darle
            jefe.vx = 0; jefe.vy = 0;
            if ((jefe.t & 15) == 0) fx_chispa(JX + 16, JY - 4);
            if (jefe.t > 70) { jefe.estado = J_PERSIGUE; jefe.t = 0; }
            break;

        case J_OCULTO:                                     // diablillo: desaparece y vuelve lejos, disparando
            jefe.vx = 0; jefe.vy = 0;
            if (jefe.t == 1) teletransportar();
            if (jefe.t > 24) {
                jefe.estado = J_PERSIGUE; jefe.t = 0;
                fx_estallido(JX + 16, JY + 16);
                disparar_abanico(DISPARO_BOLA, 32, rabia ? 5 : 3);
                sonido(SFX_LANZAR);
            }
            break;

        case J_DERROTA:
            jefe.vx = 0; jefe.vy = 0;
            if ((jefe.t & 7) == 0) { fx_estallido(JX + (random() % 32), JY + (random() % 32)); efecto_shake(8, 3); }
            if ((jefe.t & 31) == 0) sonido(SFX_EXPLOSION);
            jefe.parpadeo = 8;
            if (jefe.t > 110) {
                SPR_setVisibility(jefe.s, HIDDEN);
                fase_superada(TRUE);
                return;
            }
            break;
    }
    animar();
}
