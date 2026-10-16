// Efectos visuales: destellos, puntos flotantes, polvo, estallidos, carteles, temblor,
// scroll por líneas (marcador fijo y agua ondulante) y fundidos de paleta
#include "juego.h"

s16 shake_x = 0;
static u16 shake_timer = 0, shake_intensidad = 2;

// ------------------------------------------------------------------ sprites de efecto (efímeros)
#define MAX_FX 14
typedef struct { Sprite* s; s16 x, y, vy, t; } Fx;   // y y vy en 1/16 de píxel
static Fx fx[MAX_FX];

void fx_reset() {
    // SPR_clear() solo oculta: hay que liberar los sprites de verdad
    for (u16 i = 0; i < MAX_FX; i++) {
        if (fx[i].s) SPR_releaseSprite(fx[i].s);
        fx[i].s = NULL; fx[i].t = 0;
    }
}

static void fx_nuevo(const SpriteDefinition* def, u16 anim, s16 x, s16 y, s16 vy, s16 vida, bool encima) {
    for (u16 i = 0; i < MAX_FX; i++) {
        if (fx[i].s == NULL) {
            Sprite* s = SPR_addSprite(def, x + shake_x, y + PLAY_Y, TILE_ATTR(PAL2, 1, FALSE, FALSE));
            if (!s) return;
            SPR_setAnim(s, anim);
            SPR_setAnimationLoop(s, FALSE);
            if (encima) SPR_setDepth(s, SPR_MIN_DEPTH);
            fx[i].s = s; fx[i].x = x; fx[i].y = y << 4; fx[i].vy = vy; fx[i].t = vida;
            return;
        }
    }
}

void fx_chispa(s16 x, s16 y)            { fx_nuevo(&spark_sprite, 0, x, y, 0, 16, FALSE); }
void fx_polvo(s16 x, s16 y)             { fx_nuevo(&polvo_sprite, 0, x, y + 4, -4, 16, FALSE); }
void fx_popup(s16 x, s16 y, u16 indice) { fx_nuevo(&popup_sprite, indice, x - 8, y - 6, -10, 45, TRUE); }

// estallido de estrellas alrededor de un punto (impactos)
void fx_estallido(s16 x, s16 y) {
    fx_chispa(x - 8, y - 6);
    fx_chispa(x + 8, y - 6);
    fx_chispa(x, y + 6);
}

void fx_actualizar() {
    for (u16 i = 0; i < MAX_FX; i++) {
        Fx* f = &fx[i];
        if (!f->s) continue;
        f->y += f->vy;
        SPR_setPosition(f->s, f->x + shake_x, (f->y >> 4) + PLAY_Y);
        if (--f->t <= 0) { SPR_releaseSprite(f->s); f->s = NULL; }
    }
}

// ------------------------------------------------------------------ destello blanco
static u16 destello_t = 0;
static u16 paleta_guardada[32];
void fx_destello() {
    if (destello_t || PAL_isDoingFade()) return;
    PAL_getColors(0, paleta_guardada, 32);
    u16 claro[32];
    for (u16 i = 0; i < 32; i++) {
        u16 c = paleta_guardada[i], r = 0;
        for (u16 sh = 1; sh <= 9; sh += 4) { u16 v = (c >> sh) & 7; v = v + ((7 - v) * 3) / 4; r |= v << sh; }
        claro[i] = r;
    }
    PAL_setColors(0, claro, 32, CPU);
    destello_t = 4;
}
static void destello_actualizar() {
    if (destello_t && --destello_t == 0) PAL_setColors(0, paleta_guardada, 32, CPU);
}

// ------------------------------------------------------------------ temblor y scroll por líneas
// En partida se usa scroll por líneas: el marcador (16 primeras líneas) no tiembla nunca,
// y en el mundo marino el suelo ondula suavemente como visto a través del agua.
static s16 tabla_a[224], tabla_b[224];
static bool scroll_lineas = FALSE;

void efecto_shake(u16 duracion, u16 intensidad) { shake_timer = duracion; shake_intensidad = intensidad; }

