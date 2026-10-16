// El enemigo de cada mundo: persecución por el laberinto (BFS) con personalidad propia
//   cazador (bosque): perseguidor equilibrado
//   llama (volcán): más rápida pero más alocada
//   tiburón (mar): si te ve en línea recta, embiste a toda velocidad
//   diablillo (cielo): el más listo; de vez en cuando se teletransporta
#include "juego.h"

static Sprite* estrellas_c = NULL;
static Sprite* barra_red = NULL;          // versus: recarga de la red sobre la cabeza
static s16 destello_listo = 0;            // versus: parpadeo blanco cuando la red vuelve a estar lista

const SpriteDefinition* enemigo_del_mundo() {
    switch (mundo_actual) {
        case MUNDO_VOLCAN:  return &flame_sprite;
        case MUNDO_MARINO:  return &shark_sprite;
        case MUNDO_CELESTE: return &imp_sprite;
    }
    return &hunter_sprite;
}

void cazador_crear(s16 cx, s16 cy) {
    cazador.x = cx * CELDA; cazador.y = cy * CELDA;
    cazador.x_vel = 0; cazador.y_vel = 0;
    cazador.def = enemigo_del_mundo();
    cazador.anim = 0xFF; cazador.acumulador = 0;
    cazador.aturdido = 0; cazador.inmune = 0; cazador.embestida = 0; cazador.recarga = 120;
    cazador.tele_timer = 600; cazador.oculto = 0;
    PAL_setPalette(PAL3, cazador.def->palette->data, CPU);
    cazador.sprite = SPR_addSprite(cazador.def, cazador.x - 4, cazador.y + PLAY_Y - 8, TILE_ATTR(PAL3, 0, FALSE, FALSE));
    estrellas_c = NULL;
    cazador_tinte_actualizar(TRUE);
}

void cazador_liberar() {
    if (estrellas_c) SPR_releaseSprite(estrellas_c);
    estrellas_c = NULL;
    if (barra_red) SPR_releaseSprite(barra_red);
    barra_red = NULL; destello_listo = 0;
}

bool cazador_peligroso() { return cazador.aturdido == 0 && cazador.oculto == 0; }

