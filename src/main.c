#include <genesis.h>
#include "resources.h"
#include "hud_chars.h"

#define MAP_WIDTH   20
#define MAP_HEIGHT  13
// --- REJILLA DE 16x16 (rediseño 16 bits) ---
#define CELDA        16
#define CELDA_SHIFT  4
#define CELDA_MASK   15
#define PLAY_Y       16   // el marcador ocupa los 16 px superiores
#define PLAY_TY      2    // fila de tiles donde empieza el laberinto

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

#define FLICKY_DRAW_OFFSET   0
#define CAZADOR_DRAW_OFFSET  4
#define CAZADOR_DRAW_OFFSET_Y 8

#define ANIM_ABAJO  0
#define ANIM_ARRIBA 1
#define ANIM_LADO   2
#define ANIM_HERIDO 3
#define ANIM_FELIZ  4

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

#define MAX_HISTORIAL  256
#define HIST_MASK      255
#define SEPARACION_POLLUELOS 12
#define DURACION_PODER_FRAMES  300 

// --- DURACIONES DE ESTADO TRAS DISPERSIÓN/RABIA ---
#define TIEMPO_INVULNERABILIDAD_FRAMES 150  // ~2.5 segundos a 60fps
#define RADIO_DISPERSION 3  // Radio en celdas para dispersar polluelos

// (la música y los efectos van por el driver XGM2: ver "SONIDO" más abajo)


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

