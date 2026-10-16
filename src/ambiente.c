// Ambiente de cada mundo: pequeños detalles vivos que no afectan al juego
//   bosque: ráfagas de viento que arrastran hojas y doblan flores y hierba; mariposas en la arena
//   volcán: brasas que suben del suelo y retumbos con lluvia de ceniza
//   mar (arena): burbujas que suben;  cielo (arena): bandadas de pajaritos
#include "juego.h"

enum { AMB_HOJA, AMB_BRASA, AMB_CENIZA, AMB_BURBUJA, AMB_MARIPOSA, AMB_PAJARO };

#define MAX_AMB 12
typedef struct { Sprite* s; u8 tipo; s16 x, y, vx, vy, t, vida; } Particula;   // x, y, vx, vy en 1/16 px
static Particula pp[MAX_AMB];

static s16 rafaga_t = 0, rafaga_espera = 400;       // bosque
static s16 retumbo_espera = 900, retumbo_t = 0;     // volcán
static s16 bandada_espera = 200;                    // cielo
static u16 color_muro_3 = 0, color_muro_brillo = 0;

void ambiente_reset() {
    for (u16 i = 0; i < MAX_AMB; i++) {
        if (pp[i].s) SPR_releaseSprite(pp[i].s);
        pp[i].s = NULL;
    }
    if (rafaga_t > 0) deco_viento(FALSE);
    rafaga_t = 0; rafaga_espera = 300 + (random() % 400);
    retumbo_espera = 700 + (random() % 600); retumbo_t = 0;
    bandada_espera = 120;
}

static Particula* nueva(u8 tipo, s16 x, s16 y, s16 vx, s16 vy, s16 vida) {
    for (u16 i = 0; i < MAX_AMB; i++) {
        Particula* p = &pp[i];
        if (p->s) continue;
        p->s = SPR_addSprite(&ambiente_sprite, x + shake_x, y + PLAY_Y, TILE_ATTR(PAL2, 1, FALSE, FALSE));
        if (!p->s) return NULL;
        SPR_setAnim(p->s, tipo);
        SPR_setFrame(p->s, random() & 3);
        p->tipo = tipo; p->x = x << 4; p->y = y << 4; p->vx = vx; p->vy = vy; p->t = 0; p->vida = vida;
        return p;
    }
    return NULL;
}

static u16 cuantas(u8 tipo) {
    u16 n = 0;
    for (u16 i = 0; i < MAX_AMB; i++) if (pp[i].s && pp[i].tipo == tipo) n++;
    return n;
}

static const s8 onda[16] = { 0, 3, 6, 8, 9, 8, 6, 3, 0, -3, -6, -8, -9, -8, -6, -3 };

static void mover_particulas() {
    for (u16 i = 0; i < MAX_AMB; i++) {
        Particula* p = &pp[i];
        if (!p->s) continue;
        p->t++;
        s16 dx = 0, dy = 0;
        switch (p->tipo) {
            case AMB_HOJA:     dy = onda[(p->t >> 2) & 15]; break;            // ondulan con el viento
            case AMB_BRASA:    dx = onda[(p->t >> 3) & 15] / 3; break;
            case AMB_CENIZA:   dx = onda[(p->t >> 3) & 15] / 2; break;
            case AMB_BURBUJA:  dx = onda[(p->t >> 2) & 15] / 2; break;
            case AMB_MARIPOSA:                                                   // revolotea sin rumbo fijo
                if ((p->t & 31) == 0) { p->vx = (s16)(random() % 25) - 12; p->vy = (s16)(random() % 17) - 8; }
                dy = onda[(p->t >> 1) & 15] / 2;
                if (p->x < (24 << 4) && p->vx < 0) p->vx = -p->vx;
                if (p->x > (280 << 4) && p->vx > 0) p->vx = -p->vx;
                if (p->y < (24 << 4) && p->vy < 0) p->vy = -p->vy;
                if (p->y > (176 << 4) && p->vy > 0) p->vy = -p->vy;
                break;
            case AMB_PAJARO:   dy = onda[(p->t >> 3) & 15] / 3; break;
        }
        p->x += p->vx + dx; p->y += p->vy + dy;
        s16 px = p->x >> 4, py = p->y >> 4;
        if ((p->vida > 0 && --p->vida == 0) || px < -24 || px > 336 || py < -24 || py > 216) {
            SPR_releaseSprite(p->s); p->s = NULL; continue;
        }
        SPR_setPosition(p->s, px + shake_x, py + PLAY_Y);
        if (p->tipo == AMB_BRASA && p->vida < 16) SPR_setVisibility(p->s, (p->vida & 2) ? HIDDEN : VISIBLE);
    }
}

