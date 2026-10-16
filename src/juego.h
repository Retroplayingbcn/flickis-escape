// Flicki's Escape - declaraciones compartidas entre módulos
#ifndef JUEGO_H
#define JUEGO_H

#include <genesis.h>
#include "resources.h"
#include "hud_chars.h"

// ---------------------------------------------------------------- rejilla y mapa
#define MAP_WIDTH    20
#define MAP_HEIGHT   13
#define CELDA        16
#define CELDA_SHIFT  4
#define CELDA_MASK   15
#define PLAY_Y       16          // el marcador ocupa los 16 px superiores
#define PLAY_TY      2           // fila de tiles donde empieza el laberinto

enum { TILE_SUELO, TILE_MURO, TILE_POLLUELO, TILE_NIDO, TILE_RELOJ, TILE_RAYO, TILE_GEMA, TILE_HUEVO };

// iconos del tileset de objetos (4 tiles cada uno)
enum { ITEM_POLLUELO, ITEM_RELOJ, ITEM_RAYO, ITEM_GEMA, ITEM_CARA, ITEM_HUEVO, ITEM_RED, ITEM_RED_GRIS };

#define NIDO_X      9
#define NIDO_Y      6
#define NIDO_ANCHO  2

// animaciones de los sprites de personajes
#define ANIM_ABAJO  0
#define ANIM_ARRIBA 1
#define ANIM_LADO   2
#define ANIM_HERIDO 3
#define ANIM_FELIZ  4

// ---------------------------------------------------------------- estados
enum { ESTADO_TITULO, ESTADO_MENU, ESTADO_JUEGO, ESTADO_GAMEOVER, ESTADO_FINJUEGO, ESTADO_CONTINUE,
       ESTADO_INTRO, ESTADO_COMO_JUGAR, ESTADO_MENU_VERSUS, ESTADO_IDIOMA, ESTADO_HISTORIA, ESTADO_PASSWORD };

enum { MUNDO_BOSQUE, MUNDO_VOLCAN, MUNDO_MARINO, MUNDO_CELESTE };
#define FASE_JEFE        6       // la sexta fase de cada mundo es el enfrentamiento final

#define MAX_HISTORIAL         256
#define HIST_MASK             255
#define SEPARACION_POLLUELOS  12
#define MAX_POLLUELOS         15
#define DURACION_PODER        300
#define TIEMPO_INVULNERABLE   150
#define MAX_MUNICION          3
#define PUNTOS_VIDA_EXTRA     5000

// ---------------------------------------------------------------- idioma
extern u16 idioma;                               // 0 castellano, 1 inglés
#define T(es, en) (idioma ? (en) : (es))
#define IMG(nombre) (idioma ? &nombre##_en : &nombre)

// ---------------------------------------------------------------- entidades
typedef struct {
    s16 x, y;            // esquina superior izquierda en el área de juego (px)
    s16 x_vel, y_vel;
    s16 dir_x, dir_y;    // última dirección (para lanzar huevos)
    Sprite* sprite;
    u8 anim;
    u32 puntuacion;
} Pajaro;

typedef struct {
    s16 x, y;
    s16 x_vel, y_vel;
    Sprite* sprite;
    const SpriteDefinition* def;
    u8 anim;
    u16 acumulador;      // velocidad en 1/8 de píxel por frame
    s16 aturdido;        // frames aturdido por un huevo
    s16 inmune;          // frames sin poder volver a ser aturdido
    s16 embestida;       // tiburón: 1 mientras embiste
    s16 recarga;         // tiburón: espera entre embestidas
    s16 tele_timer;      // diablillo: cuenta hasta el siguiente teletransporte
    s16 oculto;          // diablillo: frames invisible durante el teletransporte
} Cazador;

// ---------------------------------------------------------------- estado global (partida.c)
extern Pajaro flicky;
extern Cazador cazador;
extern u8 estado_actual;
extern u8 mapa[MAP_HEIGHT][MAP_WIDTH];
extern bool alcanzable[MAP_HEIGHT][MAP_WIDTH];
extern u8 distancia[MAP_HEIGHT][MAP_WIDTH];
extern u8 deco[MAP_HEIGHT][MAP_WIDTH];        // adornos del suelo (0 = nada, 1..6) y piezas especiales:
#define DECO_ESPECIAL 0x80                      //   muro dibujado como árbol/tronco/follaje (pieza en los bits bajos)
#define DECO_CAMINO   0x40                      //   suelo de tierra (camino del claro)
enum { PIEZA_ARBOL_TL, PIEZA_ARBOL_TR, PIEZA_ARBOL_BL, PIEZA_ARBOL_BR, PIEZA_TRONCO_I, PIEZA_TRONCO_D, PIEZA_FOLLAJE1, PIEZA_FOLLAJE2 };