#define NIDO_X 9
#define NIDO_Y 6
#define NIDO_ANCHO 2

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
        VDP_setHorizontalScroll(BG_B, shake_x);
        shake_timer--;
    } else if (shake_x != 0) {
        shake_x = 0;
        VDP_setHorizontalScroll(BG_A, 0);
        VDP_setHorizontalScroll(BG_B, 0);
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

// ===== SONIDO (fase 5): música FM y efectos PCM con el driver XGM2 de SGDK =====
// Las pistas salen de tools/sound (MML original compilado con ctrmml) en el orden de resources.res
enum { PISTA_TITULO, PISTA_MENU, PISTA_BOSQUE, PISTA_VOLCAN, PISTA_MARINO, PISTA_CELESTE,
       PISTA_INTRO, PISTA_SUPERADA, PISTA_GAMEOVER, PISTA_VICTORIA, PISTA_CONTINUAR };
static s16 pista_actual = -1;

static void musica_pista(u16 pista) {
    if (pista_actual == (s16)pista && XGM2_isPlaying()) return;
    pista_actual = pista;
    XGM2_playTrack(pista);
}

void musica_detener() { XGM2_stop(); pista_actual = -1; }
void musica_pausar()  { XGM2_pause(); }
void musica_reanudar(){ XGM2_resume(); }
void musica_actualizar() {}

void musica_segun_estado(u8 estado) {
    switch (estado) {
        case ESTADO_TITULO:      musica_pista(PISTA_TITULO); break;
        case ESTADO_MENU:
        case ESTADO_MENU_VERSUS:
        case ESTADO_COMO_JUGAR:  musica_pista(PISTA_MENU); break;
        case ESTADO_INTRO:       pista_actual = -1; musica_pista(PISTA_INTRO); break;
        case ESTADO_JUEGO:
            musica_pista(mundo_actual == MUNDO_VOLCAN ? PISTA_VOLCAN : mundo_actual == MUNDO_MARINO ? PISTA_MARINO :
                         mundo_actual == MUNDO_CELESTE ? PISTA_CELESTE : PISTA_BOSQUE);
            break;
        case ESTADO_GAMEOVER:    pista_actual = -1; musica_pista(PISTA_GAMEOVER); break;
        case ESTADO_FINJUEGO:    musica_pista(PISTA_VICTORIA); break;
        case ESTADO_CONTINUE:    musica_pista(PISTA_CONTINUAR); break;
    }
}

// Efectos: canal PCM 2 (el 1 lo usa la batería de la música); los avisos importantes van con prioridad
#define SFX(nombre, canal, prio) XGM2_playPCMEx(nombre, sizeof(nombre), canal, prio, FALSE, FALSE)
void playMenuClickSound()            { SFX(sfx_click, SOUND_PCM_CH2, 0); }
void playTimeAlertSound()            { SFX(sfx_alerta, SOUND_PCM_CH3, 0); }
void reproducir_sonido_waka()        { SFX(sfx_pio, SOUND_PCM_CH2, 0); }
void reproducir_sonido_deposito(u16 n) {
    switch (n % 5) {
        case 0: SFX(sfx_nido0, SOUND_PCM_CH2, 1); break;
        case 1: SFX(sfx_nido1, SOUND_PCM_CH2, 1); break;
        case 2: SFX(sfx_nido2, SOUND_PCM_CH2, 1); break;
        case 3: SFX(sfx_nido3, SOUND_PCM_CH2, 1); break;
        default: SFX(sfx_nido4, SOUND_PCM_CH2, 1); break;
    }
}
// Se mantiene la firma antigua (el "tono" identifica el efecto)
void reproducir_sonido_item(u16 tono) {
    if (tono == 1400)      SFX(sfx_gema, SOUND_PCM_CH2, 0);
    else if (tono == 220)  SFX(sfx_reloj, SOUND_PCM_CH2, 0);
    else if (tono == 900)  SFX(sfx_rayo, SOUND_PCM_CH2, 0);
    else if (tono == 1500) SFX(sfx_meta, SOUND_PCM_CH3, 1);
    else if (tono == 180)  SFX(sfx_cierre, SOUND_PCM_CH3, 1);
    else                   SFX(sfx_click, SOUND_PCM_CH2, 0);
}
void reproducir_sonido_panico()      { SFX(sfx_panico, SOUND_PCM_CH3, 1); }
void sonido_atrapado()               { SFX(sfx_atrapado, SOUND_PCM_CH3, 1); }
void sonido_rabia()                  { SFX(sfx_rabia, SOUND_PCM_CH3, 1); }
void procesar_silencio_psg() {}

// ===== DECLARACIONES DE FUNCIONES =====
bool en_zona_nido(s16 tx, s16 ty);
void dibujar_celda(s16 cx, s16 cy);
void dibujar_mapa_completo();
void cargar_graficos_mundo();
void muros_alerta(bool activa);
void hud_forzar_redibujo();
void hud_mensaje(const char* txt);
void hud_actualizar();
void mostrar_pantalla_titulo();
void mostrar_menu_opciones();
void mostrar_pantalla_como_jugar();
void mostrar_menu_versus();
void mostrar_intro_mundo_actual();
void inicializar_juego();
void resetear_tiempo_nivel();
void leer_entrada();
void actualizar_logica();
void efecto_chispa(s16 x, s16 y);
void pantalla_sprites_reset();
Sprite* pantalla_sprite(const SpriteDefinition* def, s16 x, s16 y, u16 pal, u16 anim, s16 vx, bool flip);
extern bool titulo_parpadeo;
void pantalla_actualizar();
void pantalla_imagen(const Image* img);
void pantalla_olvidar_imagen();
void texto_centrado(const char* txt, u16 y);
void efectos_actualizar();
void efectos_reset();
void animar_polluelos_mapa();
void cazador_tinte_actualizar();
void banner(const char* txt);
void secuencia_heroe(u8 anim, u16 frames, bool salta);
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

// =====================================================================
// RENDERIZADO DEL LABERINTO, MARCADOR Y PALETAS (rediseño 16 bits)
// Cada celda lógica de 16x16 = 4 tiles VDP consecutivos en el tileset.
// =====================================================================
static u16 base_maze, base_floor, base_items, base_hud;
static const Palette* pal_maze_actual = &pal_bosque_maze;

#define META_NIDO     16   // metatiles 16-17 del tileset del laberinto
#define TILE_HUD_FONDO 72  // tile suelto: relleno del marcador
#define TILE_HUD_BORDE 73  // tile suelto: borde inferior del marcador

static void poner_meta(VDPPlane plano, u16 pal, u16 tile_base, s16 cx, s16 cy) {
    u16 x = cx << 1, y = (cy << 1) + PLAY_TY;
    VDP_setTileMapXY(plano, TILE_ATTR_FULL(pal, 0, 0, 0, tile_base),     x,     y);
    VDP_setTileMapXY(plano, TILE_ATTR_FULL(pal, 0, 0, 0, tile_base + 1), x + 1, y);
    VDP_setTileMapXY(plano, TILE_ATTR_FULL(pal, 0, 0, 0, tile_base + 2), x,     y + 1);
    VDP_setTileMapXY(plano, TILE_ATTR_FULL(pal, 0, 0, 0, tile_base + 3), x + 1, y + 1);
}

static bool hay_muro(s16 x, s16 y) {
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return TRUE;   // fuera = muro (borde continuo)
    return mapa[y][x] == TILE_MURO;
}

static bool es_celda_nido(s16 x, s16 y) {
    return (y == NIDO_Y && x >= NIDO_X && x < NIDO_X + NIDO_ANCHO);
}

void dibujar_celda(s16 cx, s16 cy) {
    if (cx < 0 || cy < 0 || cx >= MAP_WIDTH || cy >= MAP_HEIGHT) return;
    u8 t = mapa[cy][cx];
    // Plano A: muros (autotile por vecinos), nido u objetos
    if (t == TILE_MURO) {
        u16 mask = (hay_muro(cx, cy - 1) ? 1 : 0) | (hay_muro(cx + 1, cy) ? 2 : 0) |
                   (hay_muro(cx, cy + 1) ? 4 : 0) | (hay_muro(cx - 1, cy) ? 8 : 0);
        poner_meta(BG_A, PAL0, base_maze + mask * 4, cx, cy);
    } else if (es_celda_nido(cx, cy)) {
        poner_meta(BG_A, PAL0, base_maze + (META_NIDO + cx - NIDO_X) * 4, cx, cy);
    } else if (t == TILE_POLLUELO || t == TILE_RELOJ || t == TILE_RAYO || t == TILE_GEMA) {
        u16 item = (t == TILE_POLLUELO) ? 0 : (t == TILE_RELOJ) ? 1 : (t == TILE_RAYO) ? 2 : 3;
        poner_meta(BG_A, PAL2, base_items + item * 4, cx, cy);
    } else {
        poner_meta(BG_A, PAL0, 0, cx, cy);   // tile 0 = transparente
        VDP_setTileMapXY(BG_A, 0, cx << 1, (cy << 1) + PLAY_TY);
        VDP_setTileMapXY(BG_A, 0, (cx << 1) + 1, (cy << 1) + PLAY_TY);
        VDP_setTileMapXY(BG_A, 0, cx << 1, (cy << 1) + PLAY_TY + 1);
        VDP_setTileMapXY(BG_A, 0, (cx << 1) + 1, (cy << 1) + PLAY_TY + 1);
    }
    // Plano B: suelo con sombra proyectada por los muros de arriba/izquierda
    u16 sombra = (hay_muro(cx, cy - 1) ? 1 : 0) | (hay_muro(cx - 1, cy) ? 2 : 0) | (hay_muro(cx - 1, cy - 1) ? 4 : 0);
    u16 textura = ((cx * 7) + (cy * 13)) & 3;
    poner_meta(BG_B, PAL1, base_floor + (textura * 8 + sombra) * 4, cx, cy);
}

void dibujar_mapa_completo() {
    for (s16 y = 0; y < MAP_HEIGHT; y++)
        for (s16 x = 0; x < MAP_WIDTH; x++) dibujar_celda(x, y);
}

void cargar_graficos_mundo() {
    // Cada mundo tiene su tileset de muros/nido/marcador y su suelo (mismo nº de tiles en todos)
    const TileSet* ts_maze = &ts_bosque_maze;   const TileSet* ts_floor = &ts_bosque_floor;
    const Palette* p_maze  = &pal_bosque_maze;  const Palette* p_floor  = &pal_bosque_floor;
    if (mundo_actual == MUNDO_VOLCAN)       { ts_maze = &ts_volcan_maze;  ts_floor = &ts_volcan_floor;  p_maze = &pal_volcan_maze;  p_floor = &pal_volcan_floor; }
    else if (mundo_actual == MUNDO_MARINO)  { ts_maze = &ts_marino_maze;  ts_floor = &ts_marino_floor;  p_maze = &pal_marino_maze;  p_floor = &pal_marino_floor; }
    else if (mundo_actual == MUNDO_CELESTE) { ts_maze = &ts_celeste_maze; ts_floor = &ts_celeste_floor; p_maze = &pal_celeste_maze; p_floor = &pal_celeste_floor; }

    pal_maze_actual = p_maze;
    base_maze  = TILE_USER_INDEX;
    base_floor = base_maze + ts_maze->numTile;
    base_items = base_floor + ts_floor->numTile;
    base_hud   = base_items + ts_items.numTile;
    VDP_loadTileSet(ts_maze, base_maze, DMA);
    VDP_loadTileSet(ts_floor, base_floor, DMA);
    VDP_loadTileSet(&ts_items, base_items, DMA);
    VDP_loadTileSet(&ts_hud, base_hud, DMA);
    VDP_loadFont(&ts_font, DMA);
    PAL_setPalette(PAL0, p_maze->data, CPU);
    PAL_setPalette(PAL1, p_floor->data, CPU);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    color_muro_original = p_maze->data[1];
    VDP_setTextPalette(PAL0);
}

// Muros en alerta (rabia / cierre): mezcla los 5 tonos del muro con rojo (nivel 0..8)
static const u16 rojos_alerta[5] = { RGB24_TO_VDPCOLOR(0xD83020), RGB24_TO_VDPCOLOR(0x902018), RGB24_TO_VDPCOLOR(0xF06040),
                                     RGB24_TO_VDPCOLOR(0xFFA080), RGB24_TO_VDPCOLOR(0x601010) };
void muros_mezcla(u16 nivel) {
    static u16 nivel_actual = 0xFFFF;
    if (nivel == nivel_actual) return;
    nivel_actual = nivel;
    for (u16 i = 0; i < 5; i++) {
        u16 a = pal_maze_actual->data[1 + i], b = rojos_alerta[i], c = 0;
        for (u16 sh = 1; sh <= 9; sh += 4) {
            s16 ca = (a >> sh) & 7, cb = (b >> sh) & 7;
            c |= ((u16)(ca + ((cb - ca) * (s16)nivel) / 8) & 7) << sh;
        }
        PAL_setColor(1 + i, c);
    }
}
void muros_alerta(bool activa) { muros_mezcla(activa ? 8 : 0); }

// ---------------- MARCADOR (fuente 8x16 con fondo integrado, en el plano B) ----------------
static u32 hud_cache_puntos = 0xFFFFFFFF;
static u16 hud_cache_tiempo = 0xFFFF, hud_cache_huevos = 0xFFFF, hud_cache_vidas = 0xFFFF;
static u8  hud_cache_parpadeo = 0xFF;

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

void hud_forzar_redibujo() {
    hud_cache_puntos = 0xFFFFFFFF; hud_cache_tiempo = 0xFFFF; hud_cache_huevos = 0xFFFF;
    hud_cache_vidas = 0xFFFF; hud_cache_parpadeo = 0xFF;
    hud_texto("                                        ", 0);
    for (u16 x = 0; x < 40; x++) { VDP_setTileMapXY(BG_A, 0, x, 0); VDP_setTileMapXY(BG_A, 0, x, 1); }
}

void hud_mensaje(const char* txt) {
    hud_texto("                                        ", 0);
    for (u16 x = 0; x < 40; x++) { VDP_setTileMapXY(BG_A, 0, x, 0); VDP_setTileMapXY(BG_A, 0, x, 1); }
    u16 len = strlen(txt);
    hud_texto(txt, (40 - len) / 2);
}

void hud_actualizar() {
    char buf[16];
    u16 min = tiempo_restante_segundos / 60;
    u16 seg = tiempo_restante_segundos - (min * 60);
    bool redibujo_total = (hud_cache_tiempo == 0xFFFF);

    if (modo_jugadores == 1) {
        if (redibujo_total) { hud_texto("P1", 1); hud_texto("P2", 9); hud_texto("TIME", 26); }
        if (hud_cache_puntos != (u32)(victorias_pajaro * 100 + victorias_cazador)) {
            hud_cache_puntos = victorias_pajaro * 100 + victorias_cazador;
            sprintf(buf, "%d", (u16)victorias_pajaro);  hud_texto(buf, 4);
            sprintf(buf, "%d", (u16)victorias_cazador); hud_texto(buf, 12);
        }
    } else {
        if (redibujo_total) {
            if (modo_juego_seleccionada == 0) { sprintf(buf, "%d-%d", mundo_actual + 1, fase_actual); hud_texto(buf, 1); }
            else hud_texto("LIBRE", 1);
            hud_texto("SCORE", 7); hud_texto("TIME", 20);
            hud_icono(0, 29);   // polluelo
            hud_icono(4, 36);   // cabeza del protagonista
        }
        if (hud_cache_puntos != flicky.puntuacion) {
            // el marcador "cuenta" hasta la puntuación real
            if (hud_cache_puntos > flicky.puntuacion) hud_cache_puntos = flicky.puntuacion;
            else {
                u32 dif = flicky.puntuacion - hud_cache_puntos;
                hud_cache_puntos += (dif > 400) ? 50 : ((dif > 40) ? 10 : dif);
            }
            sprintf(buf, "%06ld", hud_cache_puntos); hud_texto(buf, 13);
        }
        u8 parpadeo = salida_abierta ? ((frame_counter >> 4) & 1) : 0;
        u16 huevos = total_salvados * 100 + max_polluelos_nivel;
        if (hud_cache_huevos != huevos || hud_cache_parpadeo != parpadeo) {
            hud_cache_huevos = huevos; hud_cache_parpadeo = parpadeo;
            if (salida_abierta && parpadeo) hud_texto("META!", 31);
            else { sprintf(buf, "%02d/%02d", total_salvados, max_polluelos_nivel); hud_texto(buf, 31); }
        }
        if (hud_cache_vidas != (u16)continues_restantes) {
            hud_cache_vidas = continues_restantes;
            sprintf(buf, "x%d", continues_restantes); hud_texto(buf, 38);
        }
    }
    // con poco tiempo el reloj parpadea
    u16 clave_tiempo = tiempo_restante_segundos;
    bool oculto = (tiempo_restante_segundos <= 15) && ((frame_counter & 16) != 0);
    if (oculto) clave_tiempo |= 0x8000;
    if (hud_cache_tiempo != clave_tiempo) {
        hud_cache_tiempo = clave_tiempo;
        if (oculto) strcpy(buf, "    ");
        else sprintf(buf, "%d:%02d", min, seg);
        hud_texto(buf, (modo_jugadores == 1) ? 31 : 25);
    }
}

void dispersar_polluelos() {
    for (u16 i = 0; i < semillas_en_fila; i++) {
        if (sprites_seguidores[i]) {
            SPR_releaseSprite(sprites_seguidores[i]);
            sprites_seguidores[i] = NULL;
        }
    }
    
    u16 cx = (flicky.x + 8) >> CELDA_SHIFT;
    u16 cy = (flicky.y + 8) >> CELDA_SHIFT;
    u16 dispersados = 0;
    u16 intentos = 0;
    
    while (dispersados < semillas_en_fila && intentos < 300) {
        intentos++;
        s16 rx = (s16)cx + (random() % (RADIO_DISPERSION * 2 + 1)) - RADIO_DISPERSION;
        s16 ry = (s16)cy + (random() % (RADIO_DISPERSION * 2 + 1)) - RADIO_DISPERSION;
        
        if (rx >= 1 && rx < MAP_WIDTH - 1 && ry >= 1 && ry < MAP_HEIGHT - 1) {
            if (mapa[ry][rx] == TILE_SUELO && !en_zona_nido(rx, ry)) {
                mapa[ry][rx] = TILE_POLLUELO;
                dibujar_celda(rx, ry);
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

    // driver de sonido XGM2: todas las canciones en un único bloque (comparten muestras PCM)
    XGM2_loadDriver(TRUE);
    XGM2_load(musica);

    JOY_init();

    VDP_loadFont(&ts_font, DMA);
    PAL_setPalette(PAL0, pal_bosque_maze.data, CPU);
    PAL_setPalette(PAL1, pal_bosque_floor.data, CPU);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    PAL_setPalette(PAL3, hunter_sprite.palette->data, CPU);

    color_muro_original = pal_bosque_maze.data[1];

    estado_actual = ESTADO_TITULO;
    mostrar_pantalla_titulo();

    while(1) {
        u16 joy = JOY_readJoypad(JOY_1);
        static bool boton_pulsado = FALSE;


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
                        continue;
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
                        continue;
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
                    hud_mensaje("PAUSA");
                    musica_pausar();
                } else {
                    hud_forzar_redibujo();
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

        if (estado_actual != ESTADO_JUEGO) pantalla_actualizar();
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
    pantalla_imagen(&bg_title);
    VDP_loadFont(&ts_font, DMA);
    VDP_setTextPalette(PAL0);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    PAL_setPalette(PAL3, hunter_sprite.palette->data, CPU);
    // desfile: el pájaro con sus polluelos huyendo del cazador
    pantalla_sprite(&hero_sprite, -24, 160, PAL2, ANIM_LADO, 1, FALSE);
    for (u16 i = 0; i < 3; i++) pantalla_sprite(&chick_sprite, -48 - i * 18, 160, PAL2, 0, 1, FALSE);
    pantalla_sprite(&hunter_sprite, -150, 152, PAL3, ANIM_LADO, 1, FALSE);
    texto_centrado("(C) 2026 RETROPLAYING BCN", 26);
    musica_segun_estado(ESTADO_TITULO);
    titulo_parpadeo = TRUE;
}

// Colores de texto de los menús: PAL0 blanco (seleccionado), PAL1 dorado (títulos), PAL3 gris (resto).
// El índice 13 de cada paleta es el color del panel: la fuente opaca se funde con él.
static void menu_paletas(const Image* img) {
    u16 panel = img->palette->data[13];
    PAL_setColor(16 + 13, panel); PAL_setColor(16 + 14, 0); PAL_setColor(16 + 15, RGB24_TO_VDPCOLOR(0xFFD860));
    PAL_setColor(48 + 13, panel); PAL_setColor(48 + 14, 0); PAL_setColor(48 + 15, RGB24_TO_VDPCOLOR(0x8890A8));
}
static void menu_texto(const char* txt, u16 x, u16 y, u16 pal) {
    VDP_setTextPalette(pal);
    VDP_drawText(txt, x, y);
    VDP_setTextPalette(PAL0);
}
static void menu_opcion(const char* txt, u16 x, u16 y, bool sel) {
    menu_texto(sel ? ">" : " ", x, y, PAL0);
    menu_texto(txt, x + 2, y, sel ? PAL0 : PAL3);
}

void mostrar_menu_opciones() {
    char buf[40];
    pantalla_imagen(&bg_opciones);
    VDP_loadFont(&ts_font_menu, DMA);          // fuente con fondo del color del panel
    menu_paletas(&bg_opciones);
    musica_segun_estado(ESTADO_MENU);

    sprintf(buf, "PARTIDAS %-5ld   RECORD %06ld", total_partidas, puntuacion_maxima);
    menu_texto(buf, 3, 6, PAL3);
    sprintf(buf, "VICTORIAS  PAJARO %ld  CAZADOR %ld", victorias_pajaro, victorias_cazador);
    menu_texto(buf, 3, 7, PAL3);

    menu_texto("JUGADORES", 3, 9, PAL1);           menu_texto("IZQ/DER", 30, 9, PAL3);
    menu_opcion("1 JUGADOR CONTRA LA CPU", 4, 10, modo_jugadores == 0);
    menu_opcion("2 JUGADORES", 4, 11, modo_jugadores == 1);

    menu_texto("MODO", 3, 13, PAL1);                menu_texto("A / B", 32, 13, PAL3);
    menu_opcion("HISTORIA  4 MUNDOS x 5 FASES", 4, 14, modo_juego_seleccionada == 0);
    menu_opcion("LIBRE     UN MAPA AL AZAR", 4, 15, modo_juego_seleccionada == 1);

    menu_texto("BATTLE ROYALE", 3, 17, PAL1);       menu_texto("C", 36, 17, PAL3);
    menu_opcion(modo_battle_royale ? "ACTIVADO" : "DESACTIVADO", 4, 18, TRUE);

    menu_texto("DIFICULTAD", 3, 20, PAL1);          menu_texto("ARR/ABA", 30, 20, PAL3);
    menu_opcion("FACIL    2 MIN   5 POLLUELOS", 4, 21, d_sel == 0);
    menu_opcion("MEDIO    3 MIN  10 POLLUELOS", 4, 22, d_sel == 1);
    menu_opcion("DIFICIL  5 MIN  15 POLLUELOS", 4, 23, d_sel == 2);

    menu_texto("PULSA START PARA JUGAR", 9, 25, PAL1);
}

static void icono_item(u16 base, u16 item, u16 col, u16 fila) {
    u16 t = base + item * 4;
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL2, 1, 0, 0, t),     col,     fila);
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL2, 1, 0, 0, t + 1), col + 1, fila);
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL2, 1, 0, 0, t + 2), col,     fila + 1);
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL2, 1, 0, 0, t + 3), col + 1, fila + 1);
}

