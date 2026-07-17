#include <genesis.h>
#include "resources.h"

#define MAP_WIDTH   40
#define MAP_HEIGHT  27  

// --- TILES DEL MAPA ---
#define TILE_SUELO     0
#define TILE_MURO      1
#define TILE_POLLUELO  2
#define TILE_NIDO      3
#define TILE_RELOJ     4
#define TILE_RAYO      5
#define TILE_GEMA      6
#define TILE_GFX_SUELO_B  7
#define TILE_GFX_FLECHA    8

#define FLICKY_DRAW_OFFSET   4
#define CAZADOR_DRAW_OFFSET  8

#define ANIM_ABAJO  0
#define ANIM_ARRIBA 1
#define ANIM_LADO   2

// --- ESTADOS DE JUEGO ---
#define ESTADO_TITULO      0  
#define ESTADO_MENU        1  
#define ESTADO_COMO_JUGAR  8  
#define ESTADO_MENU_VERSUS 9  
#define ESTADO_JUEGO       2
#define ESTADO_VICTORIA    3
#define ESTADO_GAMEOVER    4
#define ESTADO_FINJUEGO    5
#define ESTADO_CONTINUE    6
#define ESTADO_INTRO       7

// --- DEFINICIONES DE MUNDOS ---
#define MUNDO_BOSQUE  0
#define MUNDO_VOLCAN  1
#define MUNDO_MARINO  2
#define MUNDO_CELESTE 3  

#define MAX_HISTORIAL  128
#define DURACION_PODER_FRAMES  300 

// --- DURACIONES DE ESTADO TRAS DISPERSIÓN/RABIA ---
#define TIEMPO_INVULNERABILIDAD_FRAMES 150  // ~2.5 segundos a 60fps[cite: 1]
#define RADIO_DISPERSION 6  // Radio en tiles para dispersar polluelos[cite: 1]

// ===== SISTEMA DE MÚSICA DE FONDO =====
#define N_SIL   0
#define N_DO3   857
#define N_RE3   764
#define N_MI3   681
#define N_FA3   642
#define N_SOL3  572
#define N_LA3   508
#define N_SI3   453
#define N_DO4   428
#define N_RE4   381
#define N_MI4   339
#define N_FA4   320
#define N_SOL4  285
#define N_LA4   254
#define N_SI4   226
#define N_DO5   214
#define N_RE5   190
#define N_MI5   170
#define N_FA5   160
#define N_SOL5  143

typedef struct {
    u16 tono;
    u8  dur;
} MusicNote;

typedef struct {
    const MusicNote* notas;
    u16 longitud;
    u16 posicion;
    u16 contador;
    u8  canal;
    bool activa;
    bool loop;
} CanalMusica;

CanalMusica mus_melodia;
CanalMusica mus_bajo;
u16 mus_tempo = 6;
u16 mus_tick_counter = 0;
bool mus_activa = FALSE;
u8 mus_volumen_melodia = 7; 
u8 mus_volumen_bajo = 9;   

// --- MELODÍAS ---
const MusicNote mel_titulo[] = {
    {N_MI4, 2}, {N_SOL4, 2}, {N_LA4, 4}, {N_SOL4, 2},
    {N_MI4, 2}, {N_RE4, 4}, {N_DO4, 2}, {N_RE4, 2},
    {N_MI4, 8}, {N_SIL, 2},
    {N_SOL4, 2}, {N_LA4, 2}, {N_SI4, 4}, {N_LA4, 2},
    {N_SOL4, 2}, {N_MI4, 4}, {N_RE4, 2}, {N_MI4, 2},
    {N_DO4, 8}, {N_SIL, 4},
};
const MusicNote baj_titulo[] = {
    {N_DO3, 4}, {N_DO3, 4}, {N_SOL3, 4}, {N_SOL3, 4},
    {N_LA3, 4}, {N_LA3, 4}, {N_FA3, 4}, {N_FA3, 4},
    {N_DO3, 4}, {N_DO3, 4}, {N_SOL3, 4}, {N_SOL3, 4},
    {N_DO3, 8}, {N_SOL3, 8},
};

const MusicNote mel_menu[] = {
    {N_SOL4, 4}, {N_MI4, 4}, {N_DO4, 4}, {N_RE4, 4},
    {N_MI4, 4}, {N_FA4, 4}, {N_SOL4, 8},
    {N_LA4, 4}, {N_SOL4, 4}, {N_FA4, 4}, {N_MI4, 4},
    {N_RE4, 4}, {N_DO4, 4}, {N_RE4, 8},
};
const MusicNote baj_menu[] = {
    {N_DO3, 8}, {N_FA3, 8}, {N_SOL3, 8}, {N_DO3, 8},
    {N_FA3, 8}, {N_SOL3, 8}, {N_DO3, 8}, {N_SOL3, 8},
};

const MusicNote mel_bosque[] = {
    {N_DO4, 2}, {N_MI4, 2}, {N_SOL4, 2}, {N_DO5, 4},
    {N_SI4, 2}, {N_SOL4, 2}, {N_MI4, 4}, {N_SIL, 2},
    {N_LA4, 2}, {N_DO5, 2}, {N_MI5, 4}, {N_RE5, 2},
    {N_DO5, 2}, {N_LA4, 2}, {N_SOL4, 4}, {N_SIL, 2},
    {N_FA4, 2}, {N_LA4, 2}, {N_DO5, 4}, {N_SI4, 2},
    {N_LA4, 2}, {N_SOL4, 2}, {N_FA4, 4}, {N_SIL, 2},
    {N_SOL4, 2}, {N_MI4, 2}, {N_DO4, 4}, {N_RE4, 2},
    {N_MI4, 2}, {N_FA4, 2}, {N_SOL4, 8}, {N_SIL, 4},
};
const MusicNote baj_bosque[] = {
    {N_DO3, 4}, {N_DO3, 4}, {N_SOL3, 4}, {N_SOL3, 4},
    {N_LA3, 4}, {N_LA3, 4}, {N_FA3, 4}, {N_FA3, 4},
    {N_DO3, 4}, {N_DO3, 4}, {N_SOL3, 4}, {N_SOL3, 4},
    {N_DO3, 8}, {N_SOL3, 8},
};

const MusicNote mel_volcan[] = {
    {N_LA3, 2}, {N_DO4, 2}, {N_MI4, 4}, {N_RE4, 2},
    {N_DO4, 2}, {N_LA3, 4}, {N_SIL, 2}, {N_SI3, 2},
    {N_DO4, 4}, {N_RE4, 2}, {N_MI4, 4}, {N_SIL, 2},
    {N_FA4, 2}, {N_MI4, 2}, {N_RE4, 4}, {N_DO4, 2},
    {N_LA3, 2}, {N_SI3, 2}, {N_DO4, 8}, {N_SIL, 4},
};
const MusicNote baj_volcan[] = {
    {N_LA3, 4}, {N_LA3, 4}, {N_FA3, 4}, {N_FA3, 4},
    {N_SOL3, 4}, {N_SOL3, 4}, {N_MI3, 4}, {N_MI3, 4},
    {N_LA3, 8}, {N_FA3, 8},
};

const MusicNote mel_marino[] = {
    {N_SOL3, 4}, {N_SI3, 4}, {N_RE4, 4}, {N_DO4, 4},
    {N_SI3, 4}, {N_LA3, 4}, {N_SOL3, 8},
    {N_FA3, 4}, {N_LA3, 4}, {N_DO4, 4}, {N_SI3, 4},
    {N_LA3, 4}, {N_SOL3, 4}, {N_FA3, 8},
};
const MusicNote baj_marino[] = {
    {N_DO3, 8}, {N_FA3, 8}, {N_SOL3, 8}, {N_DO3, 8},
    {N_FA3, 8}, {N_SOL3, 8}, {N_DO3, 8}, {N_FA3, 8},
};

const MusicNote mel_celeste[] = {
    {N_DO5, 4}, {N_SOL4, 4}, {N_MI4, 4}, {N_DO4, 4},
    {N_RE4, 4}, {N_FA4, 4}, {N_LA4, 8},
    {N_SI4, 4}, {N_SOL4, 4}, {N_MI4, 4}, {N_DO4, 4},
    {N_RE4, 4}, {N_FA4, 4}, {N_SOL4, 8},
};
const MusicNote baj_celeste[] = {
    {N_DO3, 8}, {N_SOL3, 8}, {N_LA3, 8}, {N_FA3, 8},
    {N_DO3, 8}, {N_SOL3, 8}, {N_FA3, 8}, {N_DO3, 8},
};

const MusicNote mel_gameover[] = {
    {N_LA4, 4}, {N_SI4, 4}, {N_DO5, 8}, {N_SIL, 2},
    {N_SI4, 4}, {N_LA4, 4}, {N_SOL4, 8}, {N_SIL, 2},
    {N_FA4, 4}, {N_MI4, 4}, {N_RE4, 8}, {N_SIL, 2},
    {N_DO4, 4}, {N_RE4, 4}, {N_MI4, 16}, {N_SIL, 8},
};
const MusicNote baj_gameover[] = {
    {N_LA3, 8}, {N_LA3, 8}, {N_FA3, 8}, {N_FA3, 8},
    {N_DO3, 8}, {N_DO3, 8}, {N_SOL3, 16}, {N_SIL, 8},
};

const MusicNote mel_victoria[] = {
    {N_DO4, 2}, {N_MI4, 2}, {N_SOL4, 2}, {N_DO5, 4},
    {N_SOL4, 2}, {N_MI4, 2}, {N_DO4, 4}, {N_SIL, 2},
    {N_FA4, 2}, {N_LA4, 2}, {N_DO5, 4}, {N_FA5, 4},
    {N_MI5, 2}, {N_DO5, 2}, {N_LA4, 4}, {N_SIL, 2},
    {N_SOL4, 2}, {N_SI4, 2}, {N_RE5, 4}, {N_SOL5, 8},
    {N_DO5, 16}, {N_SIL, 8},
};
const MusicNote baj_victoria[] = {
    {N_DO3, 4}, {N_SOL3, 4}, {N_DO3, 4}, {N_SOL3, 4},
    {N_FA3, 4}, {N_DO3, 4}, {N_FA3, 4}, {N_DO3, 4},
    {N_SOL3, 4}, {N_RE3, 4}, {N_SOL3, 8}, {N_DO3, 16},
};

