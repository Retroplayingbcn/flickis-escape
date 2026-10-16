// Flicki: control, movimiento por la rejilla, objetos, polluelos que la siguen y que escapan
#include "juego.h"

Sprite* sprites_seguidores[MAX_POLLUELOS];
s16 historial_x[MAX_HISTORIAL], historial_y[MAX_HISTORIAL];
u16 indice_historial = 0;

static Sprite* estrellas = NULL;     // estrellitas de mareo tras un golpe
static s16 estrellas_t = 0;

void sumar_puntos(u32 puntos) {
    flicky.puntuacion += puntos;
    if (modo_jugadores == 0 && flicky.puntuacion >= proximo_hito_vida) {
        proximo_hito_vida += PUNTOS_VIDA_EXTRA;
        ganar_vida();
    }
}

void ganar_vida() {
    vidas++;
    sonido(SFX_VIDA);
    fx_popup(flicky.x, flicky.y - 10, POP_1UP);
}

void jugador_crear(s16 cx, s16 cy) {
    flicky.x = cx * CELDA; flicky.y = cy * CELDA;
    flicky.x_vel = 0; flicky.y_vel = 0; flicky.dir_x = 1; flicky.dir_y = 0;
    flicky.anim = 0xFF;
    flicky.sprite = SPR_addSprite(&hero_sprite, flicky.x, flicky.y + PLAY_Y, TILE_ATTR(PAL2, 0, FALSE, FALSE));
    for (u16 h = 0; h < MAX_HISTORIAL; h++) { historial_x[h] = flicky.x; historial_y[h] = flicky.y; }
    indice_historial = 0;
    for (u16 i = 0; i < MAX_POLLUELOS; i++) sprites_seguidores[i] = NULL;
    estrellas = NULL; estrellas_t = 0;
}

// ------------------------------------------------------------------ control
// en versus se llevan como mucho 2 huevos (contra una persona, 3 es demasiado)
u16 municion_max() { return (modo_jugadores == 1) ? 2 : MAX_MUNICION; }

void jugador_entrada(u16 joy, u16 pulsado) {
    if (timer_enredado > 0) return;                   // atrapada en la red
    if (pulsado & (BUTTON_A | BUTTON_B)) lanzar_huevo();

    s16 ox = flicky.x & CELDA_MASK, oy = flicky.y & CELDA_MASK;
    if (!((ox <= 3 || ox >= 13) && (oy <= 3 || oy >= 13))) return;     // solo se gira cerca del centro de una celda
    s16 nx = 0, ny = 0;
    if (joy & BUTTON_LEFT) nx = -1;
    else if (joy & BUTTON_RIGHT) nx = 1;
    else if (joy & BUTTON_UP) ny = -1;
    else if (joy & BUTTON_DOWN) ny = 1;
    if ((nx == 0 && ny == 0) || (nx == flicky.x_vel && ny == flicky.y_vel)) return;
    s16 cx = (flicky.x + 8) >> CELDA_SHIFT, cy = (flicky.y + 8) >> CELDA_SHIFT;
    if (hay_muro(cx + nx, cy + ny)) return;
    bool arranca = (flicky.x_vel == 0 && flicky.y_vel == 0);
    bool gira = !arranca && (nx != -flicky.x_vel || ny != -flicky.y_vel);
    flicky.x = cx << CELDA_SHIFT; flicky.y = cy << CELDA_SHIFT;
    flicky.x_vel = nx; flicky.y_vel = ny;
    flicky.dir_x = nx; flicky.dir_y = ny;
    if (arranca || gira) fx_polvo(flicky.x, flicky.y);             // nubecilla al arrancar o girar
}

