// Jefes de final de mundo (fase 6): el enemigo del mundo a doble tamaño, con barra de vida
// y un ataque propio. Se le vence a huevazos.
//
// Ritmo "arcade": el jefe avisa (un "!" sobre la cabeza) antes de cada ataque y después se queda
// cansado un segundo, que es el momento de darle. A mitad de vida se enfada (se pone rojo y acelera).
#include "juego.h"

enum { J_CAE, J_ENTRA, J_PERSIGUE, J_PREPARA, J_EMBISTE, J_RECUPERA, J_OCULTO, J_RABIA, J_RETROCEDE, J_PAUSA, J_DERROTA };

typedef struct {
    Sprite* s;
    Sprite* aviso;       // "!" antes de atacar / estrellitas cuando está cansado
    s16 fx, fy;          // posición en 1/16 px (esquina superior izquierda del sprite de 48x48)
    s16 vx, vy;          // velocidad en 1/16 px por frame
    s16 vida, vida_max;
    u16 estado;
    s16 t, parpadeo, huevos_t, objetos_t, jadeo_t;
    bool enfadado;
    u8 anim, aviso_tipo;
} Jefe;
static Jefe jefe;

#define JX (jefe.fx >> 4)
#define JY (jefe.fy >> 4)
#define JEFE_X0 (14 * CELDA - 16)       // donde aterriza
#define JEFE_Y0 (5 * CELDA - 8)

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
bool jefe_derrotado() { return jefe.s && jefe.estado == J_DERROTA; }
s16 jefe_vida_max() { return jefe.vida_max; }

static void aviso_quitar() {
    if (jefe.aviso) SPR_releaseSprite(jefe.aviso);
    jefe.aviso = NULL; jefe.aviso_tipo = 0;
}

// 1 = "!" (va a atacar), 2 = estrellitas (cansado)
static void aviso_poner(u8 tipo) {
    if (jefe.aviso_tipo == tipo) return;
    aviso_quitar();
    if (tipo == 1) {
        jefe.aviso = SPR_addSprite(&popup_sprite, 0, 0, TILE_ATTR(PAL2, 1, FALSE, FALSE));
        if (jefe.aviso) SPR_setAnim(jefe.aviso, POP_AVISO);
    } else {
        jefe.aviso = SPR_addSprite(&estrellas_sprite, 0, 0, TILE_ATTR(PAL2, 1, FALSE, FALSE));
    }
    if (jefe.aviso) SPR_setDepth(jefe.aviso, SPR_MIN_DEPTH);
    jefe.aviso_tipo = tipo;
}

void jefe_reset() {
    aviso_quitar();
    if (jefe.s) SPR_releaseSprite(jefe.s);
    jefe.s = NULL;
}

// tinte rojo progresivo sobre la paleta del jefe (0 = normal, 8 = rojo)
static void jefe_tinte(u16 nivel) {
    const u16* o = jefe_sprite_def()->palette->data;
    for (u16 i = 1; i < 16; i++) {
        u16 c = o[i];
        s16 r = (c >> 1) & 7, g = (c >> 5) & 7, b = (c >> 9) & 7;
        r = r + ((7 - r) * nivel) / 10;
        g = g - (g * nivel) / 14;
        b = b - (b * nivel) / 12;
        PAL_setColor(48 + i, (r << 1) | (g << 5) | (b << 9));
    }
}

void jefe_iniciar() {
    const SpriteDefinition* def = jefe_sprite_def();
    PAL_setPalette(PAL3, def->palette->data, CPU);
    jefe.fx = JEFE_X0 << 4; jefe.fy = (-64) << 4;    // empieza fuera de la pantalla: cae del cielo
    jefe.vx = 0; jefe.vy = 0;
    jefe.vida_max = 5 + d_sel * 2; jefe.vida = jefe.vida_max;
    jefe.estado = J_CAE; jefe.t = 0; jefe.parpadeo = 0;
    jefe.huevos_t = 60; jefe.objetos_t = 300; jefe.jadeo_t = 0;
    jefe.enfadado = FALSE; jefe.anim = 0xFF; jefe.aviso = NULL; jefe.aviso_tipo = 0;
    jefe.s = SPR_addSprite(def, JX, -64, TILE_ATTR(PAL3, 0, FALSE, TRUE));
    if (jefe.s) SPR_setVisibility(jefe.s, HIDDEN);
}