typedef struct {
    s16 x;
    s16 y;
    s16 x_vel;
    s16 y_vel;
    Sprite* sprite;
    u8  anim_actual;
    u32 puntuacion;
} Pajaro;

typedef struct {
    s16 x;
    s16 y;
    s16 x_vel;
    s16 y_vel;
    Sprite* sprite;
    u8  anim_actual;
    u16 velocidad_timer;
} Cazador;

Pajaro flicky;
Cazador cazador;
u8 estado_actual;

u16 d_sel = 1; 
u16 max_polluelos_nivel = 10; 
u16 semillas_en_fila = 0;
u16 total_salvados = 0;
Sprite* sprites_seguidores[15];

s16 historial_x[MAX_HISTORIAL];
s16 historial_y[MAX_HISTORIAL];
u16 indice_historial = 0;

s32 tiempo_restante_segundos = 300;
u32 frame_counter = 0;

s16 timer_sonido_recogida = 0;
s16 timer_sonido_deposito = 0;
s16 timer_audio_menu = 0;
s16 timer_sonido_panico = 0;

s16 timer_cazador_lento = 0;
s16 timer_flicky_rapido = 0;
s16 timer_spawn_powerup = 0;

s16 continues_restantes = 1;
u32 puntuacion_maxima = 0;
u32 proximo_hito_continue = 2500; 
bool juego_pausado = FALSE;

bool cazador_en_furia = FALSE;
s16 timer_duracion_furia = 0;

// ===== VARIABLES: INMUNIDAD Y DISPERSIÓN =====
bool flicky_invulnerable = FALSE;
s16 timer_invulnerabilidad = 0;

#define NIDO_X 17
#define NIDO_Y 12  
#define NIDO_ANCHO 10

s16 posiciones_polluelos_x[15];
s16 posiciones_polluelos_y[15];

u16 modo_juego_seleccionada = 0; 
u16 mundo_actual = MUNDO_BOSQUE;
u16 fase_actual = 1;             

u16 modo_jugadores = 0; 
u32 total_partidas = 0;
u32 victorias_pajaro = 0;
u32 victorias_cazador = 0;
u16 multiplicador_velocidad_minuto = 0; 

bool modo_battle_royale = FALSE;
u16 anillo_recorte = 0;         
bool en_preaviso_cierre = FALSE; 
bool salida_abierta = FALSE;     

u16 versus_sel = 0;
u16 versus_tiempo_idx = 2; 
u16 versus_victorias_req = 3; 
u16 versus_mundo_idx = 4;  // 0=Bosque, 1=Volcán, 2=Marino, 3=Celeste, 4=Aleatorio

bool alcanzable[MAP_HEIGHT][MAP_WIDTH];
static s16 cola_bfs_x[MAP_WIDTH * MAP_HEIGHT];
static s16 cola_bfs_y[MAP_WIDTH * MAP_HEIGHT];

u16 color_muro_original;
u8 mapa[MAP_HEIGHT][MAP_WIDTH];

// ===== SCREEN SHAKE SIMPLIFICADO =====
s16 shake_x = 0;
u16 shake_timer = 0;
u16 shake_intensidad = 2;

void efecto_shake(u16 duracion, u16 intensidad) {
    shake_timer = duracion;
    shake_intensidad = intensidad;
}

void efecto_shake_actualizar() {
    if (shake_timer > 0) {
        shake_x = (random() % (shake_intensidad * 2 + 1)) - shake_intensidad;
        VDP_setHorizontalScroll(BG_A, shake_x);
        shake_timer--;
    } else if (shake_x != 0) {
        shake_x = 0;
        VDP_setHorizontalScroll(BG_A, 0);
    }
}

// ===== TEXTO ANIMADO =====
void texto_animado(const char* texto, u16 x, u16 y, u16 velocidad) {
    u16 len = strlen(texto);
    for (u16 i = 0; i < len; i++) {
        char buf[2];
        buf[0] = texto[i];
        buf[1] = 0;
        VDP_drawText(buf, x + i, y);
        for (u16 d = 0; d < velocidad; d++) {
            SYS_doVBlankProcess();
        }
    }
}

// ===== FUNCIONES DE MÚSICA =====
void musica_iniciar(const MusicNote* melodia, u16 len_mel,
                    const MusicNote* bajo, u16 len_baj,
                    bool loop) {
    mus_melodia.notas = melodia;
    mus_melodia.longitud = len_mel;
    mus_melodia.posicion = 0;
    mus_melodia.contador = 0;
    mus_melodia.canal = 0;
    mus_melodia.activa = TRUE;
    mus_melodia.loop = loop;

    mus_bajo.notas = bajo;
    mus_bajo.longitud = len_baj;
    mus_bajo.posicion = 0;
    mus_bajo.contador = 0;
    mus_bajo.canal = 1;
    mus_bajo.activa = (bajo != NULL);
    mus_bajo.loop = loop;

    mus_tick_counter = 0;
    mus_activa = TRUE;
}

void musica_detener() {
    mus_activa = FALSE;
    mus_melodia.activa = FALSE;
    mus_bajo.activa = FALSE;
    PSG_setEnvelope(0, 15);
    PSG_setEnvelope(1, 15);
    PSG_setEnvelope(3, 15);
}

void musica_pausar() {
    mus_activa = FALSE;
    PSG_setEnvelope(0, 15);
    PSG_setEnvelope(1, 15);
}

void musica_reanudar() {
    mus_activa = TRUE;
}

void musica_actualizar() {
    if (!mus_activa) return;

    mus_tick_counter++;
    if (mus_tick_counter < mus_tempo) return;
    mus_tick_counter = 0;

    if (mus_melodia.activa) {
        if (mus_melodia.contador == 0) {
            if (mus_melodia.posicion >= mus_melodia.longitud) {
                if (mus_melodia.loop) {
                    mus_melodia.posicion = 0;
                } else {
                    mus_melodia.activa = FALSE;
                    PSG_setEnvelope(mus_melodia.canal, 15);
                    return;
                }
            }
            MusicNote n = mus_melodia.notas[mus_melodia.posicion];
            mus_melodia.contador = n.dur;
            if (n.tono == N_SIL) {
                PSG_setEnvelope(mus_melodia.canal, 15);
            } else {
                PSG_setTone(mus_melodia.canal, n.tono);
                PSG_setEnvelope(mus_melodia.canal, mus_volumen_melodia);
            }
            mus_melodia.posicion++;
        } else {
            mus_melodia.contador--;
            if (mus_melodia.contador == 1) {
                u8 de_vol = mus_volumen_melodia + 2;
                if (de_vol > 15) de_vol = 15;
                PSG_setEnvelope(mus_melodia.canal, de_vol);
            }
        }
    }

    if (mus_bajo.activa) {
        if (mus_bajo.contador == 0) {
            if (mus_bajo.posicion >= mus_bajo.longitud) {
                if (mus_bajo.loop) {
                    mus_bajo.posicion = 0;
                } else {
                    mus_bajo.activa = FALSE;
                    PSG_setEnvelope(mus_bajo.canal, 15);
                    return;
                }
            }
            MusicNote n = mus_bajo.notas[mus_bajo.posicion];
            mus_bajo.contador = n.dur;
            if (n.tono == N_SIL) {
                PSG_setEnvelope(mus_bajo.canal, 15);
            } else {
                PSG_setTone(mus_bajo.canal, n.tono);
                PSG_setEnvelope(mus_bajo.canal, mus_volumen_bajo);
            }
            mus_bajo.posicion++;
        } else {
            mus_bajo.contador--;
        }
    }
}

void musica_segun_estado(u8 estado) {
    switch (estado) {
        case ESTADO_TITULO:
            mus_tempo = 7;
            musica_iniciar(mel_titulo, sizeof(mel_titulo)/sizeof(MusicNote),
                           baj_titulo, sizeof(baj_titulo)/sizeof(MusicNote), TRUE);
            break;
        case ESTADO_MENU:
        case ESTADO_MENU_VERSUS:
            mus_tempo = 6;
            musica_iniciar(mel_menu, sizeof(mel_menu)/sizeof(MusicNote),
                           baj_menu, sizeof(baj_menu)/sizeof(MusicNote), TRUE);
            break;
        case ESTADO_INTRO:
        case ESTADO_COMO_JUGAR:
            musica_pausar();
            break;
        case ESTADO_JUEGO:
            mus_tempo = 4;
            switch (mundo_actual) {
                case MUNDO_BOSQUE:
                    musica_iniciar(mel_bosque, sizeof(mel_bosque)/sizeof(MusicNote),
                                   baj_bosque, sizeof(baj_bosque)/sizeof(MusicNote), TRUE);
                    break;
                case MUNDO_VOLCAN:
                    musica_iniciar(mel_volcan, sizeof(mel_volcan)/sizeof(MusicNote),
                                   baj_volcan, sizeof(baj_volcan)/sizeof(MusicNote), TRUE);
                    break;
                case MUNDO_MARINO:
                    musica_iniciar(mel_marino, sizeof(mel_marino)/sizeof(MusicNote),
                                   baj_marino, sizeof(baj_marino)/sizeof(MusicNote), TRUE);
                    break;
                case MUNDO_CELESTE:
                    musica_iniciar(mel_celeste, sizeof(mel_celeste)/sizeof(MusicNote),
                                   baj_celeste, sizeof(baj_celeste)/sizeof(MusicNote), TRUE);
                    break;
            }
            break;
        case ESTADO_GAMEOVER:
            mus_tempo = 10;
            musica_iniciar(mel_gameover, sizeof(mel_gameover)/sizeof(MusicNote),
                           baj_gameover, sizeof(baj_gameover)/sizeof(MusicNote), FALSE);
            break;
        case ESTADO_FINJUEGO:
            mus_tempo = 5;
            musica_iniciar(mel_victoria, sizeof(mel_victoria)/sizeof(MusicNote),
                           baj_victoria, sizeof(baj_victoria)/sizeof(MusicNote), FALSE);
            break;
        case ESTADO_CONTINUE:
            musica_pausar();
            break;
    }
}