// ------------------------------------------------------------------ movimiento y objetos
static void recoger(s16 cx, s16 cy) {
    u8 t = mapa[cy][cx];
    if (t == TILE_SUELO || t == TILE_MURO) return;
    if (t == TILE_POLLUELO && semillas_en_fila >= MAX_POLLUELOS) return;
    if (t == TILE_POLLUELO && en_jefe) return;
    mapa[cy][cx] = TILE_SUELO;
    dibujar_celda(cx, cy);
    fx_chispa(cx * CELDA, cy * CELDA);
    switch (t) {
        case TILE_POLLUELO: {
            Sprite* s = SPR_addSprite(&chick_sprite, flicky.x, flicky.y + PLAY_Y, TILE_ATTR(PAL2, 0, FALSE, FALSE));
            if (s) SPR_setAnimationLoop(s, TRUE);
            sprites_seguidores[semillas_en_fila++] = s;
            sonido(SFX_PIO);
            break;
        }
        case TILE_GEMA:
            sumar_puntos(250);
            fx_popup(cx * CELDA, cy * CELDA, POP_250);
            sonido(SFX_GEMA);
            efecto_shake(6, 1);
            break;
        case TILE_RELOJ:
            timer_cazador_lento = DURACION_PODER; timer_cazador_rapido = 0;
            cazador_en_furia = FALSE;
            muros_alerta(FALSE);
            sonido(SFX_RELOJ);
            break;
        case TILE_RAYO:
            timer_flicky_rapido = DURACION_PODER; timer_flicky_lento = 0;
            sonido(SFX_RAYO);
            efecto_shake(8, 2);
            break;
        case TILE_HUEVO:
            if (municion < municion_max()) municion++;
            else { sumar_puntos(100); fx_popup(cx * CELDA, cy * CELDA, POP_100); }
            sonido(SFX_GEMA);
            break;
    }
}