// La caída de la presentación: TRUE cuando ya ha aterrizado
bool jefe_caer() {
    if (!jefe.s) return TRUE;
    if (jefe.estado != J_CAE) return TRUE;
    SPR_setVisibility(jefe.s, VISIBLE);
    jefe.vy += 6;
    jefe.fy += jefe.vy;
    if (jefe.fy >= (JEFE_Y0 << 4)) {
        jefe.fy = JEFE_Y0 << 4; jefe.vy = 0;
        jefe.estado = J_ENTRA; jefe.t = 0;
        efecto_shake(20, 4);
        sonido(SFX_GOLPE);
        fx_polvo(JX + 4, JY + 32); fx_polvo(JX + 28, JY + 32);
        SPR_setPosition(jefe.s, JX + shake_x, JY + PLAY_Y);
        return TRUE;
    }
    SPR_setPosition(jefe.s, JX, JY + PLAY_Y);
    return FALSE;
}

static bool furioso() { return jefe.enfadado; }

// caja de colisión con los muros
static bool choca_muro(s16 x, s16 y) {
    s16 x0 = x + 10, x1 = x + 37, y0 = y + 14, y1 = y + 45;
    return hay_muro(x0 >> CELDA_SHIFT, y0 >> CELDA_SHIFT) || hay_muro(x1 >> CELDA_SHIFT, y0 >> CELDA_SHIFT) ||
           hay_muro(x0 >> CELDA_SHIFT, y1 >> CELDA_SHIFT) || hay_muro(x1 >> CELDA_SHIFT, y1 >> CELDA_SHIFT);
}