// ===== EFECTOS DE SONIDO (CANAL 2) =====
void playMenuClickSound() { 
    PSG_setTone(2, 650); 
    PSG_setEnvelope(2, 2); 
    timer_audio_menu = 4;
}

void playTimeAlertSound() {
    PSG_setTone(2, 1150);
    PSG_setEnvelope(2, 2);
    timer_sonido_recogida = 3;
}

void reproducir_sonido_waka() {
    PSG_setTone(2, 500);
    PSG_setEnvelope(2, 2);
    timer_sonido_recogida = 4;
}

void reproducir_sonido_deposito(u16 n) {
    u16 f[] = {580, 680, 800, 950, 1200};
    PSG_setTone(2, f[n > 4 ? 4 : n]);
    PSG_setEnvelope(2, 2);
    timer_sonido_deposito = 6;
}

void reproducir_sonido_item(u16 tono) {
    PSG_setTone(2, tono);
    PSG_setEnvelope(2, 1);
    timer_sonido_recogida = 8;
}

// ===== SONIDO DE PÁNICO (descendente) =====
void reproducir_sonido_panico() {
    PSG_setTone(2, 800);
    PSG_setEnvelope(2, 1);
    timer_sonido_panico = 20;
}

void procesar_silencio_psg() {
    if (timer_sonido_panico > 0) {
        timer_sonido_panico--;
        u16 tono = 800 - (u16)(20 - timer_sonido_panico) * 25;
        if (tono < 300) tono = 300;
        PSG_setTone(2, tono);
        if (timer_sonido_panico == 0) PSG_setEnvelope(2, 15);
    }
    else if (timer_sonido_recogida > 0) { 
        timer_sonido_recogida--; 
        if (timer_sonido_recogida == 2) PSG_setTone(2, 650); 
        if (timer_sonido_recogida == 0) PSG_setEnvelope(2, 15); 
    }
    if (timer_sonido_deposito > 0) { 
        timer_sonido_deposito--; 
        if (timer_sonido_deposito == 0) PSG_setEnvelope(2, 15); 
    }
}

// ===== DECLARACIONES DE FUNCIONES =====
bool en_zona_nido(s16 tx, s16 ty);
void mostrar_pantalla_titulo();
void mostrar_menu_opciones();
void mostrar_pantalla_como_jugar();
void mostrar_menu_versus();
void mostrar_intro_mundo_actual();
void inicializar_juego();
void resetear_tiempo_nivel();
void leer_entrada();
void actualizar_logica();
void mover_cazador();
void generar_mapa_aleatorio();
void actualizar_anim_direccion(u8* anim_actual, Sprite* sprite, s16 x_vel, s16 y_vel, bool mirar_izquierda_por_defecto);
void repartir_polluelos_seguros();
void calcular_alcanzables();
bool buscar_celda_libre(s16* out_x, s16* out_y);
void generar_powerup_aleatorio();
void finalizar_partida();
void aplicar_recorte_battle_royale();
void comprobar_recogida_objetos();
void mostrar_pantalla_continue();
void mostrar_game_over();
void mostrar_enhorabuena();
void verificar_fin_ronda_versus(bool gano_pajaro);

// ===== NUEVAS FUNCIONES: INMUNIDAD Y DISPERSIÓN =====
void dispersar_polluelos() {
    for (u16 i = 0; i < semillas_en_fila; i++) {
        if (sprites_seguidores[i]) {
            SPR_releaseSprite(sprites_seguidores[i]);
            sprites_seguidores[i] = NULL;
        }
    }
    
    u16 cx = flicky.x >> 3;
    u16 cy = flicky.y >> 3;
    u16 dispersados = 0;
    u16 intentos = 0;
    
    while (dispersados < semillas_en_fila && intentos < 300) {
        intentos++;
        s16 rx = (s16)cx + (random() % (RADIO_DISPERSION * 2 + 1)) - RADIO_DISPERSION;
        s16 ry = (s16)cy + (random() % (RADIO_DISPERSION * 2 + 1)) - RADIO_DISPERSION;
        
        if (rx >= 1 && rx < MAP_WIDTH - 1 && ry >= 1 && ry < MAP_HEIGHT - 1) {
            if (mapa[ry][rx] == TILE_SUELO && !en_zona_nido(rx, ry)) {
                mapa[ry][rx] = TILE_POLLUELO;
                VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX + TILE_POLLUELO), rx, ry);
                dispersados++;
            }
        }
    }
    
    semillas_en_fila = 0;
}

void actualizar_invulnerabilidad() {
    if (!flicky_invulnerable) return;
    
    timer_invulnerabilidad--;
    
    if ((timer_invulnerabilidad >> 2) & 1) {
        SPR_setVisibility(flicky.sprite, HIDDEN);
    } else {
        SPR_setVisibility(flicky.sprite, VISIBLE);
    }
    
    if (timer_invulnerabilidad <= 0) {
        flicky_invulnerable = FALSE;
        SPR_setVisibility(flicky.sprite, VISIBLE);
    }
}