void fx_scroll_juego(bool activo) {
    scroll_lineas = activo;
    shake_x = 0; shake_timer = 0;
    if (activo) {
        VDP_setScrollingMode(HSCROLL_LINE, VSCROLL_PLANE);
        for (u16 i = 0; i < 224; i++) { tabla_a[i] = 0; tabla_b[i] = 0; }
        VDP_setHorizontalScrollLine(BG_A, 0, tabla_a, 224, DMA);
        VDP_setHorizontalScrollLine(BG_B, 0, tabla_b, 224, DMA);
    } else {
        VDP_setScrollingMode(HSCROLL_PLANE, VSCROLL_PLANE);
        VDP_setHorizontalScroll(BG_A, 0);
        VDP_setHorizontalScroll(BG_B, 0);
    }
}

void fx_scroll_actualizar() {
    destello_actualizar();
    if (shake_timer > 0) { shake_x = (random() % (shake_intensidad * 2 + 1)) - shake_intensidad; shake_timer--; }
    else shake_x = 0;
    if (!scroll_lineas) return;
    static const s8 ola[32] = { 0,0,1,1,1,1,1,0, 0,0,0,-1,-1,-1,-1,-1, 0,0,1,1,1,1,1,0, 0,0,0,-1,-1,-1,-1,-1 };
    bool agua = (mundo_actual == MUNDO_MARINO);
    u16 f = frame_counter >> 2;
    for (u16 y = PLAY_Y; y < 224; y++) {
        tabla_a[y] = shake_x;
        tabla_b[y] = shake_x + (agua ? ola[((y >> 2) + f) & 31] : 0);
    }
    VDP_setHorizontalScrollLine(BG_A, 0, tabla_a, 224, DMA_QUEUE);
    VDP_setHorizontalScrollLine(BG_B, 0, tabla_b, 224, DMA_QUEUE);
}

// ------------------------------------------------------------------ fundidos
static bool pantalla_visible = FALSE;
void fx_fundido_salida() {
    if (pantalla_visible) PAL_fadeOutAll(10, FALSE);
    pantalla_visible = FALSE;
}
// se llama con todo ya dibujado y las paletas puestas: arranca en negro y funde hasta ellas
void fx_fundido_entrada() {
    static u16 destino[64];
    static const u16 negro[64] = { 0 };
    PAL_getColors(0, destino, 64);
    PAL_setColors(0, negro, 64, CPU);
    VDP_setEnable(TRUE);
    PAL_fadeIn(0, 63, destino, 12, TRUE);
    pantalla_visible = TRUE;
}

// ------------------------------------------------------------------ cartel centrado
#define TILE_HUD_FONDO 96     // tiles sueltos al final del tileset del laberinto
#define TILE_HUD_BORDE 97
void banner(const char* txt) {
    u16 len = strlen(txt);
    u16 x0 = (40 - len) / 2 - 2, x1 = x0 + len + 3;
    // arriba o abajo, para no tapar al protagonista
    u16 yc = ((flicky.y + PLAY_Y) >> 3) < 15 ? 21 : 7;
    for (u16 y = yc - 1; y <= yc + 1; y++) {
        for (u16 x = x0; x <= x1; x++) {
            u16 attr = TILE_ATTR_FULL(PAL0, 1, 0, 0, base_maze + TILE_HUD_FONDO);
            if (y == yc + 1) attr = TILE_ATTR_FULL(PAL0, 1, 0, 0, base_maze + TILE_HUD_BORDE);
            if (y == yc - 1) attr = TILE_ATTR_FULL(PAL0, 1, 1, 0, base_maze + TILE_HUD_BORDE);
            VDP_setTileMapXY(BG_B, attr, x, y);
            VDP_setTileMapXY(BG_A, 0, x, y);
        }
    }
    VDP_setTextPalette(PAL0);
    VDP_setTextPriority(1);
    VDP_drawText(txt, (40 - len) / 2, yc);
    VDP_setTextPriority(0);
}