void mostrar_pantalla_como_jugar() {
    pantalla_imagen(&bg_como_jugar);
    VDP_loadFont(&ts_font_menu, DMA);
    menu_paletas(&bg_como_jugar);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    // iconos de los objetos (mismos tiles que en el juego), justo detrás de la imagen
    u16 base = TILE_USER_INDEX + bg_como_jugar.tileset->numTile;
    VDP_loadTileSet(&ts_items, base, DMA);

    icono_item(base, 0, 2, 6);
    menu_texto("RECOGE LOS POLLUELOS Y LLEVALOS", 5, 6, PAL0);
    menu_texto("AL NIDO DEL CENTRO DEL LABERINTO.", 5, 7, PAL0);
    PAL_setPalette(PAL3, hunter_sprite.palette->data, CPU);
    pantalla_sprite(&hunter_sprite, 12, 70, PAL3, ANIM_ABAJO, 0, FALSE);
    menu_texto("SI EL CAZADOR TE PILLA, TUS", 5, 9, PAL0);
    menu_texto("POLLUELOS SE ESCAPAN. SIN NINGUNO", 5, 10, PAL0);
    menu_texto("DETRAS, PIERDES LA PARTIDA.", 5, 11, PAL0);
    icono_item(base, 1, 2, 13);
    menu_texto("RELOJ: FRENA AL CAZADOR.", 5, 13, PAL1);
    icono_item(base, 2, 2, 15);
    menu_texto("RAYO: CORRES EL DOBLE.", 5, 15, PAL1);
    icono_item(base, 3, 2, 17);
    menu_texto("GEMA: 250 PUNTOS EXTRA.", 5, 17, PAL1);
    menu_texto("SI LOS MUROS SE PONEN ROJOS, EL", 5, 19, PAL0);
    menu_texto("CAZADOR ENTRA EN RABIA: HUYE!", 5, 20, PAL0);
    menu_texto("CADA 2500 PUNTOS GANAS UN", 5, 22, PAL0);
    menu_texto("CONTINUE EXTRA.", 5, 23, PAL0);
    menu_texto("PULSA START", 14, 25, PAL1);
}