int main(bool hard) {
    VDP_init();
    SYS_disableInts();
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    SYS_enableInts();
    SPR_init();

    for(u8 i = 0; i < 4; i++) {
        PSG_setEnvelope(i, 15);
    }

    JOY_init();

    PAL_setPalette(PAL0, bkg_pal.data, CPU);                  
    PAL_setPalette(PAL1, bg_forest.palette->data, CPU);       
    PAL_setPalette(PAL2, flicky_sprite.palette->data, CPU);   
    PAL_setPalette(PAL3, cazador_sprite.palette->data, CPU);   

    color_muro_original = bkg_pal.data[1];

    estado_actual = ESTADO_TITULO;
    mostrar_pantalla_titulo();

    while(1) {
        u16 joy = JOY_readJoypad(JOY_1);
        static bool boton_pulsado = FALSE;

        if (timer_audio_menu > 0) {
            timer_audio_menu--;
            if (timer_audio_menu == 0) PSG_setEnvelope(2, 15);
        }

        if (estado_actual == ESTADO_TITULO) {
            if (joy & BUTTON_START) {
                playMenuClickSound();
                estado_actual = ESTADO_MENU;
                musica_segun_estado(ESTADO_MENU);
                mostrar_menu_opciones();
                boton_pulsado = TRUE;
                waitMs(200); 
            }
        } 
        else if (estado_actual == ESTADO_MENU) {
            if (!boton_pulsado) {
                if (joy & BUTTON_UP) { if (d_sel > 0) { d_sel--; playMenuClickSound(); mostrar_menu_opciones(); } boton_pulsado = TRUE; }
                else if (joy & BUTTON_DOWN) { if (d_sel < 2) { d_sel++; playMenuClickSound(); mostrar_menu_opciones(); } boton_pulsado = TRUE; }
                else if (joy & BUTTON_LEFT) { if (modo_jugadores > 0) { modo_jugadores--; playMenuClickSound(); mostrar_menu_opciones(); } boton_pulsado = TRUE; }
                else if (joy & BUTTON_RIGHT) { if (modo_jugadores < 1) { modo_jugadores++; playMenuClickSound(); mostrar_menu_opciones(); } boton_pulsado = TRUE; }
                
                else if (joy & BUTTON_A) { if (modo_juego_seleccionada != 0) { modo_juego_seleccionada = 0; playMenuClickSound(); mostrar_menu_opciones(); } boton_pulsado = TRUE; }
                else if (joy & BUTTON_B) { if (modo_juego_seleccionada != 1) { modo_juego_seleccionada = 1; playMenuClickSound(); mostrar_menu_opciones(); } boton_pulsado = TRUE; }
                
                else if (joy & BUTTON_C) { modo_battle_royale = !modo_battle_royale; playMenuClickSound(); mostrar_menu_opciones(); boton_pulsado = TRUE; }
                
                else if (joy & BUTTON_START) {
                    total_partidas++;
                    fase_actual = 1;
                    mundo_actual = MUNDO_BOSQUE;
                    continues_restantes = 1;
                    proximo_hito_continue = 2500; 
                    flicky.puntuacion = 0;
                    juego_pausado = FALSE;

                    victorias_pajaro = 0;
                    victorias_cazador = 0;

                    if (modo_jugadores == 1) {
                        estado_actual = ESTADO_MENU_VERSUS;
                        musica_segun_estado(ESTADO_MENU_VERSUS);
                        versus_sel = 0;
                        mostrar_menu_versus();
                    } else {
                        if (modo_juego_seleccionada == 1) {
                            mundo_actual = random() % 4; 
                        }
                        estado_actual = ESTADO_COMO_JUGAR;
                        musica_segun_estado(ESTADO_COMO_JUGAR);
                        mostrar_pantalla_como_jugar();
                    }
                    boton_pulsado = TRUE;
                    waitMs(200);
                }
            }
            if (!(joy & (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT | BUTTON_A | BUTTON_B | BUTTON_C | BUTTON_START))) { boton_pulsado = FALSE; }
        }
        else if (estado_actual == ESTADO_MENU_VERSUS) {
            if (!boton_pulsado) {
                // versus_sel ahora llega hasta 4 para incluir la opción "Volver"
                if (joy & BUTTON_UP) { if (versus_sel > 0) { versus_sel--; playMenuClickSound(); mostrar_menu_versus(); } boton_pulsado = TRUE; }
                else if (joy & BUTTON_DOWN) { if (versus_sel < 4) { versus_sel++; playMenuClickSound(); mostrar_menu_versus(); } boton_pulsado = TRUE; }
                
                else if (joy & BUTTON_LEFT || joy & BUTTON_A) {
                    if (versus_sel == 0) modo_battle_royale = !modo_battle_royale;
                    else if (versus_sel == 1) { if (versus_tiempo_idx > 0) versus_tiempo_idx--; }
                    else if (versus_sel == 2) { if (versus_victorias_req > 1) versus_victorias_req -= 2; }
                    else if (versus_sel == 3) { 
                        if (versus_mundo_idx > 0) versus_mundo_idx--; 
                        else versus_mundo_idx = 4; 
                    }
                    else if (versus_sel == 4) {
                        // Volver al menú principal
                        estado_actual = ESTADO_MENU;
                        musica_segun_estado(ESTADO_MENU);
                        mostrar_menu_opciones();
                        boton_pulsado = TRUE;
                        waitMs(200);
                        return 0;
                    }
                    playMenuClickSound();
                    mostrar_menu_versus();
                    boton_pulsado = TRUE;
                }
                else if (joy & BUTTON_RIGHT || joy & BUTTON_B) {
                    if (versus_sel == 0) modo_battle_royale = !modo_battle_royale;
                    else if (versus_sel == 1) { if (versus_tiempo_idx < 3) versus_tiempo_idx++; }
                    else if (versus_sel == 2) { if (versus_victorias_req < 5) versus_victorias_req += 2; }
                    else if (versus_sel == 3) { 
                        if (versus_mundo_idx < 4) versus_mundo_idx++; 
                        else versus_mundo_idx = 0; 
                    }
                    else if (versus_sel == 4) {
                        // Volver al menú principal
                        estado_actual = ESTADO_MENU;
                        musica_segun_estado(ESTADO_MENU);
                        mostrar_menu_opciones();
                        boton_pulsado = TRUE;
                        waitMs(200);
                        return 0;
                    }
                    playMenuClickSound();
                    mostrar_menu_versus();
                    boton_pulsado = TRUE;
                }
                else if (joy & BUTTON_START) {
                    playMenuClickSound();
                    if (versus_sel == 4) {
                        // Volver al menú principal al pulsar START en la última opción
                        estado_actual = ESTADO_MENU;
                        musica_segun_estado(ESTADO_MENU);
                        mostrar_menu_opciones();
                    } else {
                        resetear_tiempo_nivel();
                        max_polluelos_nivel = 10;
                        if (versus_mundo_idx == 4) {
                            mundo_actual = random() % 4; 
                        } else {
                            mundo_actual = versus_mundo_idx;
                        }
                        VDP_clearPlane(BG_A, TRUE);
                        inicializar_juego();
                        estado_actual = ESTADO_JUEGO;
                        musica_segun_estado(ESTADO_JUEGO);
                    }
                    boton_pulsado = TRUE;
                    waitMs(200);
                }
            }
            if (!(joy & (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT | BUTTON_A | BUTTON_B | BUTTON_START))) { boton_pulsado = FALSE; }
        }
        else if (estado_actual == ESTADO_COMO_JUGAR) {
            if (joy & BUTTON_START) {
                playMenuClickSound();
                estado_actual = ESTADO_INTRO;
                musica_segun_estado(ESTADO_INTRO);
                mostrar_intro_mundo_actual();
                boton_pulsado = TRUE;
                waitMs(200);
            }
        }
        else if (estado_actual == ESTADO_INTRO) {
            if (joy & BUTTON_START) {
                playMenuClickSound();
                resetear_tiempo_nivel();
                max_polluelos_nivel = (d_sel == 0) ? 5 : ((d_sel == 1) ? 10 : 15);
                VDP_clearPlane(BG_A, TRUE);
                inicializar_juego();
                estado_actual = ESTADO_JUEGO;
                musica_segun_estado(ESTADO_JUEGO);
                waitMs(200);
            }
        }
        else if (estado_actual == ESTADO_CONTINUE) {
            if (joy & BUTTON_START) {
                playMenuClickSound();
                continues_restantes--;
                resetear_tiempo_nivel();
                VDP_clearPlane(BG_A, TRUE);
                inicializar_juego();
                estado_actual = ESTADO_JUEGO;
                musica_segun_estado(ESTADO_JUEGO);
                waitMs(200);
            }
        }
        else if (estado_actual == ESTADO_GAMEOVER || estado_actual == ESTADO_FINJUEGO) {
            if (joy & BUTTON_START) {
                playMenuClickSound();
                estado_actual = ESTADO_MENU;
                musica_segun_estado(ESTADO_MENU);
                mostrar_menu_opciones();
                waitMs(200);
            }
        }
        else if (estado_actual == ESTADO_JUEGO) {
            u16 joy2 = (modo_jugadores == 1) ? JOY_readJoypad(JOY_2) : 0;
            if ((joy & BUTTON_START) || (joy2 & BUTTON_START)) {
                playMenuClickSound();
                juego_pausado = !juego_pausado;
                if (juego_pausado) {
                    VDP_drawText("PAUSA", 18, 13);
                    musica_pausar();
                } else {
                    VDP_drawText("      ", 18, 13); 
                    musica_reanudar();
                }
                waitMs(250); 
            }

            if (!juego_pausado) {
                leer_entrada();
                actualizar_logica();
                procesar_silencio_psg();
            }
        }

        musica_actualizar();
        SPR_update();
        SYS_doVBlankProcess();
    }
    return 0;
}

void resetear_tiempo_nivel() {
    if (modo_jugadores == 1) {
        if (versus_tiempo_idx == 0) tiempo_restante_segundos = 60;
        else if (versus_tiempo_idx == 1) tiempo_restante_segundos = 90;
        else if (versus_tiempo_idx == 2) tiempo_restante_segundos = 120;
        else tiempo_restante_segundos = 180;
    } else {
        if (modo_battle_royale) {
            tiempo_restante_segundos = 120; 
        } else {
            if (d_sel == 0) { tiempo_restante_segundos = 120; }
            else if (d_sel == 1) { tiempo_restante_segundos = 180; }
            else { tiempo_restante_segundos = 300; }
        }
    }
}

void mostrar_pantalla_titulo() {
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    PAL_setPalette(PAL0, bg_title.palette->data, CPU);
    VDP_drawImageEx(BG_B, &bg_title, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);
    VDP_setTextPalette(PAL0);
    PAL_setColor(15, RGB24_TO_VDPCOLOR(0xFFFFFF));
    
    musica_segun_estado(ESTADO_TITULO);
    texto_animado("PRESIONA START", 13, 22, 3);
}