void jugador_actualizar() {
    if (timer_flicky_lento > 0) timer_flicky_lento--;
    if (timer_enredado > 0) {                          // enredada: no se mueve
        if (--timer_enredado == 0) sonido(SFX_CLICK);
        return;
    }
    u16 pasos = (timer_flicky_rapido > 0) ? 2 : 1;
    if (timer_flicky_lento > 0 && (frame_counter & 3) == 0) pasos = 0;   // frenada por el reloj del rival
    for (u16 p = 0; p < pasos; p++) {
        if (flicky.x_vel == 0 && flicky.y_vel == 0) break;
        s16 nx = flicky.x + flicky.x_vel, ny = flicky.y + flicky.y_vel;
        s16 col_x = (flicky.x_vel > 0) ? (nx + CELDA_MASK) : nx;
        s16 col_y = (flicky.y_vel > 0) ? (ny + CELDA_MASK) : ny;
        if (mapa[col_y >> CELDA_SHIFT][col_x >> CELDA_SHIFT] != TILE_MURO) { flicky.x = nx; flicky.y = ny; }
        else {
            flicky.x = ((flicky.x + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
            flicky.y = ((flicky.y + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
            flicky.x_vel = 0; flicky.y_vel = 0;
        }
        historial_x[indice_historial] = flicky.x;
        historial_y[indice_historial] = flicky.y;
        indice_historial = (indice_historial + 1) & HIST_MASK;
    }
    recoger((flicky.x + 8) >> CELDA_SHIFT, (flicky.y + 8) >> CELDA_SHIFT);
}

void jugador_animar() {
    // polluelos en fila, siguiendo el rastro del pájaro
    for (u16 i = 0; i < semillas_en_fila; i++) {
        if (!sprites_seguidores[i]) continue;
        s16 r = (indice_historial - 1 - ((i + 1) * SEPARACION_POLLUELOS) + MAX_HISTORIAL) & HIST_MASK;
        SPR_setPosition(sprites_seguidores[i], historial_x[r] + shake_x, historial_y[r] + PLAY_Y);
    }
    // animación según la dirección
    u8 a = flicky.anim;
    if (flicky.y_vel > 0) a = ANIM_ABAJO;
    else if (flicky.y_vel < 0) a = ANIM_ARRIBA;
    else if (flicky.x_vel != 0) a = ANIM_LADO;
    else if (a == 0xFF || a > ANIM_LADO) a = ANIM_ABAJO;
    if (a != flicky.anim) { SPR_setAnim(flicky.sprite, a); flicky.anim = a; }
    if (flicky.x_vel) SPR_setHFlip(flicky.sprite, flicky.x_vel < 0);
    if ((frame_counter % 6) == 0) {
        if (flicky.x_vel || flicky.y_vel) SPR_nextFrame(flicky.sprite);
        else SPR_setFrame(flicky.sprite, 0);
    }
    SPR_setPosition(flicky.sprite, flicky.x + shake_x, flicky.y + PLAY_Y);
    // parpadeo de invulnerabilidad
    if (flicky_invulnerable) {
        if (--timer_invulnerabilidad <= 0) { flicky_invulnerable = FALSE; SPR_setVisibility(flicky.sprite, VISIBLE); }
        else SPR_setVisibility(flicky.sprite, ((timer_invulnerabilidad >> 2) & 1) ? HIDDEN : VISIBLE);
    }
    // estrellitas de mareo
    if (estrellas) {
        SPR_setPosition(estrellas, flicky.x - 4 + shake_x, flicky.y + PLAY_Y - 7);
        if ((frame_counter & 3) == 0) SPR_nextFrame(estrellas);
        if (--estrellas_t <= 0) { SPR_releaseSprite(estrellas); estrellas = NULL; }
    }
}

// ------------------------------------------------------------------ golpe con polluelos: salen volando
#define VUELO_FRAMES 28
typedef struct { Sprite* s; s16 x0, y0, x1, y1, t; } Vuelo;
static Vuelo vuelos[MAX_POLLUELOS];

void polluelos_reset() {
    for (u16 i = 0; i < MAX_POLLUELOS; i++) {
        if (vuelos[i].s) SPR_releaseSprite(vuelos[i].s);
        vuelos[i].s = NULL;
        if (sprites_seguidores[i]) SPR_releaseSprite(sprites_seguidores[i]);
        sprites_seguidores[i] = NULL;
    }
    if (estrellas) { SPR_releaseSprite(estrellas); estrellas = NULL; }
    semillas_en_fila = 0;
}

static bool destino_ocupado(s16 x, s16 y) {
    for (u16 i = 0; i < MAX_POLLUELOS; i++) if (vuelos[i].s && vuelos[i].x1 == x && vuelos[i].y1 == y) return TRUE;
    return FALSE;
}

// busca una casilla libre a poca distancia (por el laberinto, no en línea recta); siempre encuentra una
static bool destino_polluelo(s16* ox, s16* oy) {
    for (u16 intento = 0; intento < 400; intento++) {
        u16 lim = (intento < 200) ? 5 : (intento < 300) ? 9 : 254;
        s16 x = (random() % (MAP_WIDTH - 2)) + 1, y = (random() % (MAP_HEIGHT - 2)) + 1;
        u8 d = distancia[y][x];
        if (d < 2 || d > lim || mapa[y][x] != TILE_SUELO || en_zona_nido(x, y) || destino_ocupado(x, y)) continue;
        *ox = x; *oy = y; return TRUE;
    }
    for (s16 y = 1; y < MAP_HEIGHT - 1; y++)                // último recurso: la primera libre
        for (s16 x = 1; x < MAP_WIDTH - 1; x++)
            if (mapa[y][x] == TILE_SUELO && distancia[y][x] != 255 && !en_zona_nido(x, y) && !destino_ocupado(x, y)) { *ox = x; *oy = y; return TRUE; }
    return FALSE;
}

// un polluelo sale volando desde (sx, sy) en píxeles hasta una casilla libre cercana
// (las distancias deben estar calculadas desde el punto de referencia)
bool polluelo_volar(s16 sx, s16 sy) {
    s16 tx, ty;
    if (!destino_polluelo(&tx, &ty)) return FALSE;
    for (u16 k = 0; k < MAX_POLLUELOS; k++) {
        if (vuelos[k].s) continue;
        vuelos[k].s = SPR_addSprite(&chick_sprite, sx + shake_x, sy + PLAY_Y, TILE_ATTR(PAL2, 1, FALSE, FALSE));
        if (!vuelos[k].s) break;
        vuelos[k].x0 = sx; vuelos[k].y0 = sy; vuelos[k].x1 = tx; vuelos[k].y1 = ty; vuelos[k].t = 0;
        return TRUE;
    }
    mapa[ty][tx] = TILE_POLLUELO;                     // sin sprite libre: aparece directamente
    dibujar_celda(tx, ty);
    return TRUE;
}

static void soltar_seguidor(u16 i) {
    s16 sx = flicky.x, sy = flicky.y;
    if (sprites_seguidores[i]) {
        sx = SPR_getPositionX(sprites_seguidores[i]) - shake_x;
        sy = SPR_getPositionY(sprites_seguidores[i]) - PLAY_Y;
        SPR_releaseSprite(sprites_seguidores[i]);
        sprites_seguidores[i] = NULL;
    }
    polluelo_volar(sx, sy);
}

void dispersar_polluelos() {
    calcular_distancias((flicky.x + 8) >> CELDA_SHIFT, (flicky.y + 8) >> CELDA_SHIFT);
    for (u16 i = 0; i < semillas_en_fila; i++) soltar_seguidor(i);
    semillas_en_fila = 0;
}

// versus: la red del cazador la deja quieta un momento y se le escapa el último polluelo
void jugador_enredar() {
    if (flicky_invulnerable || timer_enredado > 0) return;
    timer_enredado = ENREDO_FRAMES;
    flicky.x = ((flicky.x + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
    flicky.y = ((flicky.y + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
    flicky.x_vel = 0; flicky.y_vel = 0;
    sonido(SFX_ATURDIDO);
    efecto_shake(8, 2);
    if (semillas_en_fila > 0) {
        calcular_distancias((flicky.x + 8) >> CELDA_SHIFT, (flicky.y + 8) >> CELDA_SHIFT);
        soltar_seguidor(--semillas_en_fila);
        sonido(SFX_PIO);
    }
}

void polluelos_voladores_actualizar() {
    for (u16 i = 0; i < MAX_POLLUELOS; i++) {
        Vuelo* v = &vuelos[i];
        if (!v->s) continue;
        v->t++;
        s16 tx = v->x1 * CELDA, ty = v->y1 * CELDA;
        s16 x = v->x0 + ((tx - v->x0) * v->t) / VUELO_FRAMES;
        s16 y = v->y0 + ((ty - v->y0) * v->t) / VUELO_FRAMES;
        s16 h = (4 * 24 * v->t * (VUELO_FRAMES - v->t)) / (VUELO_FRAMES * VUELO_FRAMES);   // arco de salto
        SPR_setPosition(v->s, x + shake_x, y - h + PLAY_Y);
        if (v->t >= VUELO_FRAMES) {
            SPR_releaseSprite(v->s); v->s = NULL;
            if (mapa[v->y1][v->x1] != TILE_SUELO) {          // alguien ocupó la casilla mientras volaba
                s16 nx, ny;
                if (!destino_polluelo(&nx, &ny)) continue;
                v->x1 = nx; v->y1 = ny;
            }
            mapa[v->y1][v->x1] = TILE_POLLUELO;
            dibujar_celda(v->x1, v->y1);
            fx_polvo(v->x1 * CELDA, v->y1 * CELDA);
        }
    }
}

// golpe del cazador llevando polluelos: se escapan y Flicki queda mareada unos segundos
void jugador_golpeado() {
    fx_estallido(flicky.x, flicky.y);
    fx_destello();
    dispersar_polluelos();
    flicky_invulnerable = TRUE;
    timer_invulnerabilidad = TIEMPO_INVULNERABLE;
    sonido(SFX_PANICO);
    efecto_shake(20, 3);
    if (!estrellas) {
        estrellas = SPR_addSprite(&estrellas_sprite, flicky.x - 4, flicky.y + PLAY_Y - 7, TILE_ATTR(PAL2, 1, FALSE, FALSE));
        if (estrellas) SPR_setDepth(estrellas, SPR_MIN_DEPTH);
    }
    estrellas_t = 90;
}

// animación bloqueante del protagonista (celebraciones)
void secuencia_heroe(u8 anim, u16 frames, bool salta) {
    if (!flicky.sprite) return;
    static const s8 salto[8] = {0, -3, -5, -6, -6, -5, -3, 0};
    SPR_setAnim(flicky.sprite, anim); flicky.anim = anim; SPR_setHFlip(flicky.sprite, FALSE);
    SPR_setVisibility(flicky.sprite, VISIBLE);
    for (u16 f = 0; f < frames; f++) {
        SPR_setFrame(flicky.sprite, (f >> 3) & 3);
        s16 dy = salta ? salto[(f >> 1) & 7] : 0;
        SPR_setPosition(flicky.sprite, flicky.x, flicky.y + PLAY_Y + dy);
        if ((f & 15) == 0) fx_chispa(flicky.x + ((f & 16) ? -10 : 10), flicky.y - 6);
        fx_actualizar();
        fx_scroll_actualizar();
        hud_actualizar();
        SPR_update();
        SYS_doVBlankProcess();
    }
}