// ===== MENÚ VERSUS ACTUALIZADO CON SELECCIÓN DE ESCENARIO Y "VOLVER AL MENÚ" =====
void mostrar_menu_versus() {
    char buf[40];
    pantalla_imagen(&bg_versus);
    VDP_loadFont(&ts_font_menu, DMA);
    menu_paletas(&bg_versus);

    menu_opcion(modo_battle_royale ? "BATTLE ROYALE      SI" : "BATTLE ROYALE      NO", 5, 7, versus_sel == 0);

    u16 segs = (versus_tiempo_idx == 0) ? 60 : ((versus_tiempo_idx == 1) ? 90 : ((versus_tiempo_idx == 2) ? 120 : 180));
    sprintf(buf, "TIEMPO DE RONDA    %d SEG", segs);
    menu_opcion(buf, 5, 10, versus_sel == 1);

    sprintf(buf, "AL MEJOR DE        %d", versus_victorias_req);
    menu_opcion(buf, 5, 13, versus_sel == 2);

    const char* nombre_mundo;
    switch (versus_mundo_idx) {
        case 0: nombre_mundo = "BOSQUE"; break;
        case 1: nombre_mundo = "VOLCAN"; break;
        case 2: nombre_mundo = "MAR"; break;
        case 3: nombre_mundo = "CIELO"; break;
        default: nombre_mundo = "AL AZAR"; break;
    }
    sprintf(buf, "ESCENARIO          %s", nombre_mundo);
    menu_opcion(buf, 5, 16, versus_sel == 3);

    menu_opcion("VOLVER AL MENU", 5, 19, versus_sel == 4);

    menu_texto("MANDO 1: PAJARO", 5, 22, PAL3);
    menu_texto("MANDO 2: CAZADOR", 5, 23, PAL3);
    menu_texto("PULSA START PARA EMPEZAR", 8, 25, PAL1);
}

void mostrar_intro_mundo_actual() {
    const Image* img = &bg_intro_bosque;
    const SpriteDefinition* enemigo = &hunter_sprite;
    if (mundo_actual == MUNDO_VOLCAN)       { img = &bg_intro_volcan;    enemigo = &flame_sprite; }
    else if (mundo_actual == MUNDO_MARINO)  { img = &bg_intro_acuatico;  enemigo = &shark_sprite; }
    else if (mundo_actual == MUNDO_CELESTE) { img = &bg_intro_celeste;   enemigo = &imp_sprite; }
    pantalla_imagen(img);
    VDP_loadFont(&ts_font, DMA);
    VDP_setTextPalette(PAL0);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    PAL_setPalette(PAL3, enemigo->palette->data, CPU);
    pantalla_sprite(&hero_sprite, 104, 150, PAL2, ANIM_LADO, 0, FALSE);
    pantalla_sprite(&chick_sprite, 84, 150, PAL2, 0, 0, FALSE);
    pantalla_sprite(enemigo, 196, 142, PAL3, ANIM_LADO, 0, TRUE);
    titulo_parpadeo = TRUE;
}