// el cuerpo que hace daño es más pequeño que el sprite (unos 26x30): rozar el borde no mata
bool jefe_toca(s16 x, s16 y, s16 radio) {
    if (!jefe.s || jefe.estado == J_CAE || jefe.estado == J_DERROTA || jefe.estado == J_OCULTO) return FALSE;
    s16 cx = JX + 24, cy = JY + 30;
    return abs(x - cx) < 13 + radio && abs(y - cy) < 15 + radio;
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

bool jefe_impacto(s16 x, s16 y) {
    if (!jefe_toca(x, y, 2)) return FALSE;
    if (jefe.parpadeo > 0 || jefe.estado == J_RABIA) return TRUE;   // invulnerable un momento
    jefe.vida--;
    jefe.parpadeo = 50;
    sonido(SFX_GOLPE);
    sumar_puntos(200);
    fx_popup(JX + 8, JY + 4, POP_200);
    fx_estallido(x - 8, y - 8);
    efecto_shake(10, 2);
    hud_actualizar();
    if (jefe.vida <= 0) {
        aviso_quitar();
        jefe.estado = J_DERROTA; jefe.t = 0;
        musica_detener();
        sonido(SFX_EXPLOSION);
        // golpe final: todo se congela un instante con un destello, y Flicki se detiene
        flicky.x = ((flicky.x + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
        flicky.y = ((flicky.y + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
        flicky.x_vel = 0; flicky.y_vel = 0;
        SPR_setAnim(flicky.sprite, ANIM_ABAJO); flicky.anim = ANIM_ABAJO; SPR_setFrame(flicky.sprite, 0);
        fx_destello();
        for (u16 f = 0; f < 20; f++) { fx_scroll_actualizar(); SPR_update(); SYS_doVBlankProcess(); }
        efecto_shake(20, 4);
        return TRUE;
    }
    if (!jefe.enfadado && jefe.vida * 2 <= jefe.vida_max) {
        // a media vida se enfada: ruge, se pone rojo y a partir de ahora va más rápido
        aviso_quitar();
        jefe.enfadado = TRUE;
        jefe.estado = J_RABIA; jefe.t = 0;
        sonido(SFX_RABIA);
        efecto_shake(30, 3);
        return TRUE;
    }
    // retrocede del golpe (si estaba cansado, sigue cansado: así se le pueden encadenar golpes)
    if (jefe.estado != J_RECUPERA) {
        aviso_quitar();
        s16 dx = (JX + 24) - (flicky.x + 8), dy = (JY + 30) - (flicky.y + 8);
        s16 m = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
        jefe.vx = m ? (dx * 40) / m : 0; jefe.vy = m ? (dy * 40) / m : 0;
        jefe.estado = J_RETROCEDE; jefe.t = 0;
    }
    return TRUE;
}

static void disparar_abanico(u16 tipo, s16 v, u16 n) {
    s16 vx, vy;
    apuntar(v, &vx, &vy);
    s16 ox = JX + 16, oy = JY + 20;
    disparo_jefe(tipo, ox, oy, vx, vy, 200);
    if (n >= 3) { disparo_jefe(tipo, ox, oy, vx - vy / 3, vy + vx / 3, 200); disparo_jefe(tipo, ox, oy, vx + vy / 3, vy - vx / 3, 200); }
    if (n >= 5) { disparo_jefe(tipo, ox, oy, vx - (vy * 2) / 3, vy + (vx * 2) / 3, 200); disparo_jefe(tipo, ox, oy, vx + (vy * 2) / 3, vy - (vx * 2) / 3, 200); }
}

static void teletransportar() {
    for (u16 i = 0; i < 40; i++) {
        s16 x = 16 + (random() % (MAP_WIDTH * CELDA - 80)), y = 8 + (random() % (MAP_HEIGHT * CELDA - 72));
        if (choca_muro(x, y)) continue;
        if (abs(x + 24 - flicky.x) + abs(y + 30 - flicky.y) < 120) continue;
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
    if (jefe.vx && jefe.estado != J_RETROCEDE) SPR_setHFlip(jefe.s, jefe.vx < 0);
    bool quieto = (jefe.vx == 0 && jefe.vy == 0);
    if ((frame_counter % (jefe.estado == J_EMBISTE ? 3 : 7)) == 0 && !(quieto && jefe.estado == J_RECUPERA)) SPR_nextFrame(jefe.s);
    s16 temblor = ((jefe.estado == J_PREPARA || jefe.estado == J_RABIA) && (frame_counter & 2)) ? 2 : 0;
    SPR_setPosition(jefe.s, JX + shake_x + temblor, JY + PLAY_Y);
    bool oculto = (jefe.estado == J_OCULTO) || (jefe.parpadeo > 0 && (jefe.parpadeo & 4));
    SPR_setVisibility(jefe.s, oculto ? HIDDEN : VISIBLE);
    if (jefe.aviso) {
        if (jefe.aviso_tipo == 1) SPR_setPosition(jefe.aviso, JX + 8 + shake_x, JY + PLAY_Y - 2 + ((frame_counter & 8) ? -2 : 0));
        else {
            SPR_setPosition(jefe.aviso, JX + 12 + shake_x, JY + PLAY_Y + 2);
            if ((frame_counter & 3) == 0) SPR_nextFrame(jefe.aviso);
        }
        SPR_setVisibility(jefe.aviso, oculto && jefe.estado == J_OCULTO ? HIDDEN : VISIBLE);
    }
}

// la arena se va llenando de huevos y, de vez en cuando, de algo que ayude
static void repartir_ayudas() {
    if (--jefe.huevos_t <= 0) {
        jefe.huevos_t = 90;
        u16 n = 0;
        for (s16 y = 1; y < MAP_HEIGHT - 1; y++) for (s16 x = 1; x < MAP_WIDTH - 1; x++) if (mapa[y][x] == TILE_HUEVO) n++;
        s16 hx, hy;
        if (n < 3 && buscar_celda_libre(&hx, &hy, 3)) objeto_colocar(hx, hy, TILE_HUEVO, FALSE);
    }
    if (--jefe.objetos_t <= 0) {
        jefe.objetos_t = 330 + (random() % 120);
        s16 ox, oy;
        if (!buscar_celda_libre(&ox, &oy, 3)) return;
        u16 r = random() % 100;
        u8 tipo;
        if (corazones < 3 && r < 35) tipo = TILE_POLLUELO;           // un polluelo vuelve a protegerte
        else if (r < 60) tipo = TILE_RELOJ;
        else if (r < 85) tipo = TILE_RAYO;
        else tipo = TILE_GEMA;
        objeto_colocar(ox, oy, tipo, TRUE);
    }
}

static s16 velocidad_persecucion(bool rabia) {
    s16 v;
    switch (mundo_actual) {
        case MUNDO_VOLCAN:  v = rabia ? 14 : 11; break;
        case MUNDO_MARINO:  v = rabia ? 9 : 7; break;
        case MUNDO_CELESTE: v = rabia ? 12 : 9; break;
        default:            v = rabia ? 13 : 10; break;
    }
    if (timer_cazador_lento > 0) v >>= 1;                            // el reloj también frena al jefe
    return v;
}

void jefe_actualizar() {
    if (!jefe.s || jefe.estado == J_CAE) return;
    if (jefe.parpadeo > 0) jefe.parpadeo--;
    jefe.t++;
    bool rabia = furioso();
    bool lento = timer_cazador_lento > 0;
    if (jefe.estado != J_DERROTA) repartir_ayudas();

    switch (jefe.estado) {
        case J_ENTRA:                                     // recién aterrizado: ruge y mira, sin atacar
            jefe.vx = 0; jefe.vy = 0;
            if (jefe.t == 20) { sonido(SFX_RABIA); efecto_shake(12, 2); }
            if (jefe.t > 70) { jefe.estado = J_PERSIGUE; jefe.t = 0; }
            break;

        case J_PERSIGUE: {
            apuntar(velocidad_persecucion(rabia), &jefe.vx, &jefe.vy);
            mover(jefe.vx, jefe.vy);
            if (mundo_actual == MUNDO_VOLCAN) {          // va dejando un rastro de fuego... y se cansa
                if ((jefe.t % (rabia ? 20 : 28)) == 0) disparo_jefe(DISPARO_FUEGO, JX + 16, JY + 30, 0, 0, 150);
                if (jefe.t > (rabia ? 170 : 210)) { jefe.estado = J_PREPARA; jefe.t = 0; sonido(SFX_RED); }
            } else {
                s16 espera = (mundo_actual == MUNDO_MARINO) ? 110 : (mundo_actual == MUNDO_CELESTE) ? (rabia ? 90 : 120) : (rabia ? 95 : 130);
                if (lento) espera += 60;
                if (jefe.t > espera) { jefe.estado = J_PREPARA; jefe.t = 0; sonido(SFX_RED); }
            }
            break;
        }

        case J_PREPARA:                                   // aviso claro: tiembla con un "!" encima
            jefe.vx = 0; jefe.vy = 0;
            aviso_poner(1);
            if (jefe.t > (rabia ? 36 : 48) + (lento ? 20 : 0)) {
                jefe.t = 0;
                aviso_quitar();
                if (mundo_actual == MUNDO_MARINO) { apuntar(56, &jefe.vx, &jefe.vy); jefe.estado = J_EMBISTE; }
                else if (mundo_actual == MUNDO_CELESTE) { jefe.estado = J_OCULTO; fx_estallido(JX + 16, JY + 16); sonido(SFX_TELEPORT); }
                else if (mundo_actual == MUNDO_VOLCAN) {  // llamarada en abanico y a descansar
                    disparar_abanico(DISPARO_BOLA, 30, rabia ? 5 : 3); sonido(SFX_LANZAR);
                    jefe.estado = J_RECUPERA;
                } else {
                    disparar_abanico(DISPARO_RED, 34, rabia ? 3 : 1); sonido(SFX_LANZAR);
                    jefe.estado = J_PAUSA;                // el cazador no se cansa: un gesto breve y vuelve a la carga
                }
            }
            break;

        case J_EMBISTE:                                   // tiburón: a toda velocidad hasta chocar
            if (mover(jefe.vx, jefe.vy) || jefe.t > 80) {
                jefe.estado = J_RECUPERA; jefe.t = 0;
                efecto_shake(14, 3);
                sonido(SFX_GOLPE);
                fx_estallido(JX + 16, JY + 24);
            }
            break;

        case J_RECUPERA: {                                // cansado: el mejor momento para darle
            jefe.vx = 0; jefe.vy = 0;
            aviso_poner(2);
            s16 dura = (mundo_actual == MUNDO_MARINO) ? 80 : (rabia ? 50 : 65);
            if (jefe.t > dura) { aviso_quitar(); jefe.estado = J_PERSIGUE; jefe.t = 0; }
            break;
        }

        case J_PAUSA:
            jefe.vx = 0; jefe.vy = 0;
            if (jefe.t > 24) { jefe.estado = J_PERSIGUE; jefe.t = 0; }
            break;

        case J_RETROCEDE:                                 // empujado por el huevazo
            mover(jefe.vx, jefe.vy);
            jefe.vx = (jefe.vx * 7) / 8; jefe.vy = (jefe.vy * 7) / 8;
            if (jefe.t > 16) { jefe.vx = 0; jefe.vy = 0; jefe.estado = J_PERSIGUE; jefe.t = 0; }
            break;

        case J_RABIA:                                     // se enfada: rojo, temblando, invulnerable un momento
            jefe.vx = 0; jefe.vy = 0;
            jefe_tinte((jefe.t < 40) ? ((jefe.t >> 2) & 1 ? 9 : 4) : 5);
            if ((jefe.t & 15) == 0) fx_chispa(JX + (random() % 32), JY + (random() % 24));
            if (jefe.t > 70) { jefe.estado = J_PERSIGUE; jefe.t = 0; }
            break;

        case J_OCULTO:                                    // diablillo: desaparece y vuelve lejos, disparando
            jefe.vx = 0; jefe.vy = 0;
            if (jefe.t == 1) teletransportar();
            if (jefe.t > 30) {
                jefe.estado = J_RECUPERA; jefe.t = 0;
                fx_estallido(JX + 16, JY + 16);
                disparar_abanico(DISPARO_BOLA, 28, rabia ? 5 : 3);
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