void mostrar_menu_opciones() {
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    char buf[40];

    PAL_setPalette(PAL1, bg_opciones.palette->data, CPU);
    VDP_drawImageEx(BG_B, &bg_opciones, TILE_ATTR_FULL(PAL1, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);

    VDP_setTextPalette(PAL2);
    PAL_setColor(47, RGB24_TO_VDPCOLOR(0xFFFFFF)); 

    musica_segun_estado(ESTADO_MENU);

    sprintf(buf, "PARTIDAS JUGADAS: %ld", total_partidas);
    VDP_drawText(buf, 4, 2);
    sprintf(buf, "PUNTOS MAXIMOS:   %06ld", puntuacion_maxima);
    VDP_drawText(buf, 4, 3);
    
    sprintf(buf, "WINS P1 (PAJ): %ld | P2 (CAZ): %ld", victorias_pajaro, victorias_cazador);
    VDP_drawText(buf, 4, 4);

    VDP_drawText("MODO DE JUEGO (IZQ/DER):", 4, 7);
    VDP_drawText(modo_jugadores == 0 ? "> 1 JUGADOR  (VS CPU)" : "  1 JUGADOR  (VS CPU)", 6, 8);
    VDP_drawText(modo_jugadores == 1 ? "> 2 JUGADORES (MANUAL)" : "  2 JUGADORES (MANUAL)", 6, 9);

    VDP_drawText("ESTRUCTURA PARTIDA (PULSA A/B):", 4, 12);
    VDP_drawText(modo_juego_seleccionada == 0 ? "> MODO HISTORIA (4 MUNDOS x 5 FASES)" : "  MODO HISTORIA (4 MUNDOS x 5 FASES)", 6, 13);
    VDP_drawText(modo_juego_seleccionada == 1 ? "> MODO LIBRE    (1 MAPA RANDOM)" : "  MODO LIBRE    (1 MAPA RANDOM)", 6, 14);

    VDP_drawText("MODO ESPECIAL (PULSA C):", 4, 17);
    VDP_drawText(modo_battle_royale ? "> BATTLE ROYALE: ACTIVO!" : "  BATTLE ROYALE: DESACT.", 6, 18);

    VDP_drawText("SELECCIONA DIFICULTAD (ARR/ABA):", 4, 21);
    VDP_drawText(d_sel == 0 ? "> FACIL    (2 MIN / 5 HUEVOS)" : "  FACIL    (2 MIN / 5 HUEVOS)", 6, 22);
    VDP_drawText(d_sel == 1 ? "> MEDIO   (3 MIN / 10 HUEVOS)" : "  MEDIO   (3 MIN / 10 HUEVOS)", 6, 23);
    VDP_drawText(d_sel == 2 ? "> DIFICIL (5 MIN / 15 HUEVOS)" : "  DIFICIL (5 MIN / 15 HUEVOS)", 6, 24);
}

void mostrar_pantalla_como_jugar() {
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    PAL_setPalette(PAL1, bg_como_jugar.palette->data, CPU);
    VDP_drawImageEx(BG_B, &bg_como_jugar, TILE_ATTR_FULL(PAL1, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);
}

// ===== MENÚ VERSUS ACTUALIZADO CON SELECCIÓN DE ESCENARIO Y "VOLVER AL MENÚ" =====
void mostrar_menu_versus() {
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    char buf[40];

    PAL_setPalette(PAL0, bg_versus.palette->data, CPU);
    VDP_drawImageEx(BG_B, &bg_versus, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);
    
    VDP_setTextPalette(PAL0);
    PAL_setColor(15, RGB24_TO_VDPCOLOR(0xFFFFFF));

    VDP_drawText("CONFIGURACION VERSUS", 10, 3);
    VDP_drawText("====================", 10, 4);

    sprintf(buf, "%s BATTLE ROYALE: %s", (versus_sel == 0) ? ">" : " ", modo_battle_royale ? "ON " : "OFF");
    VDP_drawText(buf, 7, 7);

    u16 segs = (versus_tiempo_idx == 0) ? 60 : ((versus_tiempo_idx == 1) ? 90 : ((versus_tiempo_idx == 2) ? 120 : 180));
    sprintf(buf, "%s TIEMPO RONDA:  %d SEG", (versus_sel == 1) ? ">" : " ", segs);
    VDP_drawText(buf, 7, 10);

    sprintf(buf, "%s AL MEJOR DE:   %d WINS", (versus_sel == 2) ? ">" : " ", versus_victorias_req);
    VDP_drawText(buf, 7, 13);

    const char* nombre_mundo;
    switch (versus_mundo_idx) {
        case 0: nombre_mundo = "BOSQUE  "; break;
        case 1: nombre_mundo = "VOLCAN  "; break;
        case 2: nombre_mundo = "MARINO  "; break;
        case 3: nombre_mundo = "CELESTE "; break;
        default: nombre_mundo = "ALEATORIO"; break;
    }
    sprintf(buf, "%s ESCENARIO:     %s", (versus_sel == 3) ? ">" : " ", nombre_mundo);
    VDP_drawText(buf, 7, 16);

    // NUEVO: Opción interactiva para volver
    sprintf(buf, "%s VOLVER AL MENU PRINCIPAL", (versus_sel == 4) ? ">" : " ");
    VDP_drawText(buf, 7, 19);

    VDP_drawText("P1 (MANDO 1) = PAJARO", 7, 22);
    VDP_drawText("P2 (MANDO 2) = CAZADOR", 7, 23);

    VDP_drawText("PULSA START PARA PELEAR", 9, 25);
}

void mostrar_intro_mundo_actual() {
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    for(u8 i = 0; i < 4; i++) PSG_setEnvelope(i, 15);

    if (mundo_actual == MUNDO_BOSQUE) {
        PAL_setPalette(PAL1, bg_intro_bosque.palette->data, CPU);
        VDP_drawImageEx(BG_B, &bg_intro_bosque, TILE_ATTR_FULL(PAL1, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);
    } else if (mundo_actual == MUNDO_VOLCAN) {
        PAL_setPalette(PAL1, bg_intro_volcan.palette->data, CPU);
        VDP_drawImageEx(BG_B, &bg_intro_volcan, TILE_ATTR_FULL(PAL1, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);
    } else if (mundo_actual == MUNDO_MARINO) {
        PAL_setPalette(PAL1, bg_intro_acuatico.palette->data, CPU);
        VDP_drawImageEx(BG_B, &bg_intro_acuatico, TILE_ATTR_FULL(PAL1, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);
    } else {
        PAL_setPalette(PAL1, bg_intro_celeste.palette->data, CPU);
        VDP_drawImageEx(BG_B, &bg_intro_celeste, TILE_ATTR_FULL(PAL1, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);
    }
}

void inicializar_juego() {
    VDP_loadTileSet(&bkg_tileset, TILE_USER_INDEX, CPU);
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    
    PAL_setPalette(PAL0, bkg_pal.data, CPU);
    PAL_setPalette(PAL2, flicky_sprite.palette->data, CPU);

    const SpriteDefinition* cazador_recurso = &cazador_sprite;
    if (mundo_actual == MUNDO_VOLCAN) {
        cazador_recurso = &sprites_llama;
    } else if (mundo_actual == MUNDO_MARINO) {
        cazador_recurso = &sprites_tiburon;
    } else if (mundo_actual == MUNDO_CELESTE) {
        cazador_recurso = &sprites_demonio;
    }
    PAL_setPalette(PAL3, cazador_recurso->palette->data, CPU);

    total_salvados = 0; semillas_en_fila = 0; frame_counter = 0;
    multiplicador_velocidad_minuto = 0;
    timer_cazador_lento = 0; timer_flicky_rapido = 0;
    timer_spawn_powerup = 120; 
    anillo_recorte = 0;
    en_preaviso_cierre = FALSE;
    salida_abierta = FALSE;
    indice_historial = 0;

    cazador_en_furia = FALSE;
    timer_duracion_furia = 0;
    
    flicky_invulnerable = FALSE;
    timer_invulnerabilidad = 0;
    timer_sonido_panico = 0;
    
    shake_x = 0;
    shake_timer = 0;
    VDP_setHorizontalScroll(BG_A, 0);

    for (u16 i = 0; i < 15; i++) {
        sprites_seguidores[i] = NULL;
    }

    generar_mapa_aleatorio();

    if (mundo_actual == MUNDO_BOSQUE) {
        PAL_setPalette(PAL1, bg_forest.palette->data, CPU);
        VDP_drawImageEx(BG_B, &bg_forest, TILE_ATTR_FULL(PAL1, 0, 0, 0, TILE_USER_INDEX + bkg_tileset.numTile), 0, 0, FALSE, DMA);
    } else if (mundo_actual == MUNDO_VOLCAN) {
        PAL_setPalette(PAL1, bg_volcan.palette->data, CPU);
        VDP_drawImageEx(BG_B, &bg_volcan, TILE_ATTR_FULL(PAL1, 0, 0, 0, TILE_USER_INDEX + bkg_tileset.numTile), 0, 0, FALSE, DMA);
    } else if (mundo_actual == MUNDO_MARINO) {
        PAL_setPalette(PAL1, bg_marino.palette->data, CPU);
        VDP_drawImageEx(BG_B, &bg_marino, TILE_ATTR_FULL(PAL1, 0, 0, 0, TILE_USER_INDEX + bkg_tileset.numTile), 0, 0, FALSE, DMA);
    } else {
        PAL_setPalette(PAL1, bg_celeste.palette->data, CPU);
        VDP_drawImageEx(BG_B, &bg_celeste, TILE_ATTR_FULL(PAL1, 0, 0, 0, TILE_USER_INDEX + bkg_tileset.numTile), 0, 0, FALSE, DMA);
    }

    for(u16 y = 0; y < MAP_HEIGHT; y++) {
        for(u16 x = 0; x < MAP_WIDTH; x++) {
            if (mapa[y][x] != TILE_SUELO && mapa[y][x] != TILE_MURO) mapa[y][x] = TILE_SUELO;
            if(mapa[y][x] == TILE_MURO) {
                VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX + TILE_MURO), x, y);
            }
        }
    }

    for(u16 x = 0; x < MAP_WIDTH; x++) {
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, 0), x, 27);
        VDP_setTileMapXY(BG_B, TILE_ATTR_FULL(PAL0, 0, 0, 0, 0), x, 27);
    }

    calcular_alcanzables();

    for (u16 i = 0; i < NIDO_ANCHO; i++) {
        VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX + TILE_NIDO), NIDO_X + i, NIDO_Y);
    }

    repartir_polluelos_seguros();

    flicky.x = 16; flicky.y = 8; flicky.x_vel = 0; flicky.y_vel = 0;
    flicky.sprite = SPR_addSprite(&flicky_sprite, flicky.x - FLICKY_DRAW_OFFSET, flicky.y - FLICKY_DRAW_OFFSET, TILE_ATTR(PAL2, 0, FALSE, FALSE));
    
    for (u16 h = 0; h < MAX_HISTORIAL; h++) {
        historial_x[h] = flicky.x;
        historial_y[h] = flicky.y;
    }

    s16 caz_tx = 19, caz_ty = 10;
    buscar_celda_libre(&caz_tx, &caz_ty);
    cazador.x = caz_tx * 8; cazador.y = caz_ty * 8; cazador.x_vel = 0; cazador.y_vel = 0;
    
    cazador.sprite = SPR_addSprite(cazador_recurso, cazador.x - CAZADOR_DRAW_OFFSET, cazador.y - CAZADOR_DRAW_OFFSET, TILE_ATTR(PAL3, 0, FALSE, FALSE));

    musica_segun_estado(ESTADO_JUEGO);
}

void lauch_partida() {} 

void finalizar_partida() {
    PAL_setColor(1, color_muro_original); 
    if (flicky.sprite)  { SPR_releaseSprite(flicky.sprite);  flicky.sprite = NULL; }
    if (cazador.sprite) { SPR_releaseSprite(cazador.sprite); cazador.sprite = NULL; }
    
    cazador.x = 0;
    cazador.y = 0;
    flicky.x = 0;
    flicky.y = 0;

    flicky_invulnerable = FALSE;
    timer_invulnerabilidad = 0;

    for (u16 i = 0; i < 15; i++) {
        if (sprites_seguidores[i]) {
            SPR_releaseSprite(sprites_seguidores[i]);
            sprites_seguidores[i] = NULL;
        }
    }
    semillas_en_fila = 0;
    
    shake_x = 0;
    shake_timer = 0;
    VDP_setHorizontalScroll(BG_A, 0);
    
    SPR_clear();
    SPR_update();
    SYS_doVBlankProcess();
}

void finalizar_partida_con_error() {
    finalizar_partida();
}

void mostrar_pantalla_continue() {
    finalizar_partida();
    estado_actual = ESTADO_CONTINUE;
    musica_segun_estado(ESTADO_CONTINUE);
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    
    PAL_setColor(1, color_muro_original);
    
    VDP_setTextPalette(PAL0); 
    char buf[40];
    sprintf(buf, "CONTINUES: %d", continues_restantes);
    VDP_drawText(buf, 14, 10);
    VDP_drawText("PULSA START PARA SEGUIR", 8, 13);
}

void mostrar_game_over() {
    if (flicky.puntuacion > puntuacion_maxima) {
        puntuacion_maxima = flicky.puntuacion;
    }
    finalizar_partida();
    estado_actual = ESTADO_GAMEOVER;
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    PAL_setPalette(PAL0, bg_game_over.palette->data, CPU);
    VDP_drawImageEx(BG_B, &bg_game_over, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);
    
    musica_segun_estado(ESTADO_GAMEOVER);
    efecto_shake(10, 2);
}