extern u16 d_sel, max_polluelos_nivel, semillas_en_fila, total_salvados, municion;
extern Sprite* sprites_seguidores[MAX_POLLUELOS];
extern s16 historial_x[MAX_HISTORIAL], historial_y[MAX_HISTORIAL];
extern u16 indice_historial;
extern s32 tiempo_restante_segundos;
extern u32 frame_counter;
extern s16 timer_cazador_lento, timer_flicky_rapido, timer_spawn_powerup;
extern s16 vidas;
extern u32 puntuacion_maxima, proximo_hito_vida;
extern bool juego_pausado, cazador_en_furia, flicky_invulnerable, salida_abierta, en_jefe;
extern s16 timer_duracion_furia, timer_invulnerabilidad;
extern u16 modo_juego_seleccionada, mundo_actual, fase_actual, modo_jugadores;
extern u32 total_partidas, victorias_j1, victorias_j2;
extern u16 multiplicador_velocidad_minuto;
extern bool modo_battle_royale, en_preaviso_cierre;
extern u16 anillo_recorte;
extern u16 versus_sel, versus_tiempo_idx, versus_victorias_req, versus_mundo_idx;
extern bool historia_desde_menu;
// versus: los papeles se alternan cada ronda; el battle royale solo existe en este modo
extern bool p1_es_pajaro, ganador_fue_pajaro;
extern u16 versus_ronda;
extern u16 versus_papeles;                      // 0: J1 pájaro, 1: J2 pájaro, 2: se alternan
extern s16 timer_enredado, timer_flicky_lento, timer_cazador_rapido;
#define VICTORIAS_PARA_GANAR ((u32)((versus_victorias_req + 1) / 2))   // al mejor de 3 = gana quien llega a 2
// jefes: Flicki crece con la fuerza de sus polluelos; cada polluelo es un corazón
extern s16 corazones;
extern bool flicky_grande;
extern s16 password_mundo;                      // último jefe alcanzado (-1 = ninguno)
#define MAX_CORAZONES 3
#define FLICKY_SPR_X (flicky.x - (flicky_grande ? 4 : 0))
#define FLICKY_SPR_Y (flicky.y - (flicky_grande ? 8 : 0))
#define BR_ACTIVO (modo_battle_royale && modo_jugadores == 1)
#define ENREDO_FRAMES 40
#define RECARGA_RED   240

// ---------------------------------------------------------------- sonido.c
enum { PISTA_TITULO, PISTA_MENU, PISTA_BOSQUE, PISTA_VOLCAN, PISTA_MARINO, PISTA_CELESTE,
       PISTA_INTRO, PISTA_SUPERADA, PISTA_GAMEOVER, PISTA_VICTORIA, PISTA_CONTINUAR, PISTA_JEFE };
enum { SFX_CLICK, SFX_ALERTA, SFX_PIO, SFX_NIDO, SFX_GEMA, SFX_RELOJ, SFX_RAYO, SFX_META, SFX_CIERRE,
       SFX_PANICO, SFX_ATRAPADO, SFX_RABIA, SFX_LANZAR, SFX_CHOF, SFX_ATURDIDO, SFX_GOLPE,
       SFX_EXPLOSION, SFX_VIDA, SFX_TELEPORT, SFX_RED };
void musica_pista(u16 pista);
void musica_reiniciar(u16 pista);
void musica_segun_estado(u8 estado);
void musica_detener();
void musica_pausar();
void musica_reanudar();
void sonido(u16 efecto);
void sonido_nido(u16 n);

// ---------------------------------------------------------------- guardado.c
void cargar_config();
void guardar_config();

// ---------------------------------------------------------------- mapa.c
extern u16 base_maze, base_floor, base_items, base_hud;
void cargar_graficos_mundo();
void generar_mapa_aleatorio();
void generar_arena();
void decorar(u16 n, bool volumen);
bool muros_en_alerta();
void deco_viento(bool doblado);
void calcular_alcanzables();
void calcular_distancias(s16 cx, s16 cy);
void repartir_polluelos_seguros();
bool buscar_celda_libre(s16* out_x, s16* out_y, s16 dist_min_jugador);
bool hay_muro(s16 x, s16 y);
bool en_zona_nido(s16 tx, s16 ty);
void dibujar_celda(s16 cx, s16 cy);
void dibujar_mapa_completo();
void dibujar_nido();
void muros_mezcla(u16 nivel);
void muros_alerta(bool activa);
void mapa_paleta_animar();
void objetos_reset();
void objeto_colocar(s16 x, s16 y, u8 tipo, bool caduca);
void objetos_actualizar();
void generar_powerup_aleatorio();
void animar_polluelos_mapa();