void inicializar_juego() {
    pantalla_sprites_reset();
    pantalla_olvidar_imagen();
    titulo_parpadeo = FALSE;
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    cargar_graficos_mundo();

    const SpriteDefinition* cazador_recurso = &hunter_sprite;
    if (mundo_actual == MUNDO_VOLCAN) {
        cazador_recurso = &flame_sprite;
    } else if (mundo_actual == MUNDO_MARINO) {
        cazador_recurso = &shark_sprite;
    } else if (mundo_actual == MUNDO_CELESTE) {
        cazador_recurso = &imp_sprite;
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
    calcular_alcanzables();
    repartir_polluelos_seguros();
    VDP_setEnable(FALSE);          // dibujar con la pantalla apagada: el nivel aparece de golpe
    dibujar_mapa_completo();
    hud_forzar_redibujo();
    VDP_setEnable(TRUE);

    flicky.x = 1 * CELDA; flicky.y = 1 * CELDA; flicky.x_vel = 0; flicky.y_vel = 0;
    flicky.sprite = SPR_addSprite(&hero_sprite, flicky.x - FLICKY_DRAW_OFFSET, flicky.y + PLAY_Y - FLICKY_DRAW_OFFSET, TILE_ATTR(PAL2, 0, FALSE, FALSE));
    
    for (u16 h = 0; h < MAX_HISTORIAL; h++) {
        historial_x[h] = flicky.x;
        historial_y[h] = flicky.y;
    }

    s16 caz_tx = MAP_WIDTH - 2, caz_ty = MAP_HEIGHT - 2;
    for (u16 intento = 0; intento < 20; intento++) {
        if (buscar_celda_libre(&caz_tx, &caz_ty) && (abs(caz_tx - 1) + abs(caz_ty - 1)) >= 10) break;
    }
    cazador.x = caz_tx * CELDA; cazador.y = caz_ty * CELDA; cazador.x_vel = 0; cazador.y_vel = 0;
    
    cazador.sprite = SPR_addSprite(cazador_recurso, cazador.x - CAZADOR_DRAW_OFFSET, cazador.y + PLAY_Y - CAZADOR_DRAW_OFFSET_Y, TILE_ATTR(PAL3, 0, FALSE, FALSE));

    musica_segun_estado(ESTADO_JUEGO);
}

void lauch_partida() {} 

void finalizar_partida() {
    muros_alerta(FALSE); 
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
    VDP_setHorizontalScroll(BG_B, 0);
    
    efectos_reset();
    pantalla_sprites_reset();
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
    muros_alerta(FALSE);
    pantalla_imagen(&bg_continue);
    VDP_loadFont(&ts_font, DMA);
    VDP_setTextPalette(PAL0);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    char buf[40];
    sprintf(buf, "TE QUEDAN %d CONTINUES", continues_restantes);
    texto_centrado(buf, 12);
    pantalla_sprite(&hero_sprite, 152, 160, PAL2, ANIM_HERIDO, 0, FALSE);
    titulo_parpadeo = TRUE;
}

void mostrar_game_over() {
    if (flicky.puntuacion > puntuacion_maxima) {
        puntuacion_maxima = flicky.puntuacion;
    }
    finalizar_partida();
    estado_actual = ESTADO_GAMEOVER;
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    pantalla_imagen(&bg_game_over);
    VDP_loadFont(&ts_font, DMA);
    VDP_setTextPalette(PAL0);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    PAL_setPalette(PAL3, hunter_sprite.palette->data, CPU);
    {
        char buf[40];
        sprintf(buf, "PUNTOS %06ld   RECORD %06ld", flicky.puntuacion, puntuacion_maxima);
        texto_centrado(buf, 12);
    }
    pantalla_sprite(&hunter_sprite, 176, 152, PAL3, ANIM_LADO, 0, TRUE);
    pantalla_sprite(&hero_sprite, 136, 160, PAL2, ANIM_HERIDO, 0, FALSE);
    titulo_parpadeo = TRUE;
    
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


    pantalla_imagen(&bg_fin_juego);
    VDP_loadFont(&ts_font, DMA);
    VDP_setTextPalette(PAL0);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    {
        char buf[40];
        sprintf(buf, "PUNTOS %06ld", flicky.puntuacion);
        texto_centrado(buf, 15);
    }
    pantalla_sprite(&hero_sprite, 152, 150, PAL2, ANIM_FELIZ, 0, FALSE);
    for (u16 i = 0; i < 4; i++) pantalla_sprite(&chick_sprite, 104 + i * 32 + (i >= 2 ? 16 : 0) - (i >= 2 ? 0 : 0), 158, PAL2, 0, 0, FALSE);
    titulo_parpadeo = TRUE;
    
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


        PAL_setPalette(PAL2, pal_items.data, CPU);
        if (victorias_pajaro >= versus_victorias_req) {
            pantalla_imagen(&bg_pajaro_ganador);
            pantalla_sprite(&hero_sprite, 152, 150, PAL2, ANIM_FELIZ, 0, FALSE);
        } else {
            pantalla_imagen(&bg_cazador_ganador);
            PAL_setPalette(PAL3, hunter_sprite.palette->data, CPU);
            pantalla_sprite(&hunter_sprite, 148, 144, PAL3, ANIM_ABAJO, 0, FALSE);
        }
        VDP_loadFont(&ts_font, DMA);
        VDP_setTextPalette(PAL0);
        {
            char buf[40];
            sprintf(buf, "PAJARO %ld  -  %ld CAZADOR", victorias_pajaro, victorias_cazador);
            texto_centrado(buf, 12);
        }
        titulo_parpadeo = TRUE;
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
    return (ty >= NIDO_Y - 1 && ty <= NIDO_Y + 1 && tx >= NIDO_X - 1 && tx <= NIDO_X + NIDO_ANCHO);   // plaza central
}

// Laberinto trenzado y simétrico en rejilla de 20x13 celdas de 16x16.
// 1) backtracker recursivo en la mitad izquierda (celdas impares), 2) se eliminan
// la mayoría de callejones sin salida para crear bucles de escape, 3) columna central
// y plaza del nido abiertas, 4) espejo horizontal. Siempre queda todo conectado.
void generar_mapa_aleatorio() {
    static s16 pila_x[64], pila_y[64];
    static bool visitada[MAP_HEIGHT][10];
    const s16 ddx[4] = {2, -2, 0, 0};
    const s16 ddy[4] = {0, 0, 2, -2};
    setRandomSeed(random() ^ (u16)vtimer);   // cada nivel distinto (depende del tiempo en menús)

    for (u16 y = 0; y < MAP_HEIGHT; y++) {
        for (u16 x = 0; x < MAP_WIDTH; x++) mapa[y][x] = TILE_MURO;
        for (u16 x = 0; x < 10; x++) visitada[y][x] = FALSE;
    }

    u16 sp = 0;
    pila_x[sp] = 1; pila_y[sp] = 1; sp++;
    visitada[1][1] = TRUE; mapa[1][1] = TILE_SUELO;
    while (sp > 0) {
        s16 x = pila_x[sp - 1], y = pila_y[sp - 1];
        u16 opciones[4]; u16 n = 0;
        for (u16 d = 0; d < 4; d++) {
            s16 nx = x + ddx[d], ny = y + ddy[d];
            if (nx >= 1 && nx <= 7 && ny >= 1 && ny <= MAP_HEIGHT - 2 && !visitada[ny][nx]) opciones[n++] = d;
        }
        if (n == 0) { sp--; continue; }
        u16 d = opciones[random() % n];
        s16 nx = x + ddx[d], ny = y + ddy[d];
        mapa[y + ddy[d] / 2][x + ddx[d] / 2] = TILE_SUELO;
        mapa[ny][nx] = TILE_SUELO;
        visitada[ny][nx] = TRUE;
        pila_x[sp] = nx; pila_y[sp] = ny; sp++;
    }

    // Trenzado: quitar callejones sin salida (70%)
    for (s16 y = 1; y <= MAP_HEIGHT - 2; y += 2) {
        for (s16 x = 1; x <= 7; x += 2) {
            u16 salidas = 0;
            if (mapa[y][x + 1] == TILE_SUELO) salidas++;
            if (mapa[y][x - 1] == TILE_SUELO) salidas++;
            if (mapa[y + 1][x] == TILE_SUELO) salidas++;
            if (mapa[y - 1][x] == TILE_SUELO) salidas++;
            if (salidas == 1 && (random() % 100) < 70) {
                u16 opciones[4]; u16 n = 0;
                for (u16 d = 0; d < 4; d++) {
                    s16 mx = x + ddx[d] / 2, my = y + ddy[d] / 2;
                    s16 tx = x + ddx[d], ty = y + ddy[d];
                    if (mapa[my][mx] == TILE_MURO && tx > 0 && tx < MAP_WIDTH - 1 && ty > 0 && ty < MAP_HEIGHT - 1) opciones[n++] = d;
                }
                if (n > 0) { u16 d = opciones[random() % n]; mapa[y + ddy[d] / 2][x + ddx[d] / 2] = TILE_SUELO; }
            }
        }
    }

    // Columna central (9) abierta con algún tope, conexiones desde la columna 8
    for (s16 y = 1; y <= MAP_HEIGHT - 2; y++) {
        mapa[y][9] = ((y % 4) == 2 && (random() & 1)) ? TILE_MURO : TILE_SUELO;
        if (y & 1) mapa[y][8] = TILE_SUELO;
    }
    // Plaza del nido
    for (s16 y = NIDO_Y - 1; y <= NIDO_Y + 1; y++) { mapa[y][8] = TILE_SUELO; mapa[y][9] = TILE_SUELO; }

    // Espejo horizontal y borde
    for (s16 y = 0; y < MAP_HEIGHT; y++)
        for (s16 x = 0; x < 10; x++) mapa[y][MAP_WIDTH - 1 - x] = mapa[y][x];
    for (s16 x = 0; x < MAP_WIDTH; x++) { mapa[0][x] = TILE_MURO; mapa[MAP_HEIGHT - 1][x] = TILE_MURO; }
    for (s16 y = 0; y < MAP_HEIGHT; y++) { mapa[y][0] = TILE_MURO; mapa[y][MAP_WIDTH - 1] = TILE_MURO; }
}

void calcular_alcanzables() {
    for (u16 y = 0; y < MAP_HEIGHT; y++)
        for (u16 x = 0; x < MAP_WIDTH; x++)
            alcanzable[y][x] = FALSE;

    u16 inicio = 0, fin = 0;
    s16 sx = 1, sy = 1; 

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
        u16 ry = (random() % (MAP_HEIGHT - 2)) + 1;
        if (mapa[ry][rx] == TILE_SUELO && !en_zona_nido(rx, ry) && alcanzable[ry][rx]) {
            bool cerca = FALSE;
            for (u16 i = 0; i < repartidos; i++) if (abs(posiciones_polluelos_x[i] - (s16)rx) < 2 && abs(posiciones_polluelos_y[i] - (s16)ry) < 2) cerca = TRUE;
            if (!cerca) {
                mapa[ry][rx] = TILE_POLLUELO;
                posiciones_polluelos_x[repartidos] = rx;
                posiciones_polluelos_y[repartidos] = ry;
                repartidos++;
            }
        }
    }
}