void mostrar_enhorabuena() {
    if (flicky.puntuacion > puntuacion_maxima) {
        puntuacion_maxima = flicky.puntuacion;
    }
    finalizar_partida();
    estado_actual = ESTADO_FINJUEGO;
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);

    for(u8 i = 0; i < 4; i++) PSG_setEnvelope(i, 15);

    PAL_setPalette(PAL0, bg_fin_juego.palette->data, CPU);
    VDP_drawImageEx(BG_B, &bg_fin_juego, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);
    
    musica_segun_estado(ESTADO_FINJUEGO);
    efecto_shake(10, 2);
}

void verificar_fin_ronda_versus(bool gano_pajaro) {
    if (gano_pajaro) {
        victorias_pajaro++;   
    } else {
        victorias_cazador++;  
    }

    finalizar_partida();

    if (victorias_pajaro >= versus_victorias_req || victorias_cazador >= versus_victorias_req) {
        estado_actual = ESTADO_FINJUEGO;
        VDP_clearPlane(BG_A, TRUE);
        VDP_clearPlane(BG_B, TRUE);

        for(u8 i = 0; i < 4; i++) PSG_setEnvelope(i, 15);

        if (victorias_pajaro >= versus_victorias_req) {
            PAL_setPalette(PAL0, bg_pajaro_ganador.palette->data, CPU);
            VDP_drawImageEx(BG_B, &bg_pajaro_ganador, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);
        } else {
            PAL_setPalette(PAL0, bg_cazador_ganador.palette->data, CPU);
            VDP_drawImageEx(BG_B, &bg_cazador_ganador, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);
        }
        musica_segun_estado(ESTADO_FINJUEGO);
    } else {
        flicky.puntuacion = 0;
        resetear_tiempo_nivel();
        VDP_clearPlane(BG_A, TRUE);
        if (versus_mundo_idx == 4) {
            mundo_actual = random() % 4; 
        }
        inicializar_juego();
    }
}

bool en_zona_nido(s16 tx, s16 ty) {
    return (ty >= NIDO_Y - 1 && ty <= NIDO_Y + 1 && tx >= NIDO_X - 1 && tx <= NIDO_X + NIDO_ANCHO);
}

void generar_mapa_aleatorio() {
    for (u16 y = 0; y < MAP_HEIGHT; y++) {
        for (u16 x = 0; x < MAP_WIDTH; x++) {
            if (x == 0 || x == MAP_WIDTH - 1 || y == 0 || y == MAP_HEIGHT - 1) {
                mapa[y][x] = TILE_MURO;
            } else if (en_zona_nido(x, y) || (x <= 4 && y <= 3)) {
                mapa[y][x] = TILE_SUELO;
            } else {
                mapa[y][x] = ((random() % 100) < 22) ? TILE_MURO : TILE_SUELO;
            }
        }
    }
}

void calcular_alcanzables() {
    for (u16 y = 0; y < MAP_HEIGHT; y++)
        for (u16 x = 0; x < MAP_WIDTH; x++)
            alcanzable[y][x] = FALSE;

    u16 inicio = 0, fin = 0;
    s16 sx = 2, sy = 1; 

    cola_bfs_x[fin] = sx; cola_bfs_y[fin] = sy; fin++;
    alcanzable[sy][sx] = TRUE;

    s16 dx[4] = {1, -1, 0, 0};
    s16 dy[4] = {0, 0, 1, -1};

    while (inicio < fin) {
        s16 cx = cola_bfs_x[inicio], cy = cola_bfs_y[inicio];
        inicio++;
        for (u8 d = 0; d < 4; d++) {
            s16 nx = cx + dx[d], ny = cy + dy[d];
            if (nx >= 0 && nx < MAP_WIDTH && ny >= 0 && ny < MAP_HEIGHT && !alcanzable[ny][nx] && mapa[ny][nx] != TILE_MURO) {
                alcanzable[ny][nx] = TRUE;
                cola_bfs_x[fin] = nx; cola_bfs_y[fin] = ny; fin++;
            }
        }
    }
}

void repartir_polluelos_seguros() {
    u16 repartidos = 0;
    u16 intentos = 0;
    while (repartidos < max_polluelos_nivel && intentos < 500) {
        intentos++;
        u16 rx = (random() % (MAP_WIDTH - 2)) + 1;
        u16 ry = (random() % (MAP_HEIGHT - 4)) + 1;
        if (mapa[ry][rx] == TILE_SUELO && !en_zona_nido(rx, ry) && alcanzable[ry][rx]) {
            bool cerca = FALSE;
            for (u16 i = 0; i < repartidos; i++) if (abs(posiciones_polluelos_x[i] - (s16)rx) < 3 && abs(posiciones_polluelos_y[i] - (s16)ry) < 3) cerca = TRUE;
            if (!cerca) {
                mapa[ry][rx] = TILE_POLLUELO;
                posiciones_polluelos_x[repartidos] = rx;
                posiciones_polluelos_y[repartidos] = ry;
                VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX + TILE_POLLUELO), rx, ry);
                repartidos++;
            }
        }
    }
}

bool buscar_celda_libre(s16* out_x, s16* out_y) {
    for (u16 intentos = 0; intentos < 200; intentos++) {
        u16 rx = (random() % (MAP_WIDTH - 2)) + 1;
        u16 ry = (random() % (MAP_HEIGHT - 4)) + 1;
        if (mapa[ry][rx] == TILE_SUELO && !en_zona_nido(rx, ry) && alcanzable[ry][rx]) {
            *out_x = rx; *out_y = ry;
            return TRUE;
        }
    }
    return FALSE;
}

void generar_powerup_aleatorio() {
    s16 rx, ry;
    if (!buscar_celda_libre(&rx, &ry)) return;

    u8 tipo;
    u16 r = random() % 100;
    
    if (multiplicador_velocidad_minuto > 0 || cazador_en_furia) {
        if (r < 60) tipo = TILE_RELOJ;
        else if (r < 80) tipo = TILE_RAYO;
        else tipo = TILE_GEMA;
    } else {
        if (r < 25) tipo = TILE_RELOJ;
        else if (r < 50) tipo = TILE_RAYO;
        else tipo = TILE_GEMA;
    }

    if (modo_battle_royale) {
        if (rx < anillo_recorte || rx >= MAP_WIDTH - anillo_recorte || ry < anillo_recorte || ry >= MAP_HEIGHT - anillo_recorte) return;
    }

    mapa[ry][rx] = tipo;
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX + tipo), rx, ry);
}

void aplicar_recorte_battle_royale() {
    anillo_recorte += 3;
    en_preaviso_cierre = FALSE;
    PAL_setColor(1, color_muro_original); 
    
    for (u16 y = 0; y < MAP_HEIGHT; y++) {
        for (u16 x = 0; x < MAP_WIDTH; x++) {
            if (x < anillo_recorte || x >= MAP_WIDTH - anillo_recorte || y < anillo_recorte || y >= MAP_HEIGHT - anillo_recorte) {
                if (mapa[y][x] == TILE_POLLUELO) {
                    if (modo_jugadores == 1) {
                        verificar_fin_ronda_versus(FALSE);
                    } else {
                        victorias_cazador++;
                        if (continues_restantes > 0) { mostrar_pantalla_continue(); } else { mostrar_game_over(); }
                    }
                    return;
                }
                mapa[y][x] = TILE_MURO;
                VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX + TILE_MURO), x, y);
            }
        }
    }
    reproducir_sonido_item(180);
}

void leer_entrada() {
    u16 value1 = JOY_readJoypad(JOY_1);
    
    s16 flicky_offset_x = flicky.x & 7; 
    s16 flicky_offset_y = flicky.y & 7; 

    if ((flicky_offset_x <= 3) && (flicky_offset_y <= 3)) {
        s16 nx = 0, ny = 0;
        if (value1 & BUTTON_LEFT) nx = -1; 
        else if (value1 & BUTTON_RIGHT) nx = 1;
        else if (value1 & BUTTON_UP) ny = -1; 
        else if (value1 & BUTTON_DOWN) ny = 1;
        
        if (nx != 0 || ny != 0) {
            s16 tx = (flicky.x >> 3) + nx; 
            s16 ty = (flicky.y >> 3) + ny; 
            if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT && mapa[ty][tx] != TILE_MURO) {
                flicky.x = (flicky.x >> 3) << 3; 
                flicky.y = (flicky.y >> 3) << 3;
                flicky.x_vel = nx; flicky.y_vel = ny;
            }
        }
    }

    if (modo_jugadores == 1) {
        u16 value2 = JOY_readJoypad(JOY_2);
        s16 cazador_offset_x = cazador.x & 7;
        s16 cazador_offset_y = cazador.y & 7;

        if ((cazador_offset_x <= 3) && (cazador_offset_y <= 3)) {
            s16 c_nx = 0, c_ny = 0;
            if (value2 & BUTTON_LEFT) c_nx = -1; 
            else if (value2 & BUTTON_RIGHT) c_nx = 1;
            else if (value2 & BUTTON_UP) c_ny = -1; 
            else if (value2 & BUTTON_DOWN) c_ny = 1;
            
            if (c_nx != 0 || c_ny != 0) {
                s16 c_tx = (cazador.x >> 3) + c_nx; 
                s16 c_ty = (cazador.y >> 3) + c_ny; 
                if (c_tx >= 0 && c_tx < MAP_WIDTH && c_ty >= 0 && c_ty < MAP_HEIGHT && mapa[c_ty][c_tx] != TILE_MURO) {
                    cazador.x = (cazador.x >> 3) << 3; 
                    cazador.y = (cazador.y >> 3) << 3;
                    cazador.x_vel = c_nx; cazador.y_vel = c_ny;
                }
            }
        }
    }
}