// ---------------------------------------------------------------- hud.c
void hud_forzar_redibujo();
void hud_mensaje(const char* txt);
void hud_actualizar();

// ---------------------------------------------------------------- fx.c
extern s16 shake_x;
void efecto_shake(u16 duracion, u16 intensidad);
void fx_reset();
void fx_actualizar();
void fx_chispa(s16 x, s16 y);
void fx_estallido(s16 x, s16 y);
void fx_popup(s16 x, s16 y, u16 indice);
void fx_polvo(s16 x, s16 y);
void fx_destello();
void fx_scroll_juego(bool activo);
void fx_scroll_actualizar();
void fx_fundido_salida();
void fx_fundido_entrada();
void banner(const char* txt);
void banner_doble(const char* a, const char* b);

// ---------------------------------------------------------------- ambiente.c
void ambiente_reset();
void ambiente_actualizar();
enum { POP_100, POP_200, POP_300, POP_400, POP_500, POP_600, POP_700, POP_800, POP_900, POP_1000, POP_250, POP_1UP, POP_AVISO };

// ---------------------------------------------------------------- jugador.c
void jugador_crear(s16 cx, s16 cy);
void jugador_entrada(u16 joy, u16 pulsado);
void jugador_actualizar();
void jugador_animar();
void jugador_golpeado();
void dispersar_polluelos();
bool polluelo_volar(s16 sx, s16 sy);
void jugador_enredar();
void jugador_crecer(bool grande);
void jugador_herido_jefe();
void polluelos_poder();
u16 municion_max();
void polluelos_voladores_actualizar();
void polluelos_reset();
void secuencia_heroe(u8 anim, u16 frames, bool salta);
void ganar_vida();
void sumar_puntos(u32 puntos);

// ---------------------------------------------------------------- proyectiles.c
void proyectiles_reset();
void lanzar_huevo();
void proyectiles_actualizar();
void disparo_jefe(u16 tipo, s16 x, s16 y, s16 vx, s16 vy, s16 vida);
enum { DISPARO_RED, DISPARO_FUEGO, DISPARO_BOLA };

// ---------------------------------------------------------------- enemigo.c
void cazador_crear(s16 cx, s16 cy);
void cazador_entrada_humana(u16 joy2, u16 pulsado2);
void cazador_actualizar();
void cazador_aturdir();
bool cazador_peligroso();
void cazador_liberar();
void cazador_tinte_actualizar(bool forzar);
const SpriteDefinition* enemigo_del_mundo();

// ---------------------------------------------------------------- jefe.c
void jefe_iniciar();
void jefe_actualizar();
bool jefe_impacto(s16 x, s16 y);
bool jefe_toca(s16 x, s16 y, s16 radio);
void jefe_reset();
bool jefe_caer();
s16 jefe_vida();
s16 jefe_vida_max();
const char* jefe_nombre();

// ---------------------------------------------------------------- partida.c
void inicializar_juego();
void finalizar_partida();
void resetear_tiempo_nivel();
void actualizar_logica();
void jugador_muere(const char* motivo);
void fase_superada(bool fue_jefe);
void verificar_fin_ronda_versus(bool gano_pajaro);
void preparados();

// ---------------------------------------------------------------- pantallas.c
void pantalla_imagen(const Image* img);
void pantalla_olvidar_imagen();
void pantalla_presentar();
void pantalla_sprites_reset();
Sprite* pantalla_sprite(const SpriteDefinition* def, s16 x, s16 y, u16 pal, u16 anim, s16 vx, bool flip);
void pantalla_actualizar();
void texto_centrado(const char* txt, u16 y);
void mostrar_idioma();
void mostrar_pantalla_titulo();
void mostrar_menu_opciones();
void mostrar_pantalla_como_jugar();
void mostrar_menu_versus();
void mostrar_intro_mundo_actual();
void mostrar_historia(bool desde_menu);
bool historia_actualizar();
void mostrar_pantalla_continue();
void mostrar_game_over();
void mostrar_enhorabuena();
void mostrar_ganador_versus();
void mostrar_password();
void password_entrada(u16 pulsado);

// ---------------------------------------------------------------- password.c
const char* password_de(u16 mundo, u16 dif);
bool password_validar(const char* clave, u16* mundo, u16* dif);
extern bool titulo_parpadeo;

#endif