// un huevo le da: se queda aturdido; si estaba en rabia, se le pasa
void cazador_aturdir() {
    if (cazador.aturdido > 0 || cazador.inmune > 0 || cazador.oculto > 0) { sonido(SFX_CHOF); return; }
    cazador.aturdido = (modo_jugadores == 1) ? 120 : 180;     // contra una persona, 2 s
    cazador.embestida = 0;
    cazador.x = ((cazador.x + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
    cazador.y = ((cazador.y + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
    cazador.x_vel = 0; cazador.y_vel = 0;
    if (cazador_en_furia) { cazador_en_furia = FALSE; muros_alerta(FALSE); }
    sumar_puntos(100);
    fx_popup(cazador.x, cazador.y - 8, POP_100);
    fx_estallido(cazador.x, cazador.y);
    sonido(SFX_ATURDIDO);
    efecto_shake(6, 1);
}

// segundo jugador (modo versus)
static s16 mira_x = -1, mira_y = 0;                   // última dirección del cazador humano

// la red: recta, frena en los muros, con recarga; cada enemigo lanza lo suyo
static void lanzar_red() {
    if (cazador.recarga > 0 || cazador.aturdido > 0) { if (cazador.recarga > 0) sonido(SFX_CLICK); return; }
    u16 tipo = (mundo_actual == MUNDO_BOSQUE) ? DISPARO_RED : DISPARO_BOLA;
    disparo_jefe(tipo, cazador.x, cazador.y, mira_x * 40, mira_y * 40, 48);
    cazador.recarga = RECARGA_RED;
    if (!barra_red) {
        barra_red = SPR_addSprite(&recarga_sprite, cazador.x, cazador.y + PLAY_Y - 18, TILE_ATTR(PAL2, 1, FALSE, FALSE));
        if (barra_red) SPR_setDepth(barra_red, SPR_MIN_DEPTH);
    }
    sonido(SFX_RED);
}

void cazador_entrada_humana(u16 joy2, u16 pulsado2) {
    if (cazador.x_vel || cazador.y_vel) { mira_x = cazador.x_vel; mira_y = cazador.y_vel; }
    if (cazador.aturdido > 0) return;
    if (pulsado2 & (BUTTON_A | BUTTON_B)) lanzar_red();
    s16 ox = cazador.x & CELDA_MASK, oy = cazador.y & CELDA_MASK;
    if (!((ox <= 3 || ox >= 13) && (oy <= 3 || oy >= 13))) return;
    s16 nx = 0, ny = 0;
    if (joy2 & BUTTON_LEFT) nx = -1;
    else if (joy2 & BUTTON_RIGHT) nx = 1;
    else if (joy2 & BUTTON_UP) ny = -1;
    else if (joy2 & BUTTON_DOWN) ny = 1;
    if ((nx == 0 && ny == 0) || (nx == cazador.x_vel && ny == cazador.y_vel)) return;
    s16 cx = (cazador.x + 8) >> CELDA_SHIFT, cy = (cazador.y + 8) >> CELDA_SHIFT;
    if (hay_muro(cx + nx, cy + ny)) return;
    cazador.x = cx << CELDA_SHIFT; cazador.y = cy << CELDA_SHIFT;
    cazador.x_vel = nx; cazador.y_vel = ny;
}

// ------------------------------------------------------------------ IA
static bool linea_libre(s16 x0, s16 y0, s16 x1, s16 y1) {
    s16 sx = (x1 > x0) - (x1 < x0), sy = (y1 > y0) - (y1 < y0);
    while (x0 != x1 || y0 != y1) { x0 += sx; y0 += sy; if (hay_muro(x0, y0)) return FALSE; }
    return TRUE;
}

static void decidir() {
    static const s16 ddx[4] = { 1, -1, 0, 0 }, ddy[4] = { 0, 0, 1, -1 };
    s16 tx = cazador.x >> CELDA_SHIFT, ty = cazador.y >> CELDA_SHIFT;
    s16 px = (flicky.x + 8) >> CELDA_SHIFT, py = (flicky.y + 8) >> CELDA_SHIFT;

    // tiburón: embestida si te tiene en línea recta
    if (mundo_actual == MUNDO_MARINO) {
        if (cazador.embestida) {
            if (!hay_muro(tx + cazador.x_vel, ty + cazador.y_vel)) return;
            cazador.embestida = 0; cazador.recarga = 150;
        } else if (cazador.recarga == 0 && (tx == px || ty == py) && abs(tx - px) + abs(ty - py) <= 9 && linea_libre(tx, ty, px, py)) {
            cazador.x_vel = (px > tx) - (px < tx); cazador.y_vel = (py > ty) - (py < ty);
            cazador.embestida = 1;
            sonido(SFX_RED);
            return;
        }
    }

    s16 op_x[4], op_y[4]; u16 n = 0;
    for (u16 d = 0; d < 4; d++) {
        if (ddx[d] == -cazador.x_vel && ddy[d] == -cazador.y_vel && (cazador.x_vel || cazador.y_vel)) continue;   // sin media vuelta
        if (hay_muro(tx + ddx[d], ty + ddy[d])) continue;
        op_x[n] = ddx[d]; op_y[n] = ddy[d]; n++;
    }
    if (n == 0) { cazador.x_vel = -cazador.x_vel; cazador.y_vel = -cazador.y_vel; return; }

    s16 p = (mundo_actual == MUNDO_VOLCAN) ? 45 : (mundo_actual == MUNDO_MARINO) ? 50 : (mundo_actual == MUNDO_CELESTE) ? 65 : 60;
    p += fase_actual * 4 + d_sel * 5 + (semillas_en_fila ? 10 : 0);
    if (cazador_en_furia) p = 95;
    if (p > 95) p = 95;

    u16 e;
    if ((s16)(random() % 100) < p) {             // persigue por el camino más corto
        calcular_distancias(px, py);
        u16 mejor = 0xFFFF; e = 0;
        for (u16 i = 0; i < n; i++) {
            u16 d = distancia[ty + op_y[i]][tx + op_x[i]];
            if (d < mejor || (d == mejor && (random() & 1))) { mejor = d; e = i; }
        }
    } else {
        e = random() % n;
    }
    cazador.x_vel = op_x[e]; cazador.y_vel = op_y[e];
}

static void paso() {
    if (modo_jugadores == 0 && (cazador.x & CELDA_MASK) == 0 && (cazador.y & CELDA_MASK) == 0) decidir();
    if (cazador.x_vel == 0 && cazador.y_vel == 0) return;
    s16 nx = cazador.x + cazador.x_vel, ny = cazador.y + cazador.y_vel;
    s16 cx = (cazador.x_vel > 0) ? (nx + CELDA_MASK) : nx;
    s16 cy = (cazador.y_vel > 0) ? (ny + CELDA_MASK) : ny;
    if (!hay_muro(cx >> CELDA_SHIFT, cy >> CELDA_SHIFT)) { cazador.x = nx; cazador.y = ny; }
    else {
        cazador.x = ((cazador.x + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
        cazador.y = ((cazador.y + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
        cazador.x_vel = 0; cazador.y_vel = 0;
        if (cazador.embestida) { cazador.embestida = 0; cazador.recarga = 150; efecto_shake(8, 2); }
    }
}

static void teletransporte() {
    s16 nx, ny;
    if (!buscar_celda_libre(&nx, &ny, 8)) return;
    fx_chispa(cazador.x, cazador.y);
    sonido(SFX_TELEPORT);
    cazador.x = nx * CELDA; cazador.y = ny * CELDA; cazador.x_vel = 0; cazador.y_vel = 0;
    cazador.oculto = 30;
    SPR_setVisibility(cazador.sprite, HIDDEN);
}

static void dibujar() {
    u8 a = cazador.anim;
    if (cazador.y_vel > 0) a = ANIM_ABAJO;
    else if (cazador.y_vel < 0) a = ANIM_ARRIBA;
    else if (cazador.x_vel != 0) a = ANIM_LADO;
    else if (a == 0xFF) a = ANIM_ABAJO;
    if (a != cazador.anim) { SPR_setAnim(cazador.sprite, a); cazador.anim = a; }
    if (cazador.x_vel) SPR_setHFlip(cazador.sprite, cazador.x_vel < 0);
    if ((frame_counter % (cazador.embestida ? 3 : 6)) == 0) {
        if ((cazador.x_vel || cazador.y_vel) && !cazador.aturdido) SPR_nextFrame(cazador.sprite);
        else SPR_setFrame(cazador.sprite, 0);
    }
    s16 temblor = (cazador.aturdido && (frame_counter & 2)) ? 1 : 0;
    if (barra_red) {                                     // la barra se llena de 0 a 7 durante la recarga
        SPR_setFrame(barra_red, ((RECARGA_RED - cazador.recarga) * 8) / (RECARGA_RED + 1));
        SPR_setPosition(barra_red, cazador.x + shake_x, cazador.y + PLAY_Y - 18);
    }
    SPR_setPosition(cazador.sprite, cazador.x - 4 + shake_x + temblor, cazador.y + PLAY_Y - 8);
    // estrellitas mientras está aturdido
    if (cazador.aturdido) {
        if (!estrellas_c) {
            estrellas_c = SPR_addSprite(&estrellas_sprite, 0, 0, TILE_ATTR(PAL2, 1, FALSE, FALSE));
            if (estrellas_c) SPR_setDepth(estrellas_c, SPR_MIN_DEPTH);
        }
        if (estrellas_c) {
            SPR_setPosition(estrellas_c, cazador.x - 4 + shake_x, cazador.y + PLAY_Y - 14);
            if ((frame_counter & 3) == 0) SPR_nextFrame(estrellas_c);
        }
    } else if (estrellas_c) { SPR_releaseSprite(estrellas_c); estrellas_c = NULL; }
}

// versus: el cazador también aprovecha el reloj (frena al pájaro) y el rayo (corre más)
static void cazador_recoger() {
    s16 cx = (cazador.x + 8) >> CELDA_SHIFT, cy = (cazador.y + 8) >> CELDA_SHIFT;
    u8 t = mapa[cy][cx];
    if (t != TILE_RELOJ && t != TILE_RAYO) return;
    mapa[cy][cx] = TILE_SUELO;
    dibujar_celda(cx, cy);
    fx_chispa(cx * CELDA, cy * CELDA);
    if (t == TILE_RELOJ) { timer_flicky_lento = DURACION_PODER; timer_flicky_rapido = 0; sonido(SFX_RELOJ); }
    else { timer_cazador_rapido = DURACION_PODER; timer_cazador_lento = 0; sonido(SFX_RAYO); efecto_shake(8, 2); }
}

void cazador_actualizar() {
    if (cazador.recarga > 0 && --cazador.recarga == 0 && modo_jugadores == 1) {   // red lista: destello y "clic"
        fx_chispa(cazador.x, cazador.y - 10);
        sonido(SFX_CLICK);
        destello_listo = 10;
        if (barra_red) { SPR_releaseSprite(barra_red); barra_red = NULL; }
    }
    if (destello_listo > 0) destello_listo--;
    if (timer_cazador_rapido > 0) timer_cazador_rapido--;
    if (cazador.inmune > 0) cazador.inmune--;
    if (cazador.aturdido > 0) {
        if (--cazador.aturdido == 0) cazador.inmune = 120;
        cazador_tinte_actualizar(FALSE);
        dibujar();
        return;
    }
    if (cazador.oculto > 0) {
        if (--cazador.oculto == 0) { SPR_setVisibility(cazador.sprite, VISIBLE); fx_chispa(cazador.x, cazador.y); }
        return;
    }
    if (modo_jugadores == 0 && mundo_actual == MUNDO_CELESTE && --cazador.tele_timer <= 0) {
        cazador.tele_timer = 480 + (random() % 240);
        s16 dx = abs(cazador.x - flicky.x), dy = abs(cazador.y - flicky.y);
        if (dx + dy > 48) { teletransporte(); return; }
    }

    // velocidad en octavos de píxel por frame
    u16 v;
    if (modo_jugadores == 1) v = (timer_cazador_lento > 0) ? 5 : (timer_cazador_rapido > 0) ? 12 : 8;
    else if (timer_cazador_lento > 0) v = 2;
    else if (cazador.embestida) v = 16;
    else if (cazador_en_furia) v = (mundo_actual == MUNDO_VOLCAN) ? 13 : 12;
    else {
        v = 4 + (semillas_en_fila >= 6 ? 2 : 0) + (multiplicador_velocidad_minuto > 3 ? 3 : multiplicador_velocidad_minuto)
              + (fase_actual >= 4 ? 1 : 0) + (d_sel == 2 ? 1 : 0) + (mundo_actual == MUNDO_VOLCAN ? 1 : 0);
    }
    if (modo_jugadores == 1) cazador_recoger();
    cazador.acumulador += v;
    while (cazador.acumulador >= 8) { cazador.acumulador -= 8; paso(); }

    cazador_tinte_actualizar(FALSE);
    dibujar();
}

// rojo en rabia, azulado cuando el reloj lo frena, pálido cuando está aturdido
void cazador_tinte_actualizar(bool forzar) {
    static u8 tinte_actual = 0xFF;
    u8 tinte = destello_listo > 0 ? ((destello_listo & 4) ? 4 : 0) : cazador.aturdido ? 3 : cazador_en_furia ? 1 : (timer_cazador_lento > 0) ? 2 : 0;
    if (forzar) tinte_actual = 0xFF;
    if (tinte == tinte_actual || cazador.def == NULL) return;
    tinte_actual = tinte;
    const u16* src = cazador.def->palette->data;
    for (u16 i = 1; i < 16; i++) {
        u16 c = src[i];
        u16 r = (c >> 1) & 7, g = (c >> 5) & 7, b = (c >> 9) & 7;
        if (tinte == 1) { r = (r + 4 > 7) ? 7 : r + 4; g >>= 1; b >>= 1; }
        else if (tinte == 2) { b = (b + 3 > 7) ? 7 : b + 3; r >>= 1; }
        else if (tinte == 3) { r = (r + 7) >> 1; g = (g + 7) >> 1; b = (b + 7) >> 1; }
        else if (tinte == 4) { r = (r + 14) / 3; g = (g + 14) / 3; b = (b + 14) / 3; }
        PAL_setColor(48 + i, (b << 9) | (g << 5) | (r << 1));
    }
}