void comprobar_recogida_objetos() {
    u16 cx = (flicky.x + 4) >> 3;
    u16 cy = (flicky.y + 4) >> 3;

    if (cx < MAP_WIDTH && cy < MAP_HEIGHT) {
        u8 tipo_objeto = mapa[cy][cx];

        if (tipo_objeto == TILE_POLLUELO && semillas_en_fila < max_polluelos_nivel) {
            mapa[cy][cx] = TILE_SUELO; 
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, 0), cx, cy); 
            sprites_seguidores[semillas_en_fila] = SPR_addSprite(&flicky_sprite, flicky.x - FLICKY_DRAW_OFFSET, flicky.y - FLICKY_DRAW_OFFSET, TILE_ATTR(PAL2, 0, FALSE, FALSE));
            semillas_en_fila++; 
            reproducir_sonido_waka();
        } 
        else if (tipo_objeto == TILE_GEMA) {
            mapa[cy][cx] = TILE_SUELO; 
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, 0), cx, cy);
            flicky.puntuacion += 250;
            reproducir_sonido_item(1400);
            efecto_shake(6, 1);
        } 
        else if (tipo_objeto == TILE_RELOJ) {
            mapa[cy][cx] = TILE_SUELO; 
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, 0), cx, cy);
            timer_cazador_lento = DURACION_PODER_FRAMES;
            cazador_en_furia = FALSE; 
            PAL_setColor(1, color_muro_original);
            reproducir_sonido_item(220);
        } 
        else if (tipo_objeto == TILE_RAYO) {
            mapa[cy][cx] = TILE_SUELO; 
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, 0, 0, 0, 0), cx, cy);
            timer_flicky_rapido = DURACION_PODER_FRAMES;
            reproducir_sonido_item(900);
            efecto_shake(8, 2);
        }

        if (modo_jugadores == 0) {
            if (flicky.puntuacion >= proximo_hito_continue) {
                continues_restantes++;
                proximo_hito_continue += 2500;
                reproducir_sonido_item(1800); 
                efecto_shake(10, 2);
            }
        }
    }
}

void actualizar_logica() {
    frame_counter++;

    efecto_shake_actualizar();
    actualizar_invulnerabilidad();  

    if (timer_cazador_lento > 0) timer_cazador_lento--;
    if (timer_flicky_rapido > 0) timer_flicky_rapido--;

    if (timer_spawn_powerup > 0) {
        timer_spawn_powerup--;
    } else {
        generar_powerup_aleatorio();
        timer_spawn_powerup = 240 + (random() % 240); 
    }

    if (modo_battle_royale && en_preaviso_cierre) {
        if ((frame_counter & 15) < 8) { 
            PAL_setColor(1, RGB24_TO_VDPCOLOR(0xFF0000)); 
        } else {
            PAL_setColor(1, color_muro_original); 
        }
    }

    if (cazador_en_furia) {
        if ((frame_counter & 8) == 0) {
            PAL_setColor(1, RGB24_TO_VDPCOLOR(0xFF0000));
        } else {
            PAL_setColor(1, color_muro_original);
        }
    }

    if (salida_abierta) {
        u16 safe_text_x = (anillo_recorte > 0) ? (anillo_recorte + 1) : 8;
        u16 safe_text_y = (anillo_recorte > 0) ? (anillo_recorte + 1) : 2;
        VDP_setTextPalette(PAL0); 
        if ((frame_counter & 31) < 15) VDP_drawText("¡VE A LA META Y ESCAPA!", safe_text_x, safe_text_y);
        else VDP_drawText("                       ", safe_text_x, safe_text_y);
    }

    static u8 frame_timer_60 = 0;
    frame_timer_60++;
    if (frame_timer_60 >= 60) {
        frame_timer_60 = 0;
        
        if (tiempo_restante_segundos > 0) {
            tiempo_restante_segundos--;
            
            s32 tiempo_inicial;
            if (modo_jugadores == 1) {
                tiempo_inicial = (versus_tiempo_idx == 0) ? 60 :
                                 (versus_tiempo_idx == 1) ? 90 :
                                 (versus_tiempo_idx == 2) ? 120 : 180;
            } else {
                tiempo_inicial = (d_sel == 0) ? 120 :
                                 (d_sel == 1) ? 180 : 300;
            }
            s32 segundos_jugados = tiempo_inicial - tiempo_restante_segundos;

            if (modo_jugadores == 0 && segundos_jugados >= 15) {
                if (!cazador_en_furia) {
                    if ((random() % 100) < 12) {
                        cazador_en_furia = TRUE;
                        timer_duracion_furia = 8; // Ampliado a 8 segundos
                        playTimeAlertSound(); 
                        efecto_shake(15, 2);
                    }
                } else {
                    timer_duracion_furia--;
                    if (timer_duracion_furia <= 0) {
                        cazador_en_furia = FALSE;
                        PAL_setColor(1, color_muro_original);
                    }
                }
            }

            if (modo_battle_royale) {
                if (tiempo_restante_segundos == 85 || tiempo_restante_segundos == 55 || tiempo_restante_segundos == 25) en_preaviso_cierre = TRUE;
                if (tiempo_restante_segundos == 80 || tiempo_restante_segundos == 50 || tiempo_restante_segundos == 20) {
                    aplicar_recorte_battle_royale();
                    if (estado_actual == ESTADO_MENU || estado_actual == ESTADO_GAMEOVER || estado_actual == ESTADO_CONTINUE || estado_actual == ESTADO_FINJUEGO) return;
                }
                if (en_preaviso_cierre) playTimeAlertSound(); 
            }

            if (tiempo_restante_segundos <= 10 && !en_preaviso_cierre) playTimeAlertSound();

            static u8 control_minuto = 0;
            control_minuto++;
            if (segundos_jugados > 0 && control_minuto >= 60) {
                control_minuto = 0;
                if (modo_jugadores == 0) {
                    multiplicador_velocidad_minuto++;
                }
            }

        } else {
            if (modo_jugadores == 1) {
                verificar_fin_ronda_versus(FALSE);
            } else {
                victorias_cazador++;
                if (continues_restantes > 0) { mostrar_pantalla_continue(); } else { mostrar_game_over(); }
            }
            return;
        }
    }

    if (!flicky_invulnerable && (tiempo_restante_segundos <= 15 || en_preaviso_cierre)) {
        if ((frame_counter & 8) == 0) {
            SPR_setVisibility(cazador.sprite, HIDDEN);
        } else {
            SPR_setVisibility(cazador.sprite, VISIBLE);
        }
    } else if (!flicky_invulnerable) {
        SPR_setVisibility(cazador.sprite, VISIBLE);
    }

    u8 pasos_flicky = (timer_flicky_rapido > 0) ? 2 : 1;
    for (u8 p = 0; p < pasos_flicky; p++) {
        if (flicky.x_vel == 0 && flicky.y_vel == 0) break;
        s16 next_x = flicky.x + flicky.x_vel;
        s16 next_y = flicky.y + flicky.y_vel;
        s16 col_x = (flicky.x_vel > 0) ? (next_x + 7) : next_x;
        s16 col_y = (flicky.y_vel > 0) ? (next_y + 7) : next_y;

        if (mapa[col_y >> 3][col_x >> 3] != TILE_MURO) {
            flicky.x = next_x;
            flicky.y = next_y;
        } else {
            flicky.x = (flicky.x >> 3) << 3;
            flicky.y = (flicky.y >> 3) << 3;
            flicky.x_vel = 0; flicky.y_vel = 0;
        }

        historial_x[indice_historial] = flicky.x;
        historial_y[indice_historial] = flicky.y;
        indice_historial = (indice_historial + 1) & 127; 
    }

    comprobar_recogida_objetos();

    u16 cx = flicky.x >> 3;
    u16 cy = flicky.y >> 3;

    if (cx >= NIDO_X && cx <= NIDO_X + NIDO_ANCHO - 1 && cy == NIDO_Y) {
        if (semillas_en_fila > 0) {
            u16 c = semillas_en_fila;
            for (u16 i = 0; i < c; i++) { 
                semillas_en_fila--; 
                if (sprites_seguidores[semillas_en_fila]) {
                    SPR_releaseSprite(sprites_seguidores[semillas_en_fila]);
                    sprites_seguidores[semillas_en_fila] = NULL;
                }
                total_salvados++; 
                reproducir_sonido_deposito(i); 
                for(s16 d=0; d<6; d++) SYS_doVBlankProcess(); 
            }
            flicky.puntuacion += (c * 100);
            efecto_shake(8, 1);
        }
        
        if (total_salvados >= max_polluelos_nivel) {
            salida_abierta = TRUE;
            efecto_shake(15, 2);
            reproducir_sonido_item(1500);
        }

        if (salida_abierta && semillas_en_fila == 0) {
            if (modo_jugadores == 1) {
                verificar_fin_ronda_versus(TRUE);
                return;
            } else {
                victorias_pajaro++;
                if (modo_juego_seleccionada == 0) { 
                    fase_actual++;
                    if (fase_actual > 5) {
                        fase_actual = 1;
                        mundo_actual++; 
                    }
                    salida_abierta = FALSE; 
                    finalizar_partida(); 
                    
                    if (mundo_actual > MUNDO_CELESTE) {
                        mostrar_enhorabuena();
                        return;
                    } else {
                        if (fase_actual == 1) {
                            estado_actual = ESTADO_INTRO;
                            musica_segun_estado(ESTADO_INTRO);
                            mostrar_intro_mundo_actual();
                        } else {
                            resetear_tiempo_nivel();
                            VDP_clearPlane(BG_A, TRUE);
                            inicializar_juego();
                        }
                        return;
                    }
                } else { 
                    estado_actual = ESTADO_VICTORIA;
                    VDP_setTextPalette(PAL0);
                    VDP_drawText("¡HAS ESCAPADO CON EXITO!", 8, 13);
                    PSG_setTone(2, 900);  PSG_setEnvelope(2, 2); waitMs(150);
                    PSG_setTone(2, 1200); waitMs(150);
                    PSG_setTone(2, 1500); waitMs(600);
                    PSG_setEnvelope(2, 15);
                    finalizar_partida();
                    estado_actual = ESTADO_MENU; 
                    mostrar_menu_opciones(); 
                    return;
                }
            }
        }
    }

    for (u16 i = 0; i < semillas_en_fila; i++) { 
        if (sprites_seguidores[i]) {
            s16 r = (indice_historial - 1 - ((i + 1) * 6) + MAX_HISTORIAL) & 127; 
            SPR_setPosition(sprites_seguidores[i], historial_x[r] - FLICKY_DRAW_OFFSET, historial_y[r] - FLICKY_DRAW_OFFSET); 
        }
    }

    actualizar_anim_direccion(&flicky.anim_actual, flicky.sprite, flicky.x_vel, flicky.y_vel, FALSE);
    
    static u8 anim_counter = 0;
    anim_counter++;
    if (anim_counter >= 6) { 
        anim_counter = 0;
        if (flicky.x_vel != 0 || flicky.y_vel != 0) SPR_nextFrame(flicky.sprite); else SPR_setFrame(flicky.sprite, 0); 
        if (cazador.x_vel != 0 || cazador.y_vel != 0) SPR_nextFrame(cazador.sprite); else SPR_setFrame(cazador.sprite, 0);
    }
    
    SPR_setPosition(flicky.sprite, flicky.x - FLICKY_DRAW_OFFSET, flicky.y - FLICKY_DRAW_OFFSET);
    mover_cazador();

    // ===== COLISIÓN CON CAZADOR (MECÁNICAS AJUSTADAS) =====
    if (abs(flicky.x - cazador.x) < 8 && abs(flicky.y - cazador.y) < 8) {
        if (flicky_invulnerable) {
            // Ignorar daño durante la inmunidad temporal
        }
        // NUEVO: Si está en MODO RABIA (cazador_en_furia), ignoramos la protección de polluelos y muere directamente
        else if (cazador_en_furia && modo_jugadores == 0) {
            efecto_shake(20, 3);
            reproducir_sonido_panico();
            victorias_cazador++;
            VDP_setTextPalette(PAL0);
            VDP_drawText("¡LA RABIA TE DESTRUYO!", 8, 13);
            PSG_setTone(2, 180); PSG_setEnvelope(2, 2);
            for (u16 f = 0; f < 90; f++) {
                if (f == 40) PSG_setTone(2, 100);
                if (f == 70) PSG_setEnvelope(2, 15);
                if (f < 30) efecto_shake_actualizar();
                SYS_doVBlankProcess();
            }
            if (continues_restantes > 0) { mostrar_pantalla_continue(); } else { mostrar_game_over(); }
            return;
        }
        // Modo normal con polluelos: salvaguarda por dispersión
        else if (semillas_en_fila > 0 && modo_jugadores == 0) {
            dispersar_polluelos();
            flicky_invulnerable = TRUE;
            timer_invulnerabilidad = TIEMPO_INVULNERABILIDAD_FRAMES;
            reproducir_sonido_panico();
            efecto_shake(20, 3);
        }
        // Modo versus: muerte directa
        else if (modo_jugadores == 1) {
            efecto_shake(20, 3);
            reproducir_sonido_panico();
            verificar_fin_ronda_versus(FALSE);
            return;
        }
        // Modo historia sin polluelos: muerte
        else {
            efecto_shake(20, 3);
            reproducir_sonido_panico();
            victorias_cazador++;
            VDP_setTextPalette(PAL0);
            VDP_drawText("EL CAZADOR TE ATRAPO!", 8, 13);
            PSG_setTone(2, 180); PSG_setEnvelope(2, 2);
            for (u16 f = 0; f < 90; f++) {
                if (f == 40) PSG_setTone(2, 100);
                if (f == 70) PSG_setEnvelope(2, 15);
                if (f < 30) efecto_shake_actualizar();
                SYS_doVBlankProcess();
            }
            if (continues_restantes > 0) { mostrar_pantalla_continue(); } else { mostrar_game_over(); }
            return;
        }
    }

    char m[55];
    u16 min = tiempo_restante_segundos / 60;
    u16 seg = tiempo_restante_segundos - (min * 60); 
    
    VDP_setTextPalette(PAL0);
    if (modo_jugadores == 1) {
        sprintf(m, "VERSUS | P1 SCORE:%d - P2 SCORE:%d | T:%01d:%02d", (u16)victorias_pajaro, (u16)victorias_cazador, min, seg);
    } else {
        if (modo_juego_seleccionada == 0) {
            sprintf(m, "W%d-L%d PTS:%05ld | T:%01d:%02d | H:%d/%d | C:%d", mundo_actual+1, fase_actual, flicky.puntuacion, min, seg, total_salvados, max_polluelos_nivel, continues_restantes);
        } else {
            sprintf(m, "LIBRE  PTS:%05ld | T:%01d:%02d | H:%d/%d | C:%d", flicky.puntuacion, min, seg, total_salvados, max_polluelos_nivel, continues_restantes);
        }
    }
    VDP_drawText(m, 0, 27); 
}