// ------------------------------------------------------------------ bosque
static void bosque() {
    if (rafaga_t > 0) {
        rafaga_t--;
        // hojas que cruzan de izquierda a derecha
        if ((rafaga_t % 9) == 0 && cuantas(AMB_HOJA) < 8)
            nueva(AMB_HOJA, -12, 8 + (random() % 176), 40 + (random() % 24), 4 - (s16)(random() % 9), 0);
        // un brillo recorre los setos (si no están en alerta)
        if (!muros_en_alerta()) PAL_setColor(3, ((rafaga_t >> 4) & 1) ? color_muro_3 : color_muro_brillo);
        if (rafaga_t == 0) {
            deco_viento(FALSE);
            if (!muros_en_alerta()) PAL_setColor(3, color_muro_3);
            rafaga_espera = 600 + (random() % 600);
        }
    } else if (--rafaga_espera <= 0) {
        rafaga_t = 150;
        color_muro_3 = PAL_getColor(3);
        u16 b = PAL_getColor(4), c = color_muro_3;           // a medio camino entre el tono claro y el brillo
        color_muro_brillo = ((((c >> 1) & 7) + ((b >> 1) & 7)) / 2) << 1 | ((((c >> 5) & 7) + ((b >> 5) & 7)) / 2) << 5 |
                            ((((c >> 9) & 7) + ((b >> 9) & 7)) / 2) << 9;
        deco_viento(TRUE);
    }
    if (en_jefe && cuantas(AMB_MARIPOSA) < 3 && (frame_counter & 63) == 0)
        nueva(AMB_MARIPOSA, 40 + (random() % 240), 30 + (random() % 140), 0, 0, 0);
}

// ------------------------------------------------------------------ volcán
static void volcan() {
    // brasas que salen del suelo y suben, apagándose
    if ((frame_counter % 18) == 0 && cuantas(AMB_BRASA) < 5)
        nueva(AMB_BRASA, 16 + (random() % 288), 40 + (random() % 150), 0, -6 - (s16)(random() % 6), 60 + (random() % 40));
    // retumbo: tiembla todo, la pantalla se enrojece un instante y cae ceniza
    if (retumbo_t > 0) {
        retumbo_t--;
        if (!muros_en_alerta() && !cazador_en_furia) {
            if (retumbo_t == 59) muros_mezcla(3);
            if (retumbo_t == 40) muros_mezcla(0);
        }
        if ((retumbo_t % 8) == 0 && cuantas(AMB_CENIZA) < 5)
            nueva(AMB_CENIZA, 16 + (random() % 288), -8, 0, 10 + (random() % 8), 0);
    } else if (--retumbo_espera <= 0) {
        retumbo_t = 60;
        retumbo_espera = 900 + (random() % 900);
        efecto_shake(36, 2);
        sonido(SFX_GOLPE);
    }
}

// ------------------------------------------------------------------ mar y cielo (arenas)
static void marino() {
    if ((frame_counter % 30) == 0 && cuantas(AMB_BURBUJA) < 5)
        nueva(AMB_BURBUJA, 16 + (random() % 288), 196, 0, -10 - (s16)(random() % 6), 0);
}

static void celeste() {
    if (--bandada_espera > 0) return;
    bandada_espera = 300 + (random() % 300);
    s16 y = 12 + (random() % 150);
    for (u16 i = 0; i < 3; i++) nueva(AMB_PAJARO, -16 - i * 18, y + (i & 1) * 10 - i * 4, 22, 0, 0);
}

void ambiente_actualizar() {
    switch (mundo_actual) {
        case MUNDO_BOSQUE:  bosque(); break;
        case MUNDO_VOLCAN:  volcan(); break;
        case MUNDO_MARINO:  if (en_jefe) marino(); break;
        case MUNDO_CELESTE: if (en_jefe) celeste(); break;
    }
    mover_particulas();
}
