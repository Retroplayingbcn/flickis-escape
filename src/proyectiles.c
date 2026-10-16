// Proyectiles: el huevo que lanza Flicki y los ataques de los jefes
#include "juego.h"

// ------------------------------------------------------------------ huevo de Flicki
typedef struct { Sprite* s; s16 x, y, vx, vy, t; bool roto; } Huevo;
static Huevo huevo;

#define VEL_HUEVO 4

void lanzar_huevo() {
    if (huevo.s) return;                         // uno cada vez
    if (municion == 0) { sonido(SFX_CLICK); return; }
    municion--;
    huevo.x = flicky.x; huevo.y = flicky.y;
    huevo.vx = flicky.dir_x * VEL_HUEVO; huevo.vy = flicky.dir_y * VEL_HUEVO;
    if (huevo.vx == 0 && huevo.vy == 0) huevo.vx = VEL_HUEVO;
    huevo.t = 0; huevo.roto = FALSE;
    huevo.s = SPR_addSprite(&huevo_sprite, huevo.x + shake_x, huevo.y + PLAY_Y, TILE_ATTR(PAL2, 1, FALSE, FALSE));
    if (huevo.s) { SPR_setAnim(huevo.s, 0); SPR_setDepth(huevo.s, SPR_MIN_DEPTH); }
    sonido(SFX_LANZAR);
}

static void huevo_romper() {
    huevo.roto = TRUE; huevo.t = 0;
    SPR_setAnim(huevo.s, 1);
    SPR_setAnimationLoop(huevo.s, FALSE);
    sonido(SFX_CHOF);
}

static void huevo_actualizar() {
    if (!huevo.s) return;
    huevo.t++;
    if (huevo.roto) {
        SPR_setPosition(huevo.s, huevo.x + shake_x, huevo.y + PLAY_Y);
        if (huevo.t > 18) { SPR_releaseSprite(huevo.s); huevo.s = NULL; }
        return;
    }
    // avanza en dos medios pasos para no atravesar paredes ni enemigos
    for (u16 paso = 0; paso < 2; paso++) {
        huevo.x += huevo.vx / 2; huevo.y += huevo.vy / 2;
        s16 cx = huevo.x + 8, cy = huevo.y + 8;
        if (hay_muro(cx >> CELDA_SHIFT, cy >> CELDA_SHIFT)) {
            huevo.x -= huevo.vx / 2; huevo.y -= huevo.vy / 2;
            huevo_romper(); break;
        }
        if (en_jefe) {
            if (jefe_impacto(cx, cy)) { huevo_romper(); break; }
        } else if (abs(cx - (cazador.x + 8)) < 12 && abs(cy - (cazador.y + 8)) < 12 && !cazador.oculto) {
            cazador_aturdir();
            huevo_romper(); break;
        }
    }
    if (!huevo.roto && huevo.t > 90) huevo_romper();
    SPR_setPosition(huevo.s, huevo.x + shake_x, huevo.y + PLAY_Y);
}

// ------------------------------------------------------------------ ataques de los jefes
#define MAX_DISPAROS 12
typedef struct { Sprite* s; u16 tipo; s16 x, y, vx, vy, vida; bool pegado; } Disparo;   // x, y, vx, vy en 1/16 px
static Disparo disparos[MAX_DISPAROS];

void disparo_jefe(u16 tipo, s16 x, s16 y, s16 vx, s16 vy, s16 vida) {
    for (u16 i = 0; i < MAX_DISPAROS; i++) {
        Disparo* d = &disparos[i];
        if (d->s) continue;
        d->s = SPR_addSprite(&proyectil_sprite, x + shake_x, y + PLAY_Y, TILE_ATTR(PAL2, 1, FALSE, FALSE));
        if (!d->s) return;
        SPR_setAnim(d->s, tipo);
        d->pegado = FALSE;
        d->tipo = tipo; d->x = x << 4; d->y = y << 4; d->vx = vx; d->vy = vy; d->vida = vida;
        return;
    }
}

void proyectiles_reset() {
    if (huevo.s) SPR_releaseSprite(huevo.s);
    huevo.s = NULL;
    for (u16 i = 0; i < MAX_DISPAROS; i++) {
        if (disparos[i].s) SPR_releaseSprite(disparos[i].s);
        disparos[i].s = NULL;
    }
}

void proyectiles_actualizar() {
    huevo_actualizar();
    for (u16 i = 0; i < MAX_DISPAROS; i++) {
        Disparo* d = &disparos[i];
        if (!d->s) continue;
        if (d->pegado) {                               // red enganchada a Flicki mientras dura el enredo
            if (timer_enredado <= 0) { SPR_releaseSprite(d->s); d->s = NULL; continue; }
            SPR_setPosition(d->s, flicky.x + shake_x, flicky.y + PLAY_Y + ((frame_counter & 8) ? 1 : 0));
            continue;
        }
        d->x += d->vx; d->y += d->vy;
        s16 px = d->x >> 4, py = d->y >> 4;
        bool fin = (--d->vida <= 0);
        if (d->tipo != DISPARO_FUEGO && hay_muro((px + 8) >> CELDA_SHIFT, (py + 8) >> CELDA_SHIFT)) fin = TRUE;
        if (fin) { SPR_releaseSprite(d->s); d->s = NULL; continue; }
        SPR_setPosition(d->s, px + shake_x, py + PLAY_Y);
        // las llamas se apagan parpadeando
        if (d->tipo == DISPARO_FUEGO && d->vida < 40) SPR_setVisibility(d->s, (d->vida & 4) ? HIDDEN : VISIBLE);
        if (modo_jugadores == 1) {                     // versus: la red enreda, no mata
            if (!flicky_invulnerable && timer_enredado == 0 && abs(px - flicky.x) < 10 && abs(py - flicky.y) < 10) {
                jugador_enredar();
                d->pegado = TRUE;
                SPR_setDepth(d->s, SPR_MIN_DEPTH);
            }
            continue;
        }
        if (!flicky_invulnerable && abs(px - flicky.x) < 11 && abs(py - flicky.y) < 11) {
            jugador_muere(T("EL JEFE TE ALCANZO!", "THE BOSS GOT YOU!"));
            return;
        }
    }
}