bool buscar_celda_libre(s16* out_x, s16* out_y) {
    for (u16 intentos = 0; intentos < 200; intentos++) {
        u16 rx = (random() % (MAP_WIDTH - 2)) + 1;
        u16 ry = (random() % (MAP_HEIGHT - 2)) + 1;
        if (mapa[ry][rx] == TILE_SUELO && !en_zona_nido(rx, ry) && alcanzable[ry][rx] && !(rx == (u16)((flicky.x + 8) >> CELDA_SHIFT) && ry == (u16)((flicky.y + 8) >> CELDA_SHIFT))) {
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
    dibujar_celda(rx, ry);
}

void aplicar_recorte_battle_royale() {
    anillo_recorte += 1;
    en_preaviso_cierre = FALSE;
    muros_alerta(FALSE); 
    
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
            }
        }
    }
    dibujar_mapa_completo();
    reproducir_sonido_item(180);
}

void leer_entrada() {
    u16 value1 = JOY_readJoypad(JOY_1);
    
    s16 flicky_offset_x = flicky.x & CELDA_MASK; 
    s16 flicky_offset_y = flicky.y & CELDA_MASK; 

    if ((flicky_offset_x <= 3 || flicky_offset_x >= 13) && (flicky_offset_y <= 3 || flicky_offset_y >= 13)) {
        s16 nx = 0, ny = 0;
        if (value1 & BUTTON_LEFT) nx = -1; 
        else if (value1 & BUTTON_RIGHT) nx = 1;
        else if (value1 & BUTTON_UP) ny = -1; 
        else if (value1 & BUTTON_DOWN) ny = 1;
        
        if ((nx != 0 || ny != 0) && (nx != flicky.x_vel || ny != flicky.y_vel)) {
            s16 tx = ((flicky.x + 8) >> CELDA_SHIFT) + nx; 
            s16 ty = ((flicky.y + 8) >> CELDA_SHIFT) + ny; 
            if (tx >= 0 && tx < MAP_WIDTH && ty >= 0 && ty < MAP_HEIGHT && mapa[ty][tx] != TILE_MURO) {
                flicky.x = ((flicky.x + 8) >> CELDA_SHIFT) << CELDA_SHIFT; 
                flicky.y = ((flicky.y + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
                flicky.x_vel = nx; flicky.y_vel = ny;
            }
        }
    }

    if (modo_jugadores == 1) {
        u16 value2 = JOY_readJoypad(JOY_2);
        s16 cazador_offset_x = cazador.x & CELDA_MASK;
        s16 cazador_offset_y = cazador.y & CELDA_MASK;

        if ((cazador_offset_x <= 3 || cazador_offset_x >= 13) && (cazador_offset_y <= 3 || cazador_offset_y >= 13)) {
            s16 c_nx = 0, c_ny = 0;
            if (value2 & BUTTON_LEFT) c_nx = -1; 
            else if (value2 & BUTTON_RIGHT) c_nx = 1;
            else if (value2 & BUTTON_UP) c_ny = -1; 
            else if (value2 & BUTTON_DOWN) c_ny = 1;
            
            if ((c_nx != 0 || c_ny != 0) && (c_nx != cazador.x_vel || c_ny != cazador.y_vel)) {
                s16 c_tx = ((cazador.x + 8) >> CELDA_SHIFT) + c_nx; 
                s16 c_ty = ((cazador.y + 8) >> CELDA_SHIFT) + c_ny; 
                if (c_tx >= 0 && c_tx < MAP_WIDTH && c_ty >= 0 && c_ty < MAP_HEIGHT && mapa[c_ty][c_tx] != TILE_MURO) {
                    cazador.x = ((cazador.x + 8) >> CELDA_SHIFT) << CELDA_SHIFT; 
                    cazador.y = ((cazador.y + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
                    cazador.x_vel = c_nx; cazador.y_vel = c_ny;
                }
            }
        }
    }
}

void comprobar_recogida_objetos() {
    u16 cx = (flicky.x + 8) >> CELDA_SHIFT;
    u16 cy = (flicky.y + 8) >> CELDA_SHIFT;

    if (cx < MAP_WIDTH && cy < MAP_HEIGHT) {
        u8 tipo_objeto = mapa[cy][cx];

        if (tipo_objeto == TILE_POLLUELO && semillas_en_fila < max_polluelos_nivel) {
            mapa[cy][cx] = TILE_SUELO; 
            dibujar_celda(cx, cy); 
            sprites_seguidores[semillas_en_fila] = SPR_addSprite(&chick_sprite, flicky.x, flicky.y + PLAY_Y, TILE_ATTR(PAL2, 0, FALSE, FALSE));
            SPR_setAnimationLoop(sprites_seguidores[semillas_en_fila], TRUE);
            semillas_en_fila++; 
            efecto_chispa(cx * CELDA, cy * CELDA);
            reproducir_sonido_waka();
        } 
        else if (tipo_objeto == TILE_GEMA) {
            mapa[cy][cx] = TILE_SUELO; 
            dibujar_celda(cx, cy);
            flicky.puntuacion += 250;
            efecto_chispa(cx * CELDA, cy * CELDA);
            reproducir_sonido_item(1400);
            efecto_shake(6, 1);
        } 
        else if (tipo_objeto == TILE_RELOJ) {
            mapa[cy][cx] = TILE_SUELO; 
            dibujar_celda(cx, cy);
            timer_cazador_lento = DURACION_PODER_FRAMES;
            cazador_en_furia = FALSE; 
            muros_alerta(FALSE);
            efecto_chispa(cx * CELDA, cy * CELDA);
            reproducir_sonido_item(220);
        } 
        else if (tipo_objeto == TILE_RAYO) {
            mapa[cy][cx] = TILE_SUELO; 
            dibujar_celda(cx, cy);
            timer_flicky_rapido = DURACION_PODER_FRAMES;
            efecto_chispa(cx * CELDA, cy * CELDA);
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
    efectos_actualizar();
    animar_polluelos_mapa();
    cazador_tinte_actualizar();
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
        u16 fase = (frame_counter >> 1) & 15;
        muros_mezcla(fase < 8 ? fase : 16 - fase);
    }

    if (cazador_en_furia) {
        // pulso suave (sin parpadeo brusco a pantalla completa)
        u16 fase = (frame_counter >> 1) & 15;
        muros_mezcla(fase < 8 ? fase : 16 - fase);
    }

    // (el aviso de "meta abierta" parpadea en el marcador: ver hud_actualizar)

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
                        sonido_rabia();
                        efecto_shake(15, 2);
                    }
                } else {
                    timer_duracion_furia--;
                    if (timer_duracion_furia <= 0) {
                        cazador_en_furia = FALSE;
                        muros_alerta(FALSE);
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
        s16 col_x = (flicky.x_vel > 0) ? (next_x + CELDA_MASK) : next_x;
        s16 col_y = (flicky.y_vel > 0) ? (next_y + CELDA_MASK) : next_y;

        if (mapa[col_y >> CELDA_SHIFT][col_x >> CELDA_SHIFT] != TILE_MURO) {
            flicky.x = next_x;
            flicky.y = next_y;
        } else {
            flicky.x = ((flicky.x + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
            flicky.y = ((flicky.y + 8) >> CELDA_SHIFT) << CELDA_SHIFT;
            flicky.x_vel = 0; flicky.y_vel = 0;
        }

        historial_x[indice_historial] = flicky.x;
        historial_y[indice_historial] = flicky.y;
        indice_historial = (indice_historial + 1) & HIST_MASK; 
    }

    comprobar_recogida_objetos();

    u16 cx = (flicky.x + 8) >> CELDA_SHIFT;
    u16 cy = (flicky.y + 8) >> CELDA_SHIFT;

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
                efecto_chispa((NIDO_X * CELDA) + ((i & 1) ? 16 : 0), NIDO_Y * CELDA - 4);
                hud_actualizar();
                for(s16 d=0; d<6; d++) { efectos_actualizar(); SPR_update(); SYS_doVBlankProcess(); }
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
                muros_alerta(FALSE);
                cazador_en_furia = FALSE; timer_cazador_lento = 0; cazador_tinte_actualizar();
                banner("FASE SUPERADA!");
                musica_detener(); XGM2_playTrack(PISTA_SUPERADA);
                secuencia_heroe(ANIM_FELIZ, 150, TRUE);
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
                    banner("HAS ESCAPADO CON EXITO!");
                    musica_detener(); XGM2_playTrack(PISTA_SUPERADA);
                    for (u16 f = 0; f < 150; f++) { SPR_update(); SYS_doVBlankProcess(); }
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
            s16 r = (indice_historial - 1 - ((i + 1) * SEPARACION_POLLUELOS) + MAX_HISTORIAL) & HIST_MASK; 
            SPR_setPosition(sprites_seguidores[i], historial_x[r], historial_y[r] + PLAY_Y); 
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
    
    SPR_setPosition(flicky.sprite, flicky.x - FLICKY_DRAW_OFFSET + shake_x, flicky.y + PLAY_Y - FLICKY_DRAW_OFFSET);
    mover_cazador();

    // ===== COLISIÓN CON CAZADOR (MECÁNICAS AJUSTADAS) =====
    if (abs(flicky.x - cazador.x) < 12 && abs(flicky.y - cazador.y) < 12) {
        if (flicky_invulnerable) {
            // Ignorar daño durante la inmunidad temporal
        }
        // NUEVO: Si está en MODO RABIA (cazador_en_furia), ignoramos la protección de polluelos y muere directamente
        else if (cazador_en_furia && modo_jugadores == 0) {
            efecto_shake(20, 3);
            reproducir_sonido_panico();
            victorias_cazador++;
            muros_alerta(TRUE);
            banner("LA RABIA TE DESTRUYO!");
            musica_detener();
            sonido_atrapado();
            SPR_setAnim(flicky.sprite, ANIM_HERIDO); flicky.anim_actual = ANIM_HERIDO; SPR_setHFlip(flicky.sprite, FALSE);
            for (u16 f = 0; f < 100; f++) {
                if (f < 30) efecto_shake_actualizar();
                SPR_setFrame(flicky.sprite, (f >> 3) & 3);
                SPR_setPosition(flicky.sprite, flicky.x + shake_x, flicky.y + PLAY_Y);
                efectos_actualizar();
                SPR_update();
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
            muros_alerta(FALSE);
            banner("EL CAZADOR TE ATRAPO!");
            musica_detener();
            sonido_atrapado();
            SPR_setAnim(flicky.sprite, ANIM_HERIDO); flicky.anim_actual = ANIM_HERIDO; SPR_setHFlip(flicky.sprite, FALSE);
            for (u16 f = 0; f < 100; f++) {
                if (f < 30) efecto_shake_actualizar();
                SPR_setFrame(flicky.sprite, (f >> 3) & 3);
                SPR_setPosition(flicky.sprite, flicky.x + shake_x, flicky.y + PLAY_Y);
                efectos_actualizar();
                SPR_update();
                SYS_doVBlankProcess();
            }
            if (continues_restantes > 0) { mostrar_pantalla_continue(); } else { mostrar_game_over(); }
            return;
        }
    }

    hud_actualizar();
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
            if (((cazador.x & CELDA_MASK) == 0) && ((cazador.y & CELDA_MASK) == 0)) {
                s16 tx = cazador.x >> CELDA_SHIFT, ty = cazador.y >> CELDA_SHIFT;
                bool lizq = (tx > 0) && (mapa[ty][tx - 1] != TILE_MURO) && (cazador.x_vel != 1);
                bool lder = (tx < MAP_WIDTH - 1) && (mapa[ty][tx + 1] != TILE_MURO) && (cazador.x_vel != -1);
                bool larr = (ty > 0) && (mapa[ty - 1][tx] != TILE_MURO) && (cazador.y_vel != 1);
                bool laba = (ty < MAP_HEIGHT - 1) && (mapa[ty + 1][tx] != TILE_MURO) && (cazador.y_vel != -1);
                s16 ptx = tx + cazador.x_vel, pty = ty + cazador.y_vel;
                bool blq = (ptx < 0 || ptx >= MAP_WIDTH || pty < 0 || pty >= MAP_HEIGHT || mapa[pty][ptx] == TILE_MURO);

                if (semillas_en_fila > 0 && ((random() & 63) < 45)) { 
                    s16 ox = 0, oy = 0;
                    if (((flicky.x + 8) >> CELDA_SHIFT) < tx && lizq) { ox = -1; oy = 0; }
                    else if (((flicky.x + 8) >> CELDA_SHIFT) > tx && lder) { ox = 1; oy = 0; }
                    else if (((flicky.y + 8) >> CELDA_SHIFT) < ty && larr) { ox = 0; oy = -1; }
                    else if (((flicky.y + 8) >> CELDA_SHIFT) > ty && laba) { ox = 0; oy = 1; }
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
        s16 c_col_x = (cazador.x_vel > 0) ? (px + CELDA_MASK) : px;
        s16 c_col_y = (cazador.y_vel > 0) ? (py + CELDA_MASK) : py;

        if (mapa[c_col_y >> CELDA_SHIFT][c_col_x >> CELDA_SHIFT] != TILE_MURO) { 
            cazador.x = px; cazador.y = py; 
        } else { 
            cazador.x = ((cazador.x + 8) >> CELDA_SHIFT) << CELDA_SHIFT; cazador.y = ((cazador.y + 8) >> CELDA_SHIFT) << CELDA_SHIFT; 
            cazador.x_vel = 0; cazador.y_vel = 0; 
        }
    }

    bool cazador_mira_izquierda = FALSE;   // todos los sprites nuevos miran a la derecha
    actualizar_anim_direccion(&cazador.anim_actual, cazador.sprite, cazador.x_vel, cazador.y_vel, cazador_mira_izquierda);
    
    SPR_setPosition(cazador.sprite, cazador.x - CAZADOR_DRAW_OFFSET + shake_x, cazador.y + PLAY_Y - CAZADOR_DRAW_OFFSET_Y);
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

// ================= FASE 2: EFECTOS Y ANIMACIONES =================
#define MAX_CHISPAS 6
static Sprite* chispas[MAX_CHISPAS];
static u8 chispa_timer[MAX_CHISPAS];

void efectos_reset() {
    // SPR_clear() solo oculta: hay que liberar los sprites de verdad
    for (u16 i = 0; i < MAX_CHISPAS; i++) {
        if (chispas[i]) SPR_releaseSprite(chispas[i]);
        chispas[i] = NULL; chispa_timer[i] = 0;
    }
}

void efecto_chispa(s16 x, s16 y) {
    for (u16 i = 0; i < MAX_CHISPAS; i++) {
        if (chispas[i] == NULL) {
            chispas[i] = SPR_addSprite(&spark_sprite, x + shake_x, y + PLAY_Y, TILE_ATTR(PAL2, 1, FALSE, FALSE));
            if (chispas[i]) { SPR_setAnimationLoop(chispas[i], FALSE); chispa_timer[i] = 16; }
            return;
        }
    }
}

void efectos_actualizar() {
    for (u16 i = 0; i < MAX_CHISPAS; i++) {
        if (chispas[i] && --chispa_timer[i] == 0) { SPR_releaseSprite(chispas[i]); chispas[i] = NULL; }
    }
}

// Los polluelos del mapa (plano A) se animan cambiando sus 4 tiles en VRAM
void animar_polluelos_mapa() {
    static u8 t = 0, f = 0;
    if (++t < 10) return;
    t = 0; f = (f + 1) & 3;
    VDP_loadTileData(ts_chick_anim.tiles + f * 4 * 8, base_items, 4, DMA);
}

// Cazador: rojo en modo rabia, azulado cuando el reloj lo ralentiza
void cazador_tinte_actualizar() {
    static u8 tinte_actual = 0xFF;
    u8 tinte = cazador_en_furia ? 1 : ((timer_cazador_lento > 0) ? 2 : 0);
    if (tinte == tinte_actual || cazador.sprite == NULL) return;
    tinte_actual = tinte;
    const u16* src = cazador.sprite->definition->palette->data;
    for (u16 i = 1; i < 16; i++) {
        u16 c = src[i];
        u16 r = (c >> 1) & 7, g = (c >> 5) & 7, b = (c >> 9) & 7;
        if (tinte == 1) { r = (r + 4 > 7) ? 7 : r + 4; g >>= 1; b >>= 1; }
        else if (tinte == 2) { b = (b + 3 > 7) ? 7 : b + 3; r >>= 1; }
        PAL_setColor(48 + i, (b << 9) | (g << 5) | (r << 1));
    }
}

// Cartel centrado sobre el laberinto (fondo de la barra del marcador)
void banner(const char* txt) {
    u16 len = strlen(txt);
    u16 x0 = (40 - len) / 2 - 2, x1 = x0 + len + 3;
    // arriba o abajo, para no tapar al protagonista
    u16 yc = ((flicky.y + PLAY_Y) >> 3) < 15 ? 21 : 7;
    for (u16 y = yc - 1; y <= yc + 1; y++) {
        for (u16 x = x0; x <= x1; x++) {
            u16 tile = base_maze + 72;
            u16 attr = TILE_ATTR_FULL(PAL0, 1, 0, 0, tile);
            if (y == yc + 1) attr = TILE_ATTR_FULL(PAL0, 1, 0, 0, base_maze + 73);
            if (y == yc - 1) attr = TILE_ATTR_FULL(PAL0, 1, 1, 0, base_maze + 73);
            VDP_setTileMapXY(BG_B, attr, x, y);
            VDP_setTileMapXY(BG_A, 0, x, y);
        }
    }
    VDP_setTextPalette(PAL0);
    VDP_setTextPriority(1);
    VDP_drawText(txt, (40 - len) / 2, yc);
    VDP_setTextPriority(0);
}

// Reproduce una animación del protagonista bloqueando el juego (celebración)
void secuencia_heroe(u8 anim, u16 frames, bool salta) {
    if (!flicky.sprite) return;
    static const s8 salto[8] = {0, -3, -5, -6, -6, -5, -3, 0};
    SPR_setAnim(flicky.sprite, anim); flicky.anim_actual = anim; SPR_setHFlip(flicky.sprite, FALSE);
    for (u16 f = 0; f < frames; f++) {
        SPR_setFrame(flicky.sprite, (f >> 3) & 3);
        s16 dy = salta ? salto[(f >> 1) & 7] : 0;
        SPR_setPosition(flicky.sprite, flicky.x, flicky.y + PLAY_Y + dy);
        if ((f & 15) == 0) efecto_chispa(flicky.x + ((f & 16) ? -10 : 10), flicky.y - 6);
        efectos_actualizar();
        musica_actualizar();
        SPR_update();
        SYS_doVBlankProcess();
    }
}


// ================= FASE 4: PANTALLAS =================
#define MAX_SPR_PANTALLA 10
static Sprite* spr_pant[MAX_SPR_PANTALLA];
static s16 spr_pant_x[MAX_SPR_PANTALLA], spr_pant_y[MAX_SPR_PANTALLA], spr_pant_vx[MAX_SPR_PANTALLA];
bool titulo_parpadeo = FALSE;

void pantalla_olvidar_imagen();
void pantalla_sprites_reset() {
    for (u16 i = 0; i < MAX_SPR_PANTALLA; i++) {
        if (spr_pant[i]) SPR_releaseSprite(spr_pant[i]);
        spr_pant[i] = NULL;
    }
}

Sprite* pantalla_sprite(const SpriteDefinition* def, s16 x, s16 y, u16 pal, u16 anim, s16 vx, bool flip) {
    for (u16 i = 0; i < MAX_SPR_PANTALLA; i++) {
        if (spr_pant[i] == NULL) {
            Sprite* sp = SPR_addSprite(def, x, y, TILE_ATTR(pal, 1, FALSE, flip));
            if (!sp) return NULL;
            SPR_setAnim(sp, anim);
            spr_pant[i] = sp; spr_pant_x[i] = x; spr_pant_y[i] = y; spr_pant_vx[i] = vx;
            return sp;
        }
    }
    return NULL;
}

// Imagen de fondo a pantalla completa en el plano B (paleta 0) con el plano A limpio
static const Image* imagen_actual = NULL;
void pantalla_imagen(const Image* img) {
    pantalla_sprites_reset();
    if (img == imagen_actual) {          // misma pantalla (p.ej. moverse por un menú): solo se rehace el texto
        VDP_clearPlane(BG_A, TRUE);
        return;
    }
    imagen_actual = img;
    VDP_setEnable(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    VDP_setHorizontalScroll(BG_A, 0);
    VDP_setHorizontalScroll(BG_B, 0);
    PAL_setPalette(PAL0, img->palette->data, CPU);
    VDP_drawImageEx(BG_B, img, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);
    VDP_setEnable(TRUE);
}

void pantalla_olvidar_imagen() { imagen_actual = NULL; }

void texto_centrado(const char* txt, u16 y) {
    VDP_drawText(txt, (40 - strlen(txt)) / 2, y);
}

// Animación de las pantallas: sprites que andan o se mueven en el sitio y "PULSA START" parpadeando
void pantalla_actualizar() {
    static u16 t = 0;
    t++;
    for (u16 i = 0; i < MAX_SPR_PANTALLA; i++) {
        Sprite* sp = spr_pant[i];
        if (!sp) continue;
        if ((t & 7) == 0) SPR_nextFrame(sp);
        if (spr_pant_vx[i] != 0 && (t & 1)) {
            spr_pant_x[i] += spr_pant_vx[i];
            if (spr_pant_x[i] > 336) spr_pant_x[i] -= 400;
        }
        // los polluelos dan saltitos
        s16 dy = (sp->definition == &chick_sprite) ? (((t + i * 5) & 15) < 4 ? -2 : 0) : 0;
        SPR_setPosition(sp, spr_pant_x[i], spr_pant_y[i] + dy);
    }
    if (titulo_parpadeo && (estado_actual == ESTADO_TITULO || estado_actual == ESTADO_INTRO ||
                            estado_actual == ESTADO_CONTINUE || estado_actual == ESTADO_GAMEOVER || estado_actual == ESTADO_FINJUEGO)) {
        u16 fila = (estado_actual == ESTADO_TITULO) ? 17 : 24;
        if ((t & 31) == 0) texto_centrado("PULSA START", fila);
        else if ((t & 31) == 20) VDP_clearText(14, fila, 12);
    }
}