void mover_cazador() {
    cazador.velocidad_timer++;
    u8 pasos = 1;

    if (timer_cazador_lento > 0) {
        static u8 lento_c = 0;
        lento_c++;
        if (lento_c >= 3) { lento_c = 0; } else { return; }
    } else {
        u16 velocidad_actual = 1;
        
        if (cazador_en_furia) {
            velocidad_actual = (frame_counter & 1) ? 2 : 1;
        } else {
            if (semillas_en_fila >= 6) velocidad_actual++;
        }
        
        if (modo_jugadores == 0) {
            if (velocidad_actual == 1 && (cazador.velocidad_timer & 1)) return;
            pasos = velocidad_actual;
        } else {
            pasos = 1; 
        }
    }

    for (u8 p = 0; p < pasos; p++) {
        if (modo_jugadores == 0) {
            if (((cazador.x & 7) == 0) && ((cazador.y & 7) == 0)) {
                s16 tx = cazador.x >> 3, ty = cazador.y >> 3;
                bool lizq = (tx > 0) && (mapa[ty][tx - 1] != TILE_MURO) && (cazador.x_vel != 1);
                bool lder = (tx < MAP_WIDTH - 1) && (mapa[ty][tx + 1] != TILE_MURO) && (cazador.x_vel != -1);
                bool larr = (ty > 0) && (mapa[ty - 1][tx] != TILE_MURO) && (cazador.y_vel != 1);
                bool laba = (ty < MAP_HEIGHT - 1) && (mapa[ty + 1][tx] != TILE_MURO) && (cazador.y_vel != -1);
                s16 ptx = tx + cazador.x_vel, pty = ty + cazador.y_vel;
                bool blq = (ptx < 0 || ptx >= MAP_WIDTH || pty < 0 || pty >= MAP_HEIGHT || mapa[pty][ptx] == TILE_MURO);

                if (semillas_en_fila > 0 && ((random() & 63) < 45)) { 
                    s16 ox = 0, oy = 0;
                    if ((flicky.x >> 3) < tx && lizq) { ox = -1; oy = 0; }
                    else if ((flicky.x >> 3) > tx && lder) { ox = 1; oy = 0; }
                    else if ((flicky.y >> 3) < ty && larr) { ox = 0; oy = -1; }
                    else if ((flicky.y >> 3) > ty && laba) { ox = 0; oy = 1; }
                    if (ox != 0 || oy != 0) { cazador.x_vel = ox; cazador.y_vel = oy; blq = FALSE; }
                }
                if (blq || ((random() & 31) < 3)) { 
                    s16 ox[4], oy[4], n = 0;
                    if (lizq) { ox[n] = -1; oy[n] = 0; n++; } if (lder) { ox[n] = 1; oy[n] = 0; n++; }
                    if (larr) { ox[n] = 0; oy[n] = -1; n++; } if (laba) { ox[n] = 0; oy[n] = 1; n++; }
                    if (n > 0) { u16 e = random() & 3; while(e >= n) e -= n; cazador.x_vel = ox[e]; cazador.y_vel = oy[e]; } else { cazador.x_vel = -cazador.x_vel; cazador.y_vel = -cazador.y_vel; }
                }
            }
        }

        s16 px = cazador.x + cazador.x_vel, py = cazador.y + cazador.y_vel;
        s16 c_col_x = (cazador.x_vel > 0) ? (px + 7) : px;
        s16 c_col_y = (cazador.y_vel > 0) ? (py + 7) : py;

        if (mapa[c_col_y >> 3][c_col_x >> 3] != TILE_MURO) { 
            cazador.x = px; cazador.y = py; 
        } else { 
            cazador.x = (cazador.x >> 3) << 3; cazador.y = (cazador.y >> 3) << 3; 
            cazador.x_vel = 0; cazador.y_vel = 0; 
        }
    }

    bool cazador_mira_izquierda = (mundo_actual == MUNDO_VOLCAN || mundo_actual == MUNDO_MARINO || mundo_actual == MUNDO_CELESTE);
    actualizar_anim_direccion(&cazador.anim_actual, cazador.sprite, cazador.x_vel, cazador.y_vel, cazador_mira_izquierda);
    
    SPR_setPosition(cazador.sprite, cazador.x - CAZADOR_DRAW_OFFSET, cazador.y - CAZADOR_DRAW_OFFSET);
}

void actualizar_anim_direccion(u8* a, Sprite* s, s16 xv, s16 yv, bool mirar_izquierda_por_defecto) {
    u8 o = *a; 
    
    if (yv > 0) o = ANIM_ABAJO; 
    else if (yv < 0) o = ANIM_ARRIBA; 
    else if (xv != 0) o = ANIM_LADO;
    
    if (o != *a) { 
        SPR_setAnim(s, o); 
        *a = o; 
    }
    
    if (xv != 0) {
        if (mirar_izquierda_por_defecto) {
            if (xv > 0) SPR_setHFlip(s, TRUE); 
            else SPR_setHFlip(s, FALSE);
        } else {
            if (xv < 0) SPR_setHFlip(s, TRUE); 
            else SPR_setHFlip(s, FALSE);
        }
    }
}