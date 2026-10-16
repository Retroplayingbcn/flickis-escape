// Sonido: música FM y efectos PCM con el driver XGM2 de SGDK.
// Las pistas salen de tools/sound (MML original compilado con ctrmml), en el orden de resources.res.
#include "juego.h"

static s16 pista_actual = -1;

void musica_pista(u16 pista) {
    if (pista_actual == (s16)pista && XGM2_isPlaying()) return;
    pista_actual = pista;
    XGM2_playTrack(pista);
}

// para los fragmentos cortos (fanfarrias) que deben empezar siempre desde el principio
void musica_reiniciar(u16 pista) {
    pista_actual = pista;
    XGM2_playTrack(pista);
}

void musica_detener()  { XGM2_stop(); pista_actual = -1; }
void musica_pausar()   { XGM2_pause(); }
void musica_reanudar() { XGM2_resume(); }

void musica_segun_estado(u8 estado) {
    switch (estado) {
        case ESTADO_TITULO:      musica_pista(PISTA_TITULO); break;
        case ESTADO_MENU:
        case ESTADO_MENU_VERSUS:
        case ESTADO_IDIOMA:
        case ESTADO_COMO_JUGAR:  musica_pista(PISTA_MENU); break;
        case ESTADO_HISTORIA:    musica_pista(PISTA_CONTINUAR); break;
        case ESTADO_INTRO:       musica_reiniciar(PISTA_INTRO); break;
        case ESTADO_JUEGO:
            if (en_jefe) { musica_pista(PISTA_JEFE); break; }
            musica_pista(mundo_actual == MUNDO_VOLCAN ? PISTA_VOLCAN : mundo_actual == MUNDO_MARINO ? PISTA_MARINO :
                         mundo_actual == MUNDO_CELESTE ? PISTA_CELESTE : PISTA_BOSQUE);
            break;
        case ESTADO_GAMEOVER:    musica_reiniciar(PISTA_GAMEOVER); break;
        case ESTADO_FINJUEGO:    musica_pista(PISTA_VICTORIA); break;
        case ESTADO_CONTINUE:    musica_pista(PISTA_CONTINUAR); break;
    }
}

// Efectos: el canal PCM 1 lo usa la batería de la música; los efectos van por el 2 y el 3.
// Los avisos importantes (canal 3) llevan prioridad para no cortarse.
typedef struct { const u8* datos; u32 largo; u8 canal; u8 prioridad; } Efecto;
#define EF(nombre, canal, prio) { nombre, sizeof(nombre), canal, prio }
static const Efecto efectos[] = {
    [SFX_CLICK]     = EF(sfx_click,     SOUND_PCM_CH2, 0),
    [SFX_ALERTA]    = EF(sfx_alerta,    SOUND_PCM_CH3, 0),
    [SFX_PIO]       = EF(sfx_pio,       SOUND_PCM_CH2, 0),
    [SFX_NIDO]      = EF(sfx_nido0,     SOUND_PCM_CH2, 1),
    [SFX_GEMA]      = EF(sfx_gema,      SOUND_PCM_CH2, 0),
    [SFX_RELOJ]     = EF(sfx_reloj,     SOUND_PCM_CH2, 0),
    [SFX_RAYO]      = EF(sfx_rayo,      SOUND_PCM_CH2, 0),
    [SFX_META]      = EF(sfx_meta,      SOUND_PCM_CH3, 1),
    [SFX_CIERRE]    = EF(sfx_cierre,    SOUND_PCM_CH3, 1),
    [SFX_PANICO]    = EF(sfx_panico,    SOUND_PCM_CH3, 1),
    [SFX_ATRAPADO]  = EF(sfx_atrapado,  SOUND_PCM_CH3, 1),
    [SFX_RABIA]     = EF(sfx_rabia,     SOUND_PCM_CH3, 1),
    [SFX_LANZAR]    = EF(sfx_lanzar,    SOUND_PCM_CH2, 0),
    [SFX_CHOF]      = EF(sfx_chof,      SOUND_PCM_CH2, 1),
    [SFX_ATURDIDO]  = EF(sfx_aturdido,  SOUND_PCM_CH3, 0),
    [SFX_GOLPE]     = EF(sfx_golpe,     SOUND_PCM_CH3, 1),
    [SFX_EXPLOSION] = EF(sfx_explosion, SOUND_PCM_CH3, 1),
    [SFX_VIDA]      = EF(sfx_unavida,   SOUND_PCM_CH3, 1),
    [SFX_TELEPORT]  = EF(sfx_teleport,  SOUND_PCM_CH3, 0),
    [SFX_RED]       = EF(sfx_red,       SOUND_PCM_CH3, 0),
};

void sonido(u16 id) {
    const Efecto* e = &efectos[id];
    XGM2_playPCMEx(e->datos, e->largo, e->canal, e->prioridad, FALSE, FALSE);
}

// cada polluelo que entra en el nido suena una nota más aguda
void sonido_nido(u16 n) {
    static const u8* const notas[5] = { sfx_nido0, sfx_nido1, sfx_nido2, sfx_nido3, sfx_nido4 };
    static const u32 largos[5] = { sizeof(sfx_nido0), sizeof(sfx_nido1), sizeof(sfx_nido2), sizeof(sfx_nido3), sizeof(sfx_nido4) };
    u16 i = (n > 4) ? 4 : n;
    XGM2_playPCMEx(notas[i], largos[i], SOUND_PCM_CH2, 1, FALSE, FALSE);
}
