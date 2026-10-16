// Pantallas: idioma, título, menús, historia, cómo jugar, intros de mundo y desenlaces
#include "juego.h"

bool titulo_parpadeo = FALSE;
static const Image* imagen_actual = NULL;
static bool presentar_pendiente = FALSE;

// ------------------------------------------------------------------ utilidades
#define MAX_SPR_PANTALLA 10
static Sprite* spr_pant[MAX_SPR_PANTALLA];
static s16 spr_x[MAX_SPR_PANTALLA], spr_y[MAX_SPR_PANTALLA], spr_vx[MAX_SPR_PANTALLA];

void pantalla_sprites_reset() {
    for (u16 i = 0; i < MAX_SPR_PANTALLA; i++) {
        if (spr_pant[i]) SPR_releaseSprite(spr_pant[i]);
        spr_pant[i] = NULL;
    }
}

Sprite* pantalla_sprite(const SpriteDefinition* def, s16 x, s16 y, u16 pal, u16 anim, s16 vx, bool flip) {
    for (u16 i = 0; i < MAX_SPR_PANTALLA; i++) {
        if (spr_pant[i]) continue;
        Sprite* sp = SPR_addSprite(def, x, y, TILE_ATTR(pal, 1, FALSE, flip));
        if (!sp) return NULL;
        SPR_setAnim(sp, anim);
        spr_pant[i] = sp; spr_x[i] = x; spr_y[i] = y; spr_vx[i] = vx;
        return sp;
    }
    return NULL;
}

// Imagen de fondo a pantalla completa (plano B, paleta 0). Entre pantallas distintas hay fundido;
// si es la misma (moverse por un menú) solo se rehace el texto.
void pantalla_imagen(const Image* img) {
    pantalla_sprites_reset();
    titulo_parpadeo = FALSE;
    if (img == imagen_actual) { VDP_clearPlane(BG_A, TRUE); return; }
    fx_fundido_salida();
    imagen_actual = img;
    VDP_setEnable(FALSE);
    fx_scroll_juego(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    PAL_setPalette(PAL0, img->palette->data, CPU);
    VDP_drawImageEx(BG_B, img, TILE_ATTR_FULL(PAL0, 0, 0, 0, TILE_USER_INDEX), 0, 0, FALSE, DMA);
    presentar_pendiente = TRUE;
}

void pantalla_olvidar_imagen() { imagen_actual = NULL; }

// se llama cada frame desde el bucle principal: cuando la pantalla nueva está lista, funde desde negro
void pantalla_presentar() {
    if (!presentar_pendiente) return;
    presentar_pendiente = FALSE;
    fx_fundido_entrada();
}

void texto_centrado(const char* txt, u16 y) {
    VDP_drawText(txt, (40 - strlen(txt)) / 2, y);
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
static void icono_item(u16 base, u16 item, u16 col, u16 fila) {
    u16 t = base + item * 4;
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL2, 1, 0, 0, t),     col,     fila);
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL2, 1, 0, 0, t + 1), col + 1, fila);
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL2, 1, 0, 0, t + 2), col,     fila + 1);
    VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL2, 1, 0, 0, t + 3), col + 1, fila + 1);
}

// Animación de las pantallas: sprites que andan o se mueven en el sitio, "PULSA START" parpadeando
// y un brillo que recorre el logo del título cada pocos segundos.
void pantalla_actualizar() {
    static u16 t = 0;
    t++;
    for (u16 i = 0; i < MAX_SPR_PANTALLA; i++) {
        Sprite* sp = spr_pant[i];
        if (!sp) continue;
        if ((t & 7) == 0) SPR_nextFrame(sp);
        if (spr_vx[i] != 0 && (t & 1)) {
            spr_x[i] += spr_vx[i];
            if (spr_x[i] > 336) spr_x[i] -= 400;
        }
        s16 dy = (sp->definition == &chick_sprite) ? (((t + i * 5) & 15) < 4 ? -2 : 0) : 0;
        SPR_setPosition(sp, spr_x[i], spr_y[i] + dy);
    }
    if (estado_actual == ESTADO_TITULO && imagen_actual == &bg_title && !PAL_isDoingFade()) {
        u16 f = t % 240;
        if (f < 12) {
            for (u16 c = 10; c <= 12; c++) {
                u16 o = bg_title.palette->data[c];
                PAL_setColor(c, (f >= 2 && f < 10) ? (o | 0x0444) : o);
            }
        }
    }
    if (titulo_parpadeo) {
        u16 fila = (estado_actual == ESTADO_TITULO) ? 17 : 24;
        if ((t & 31) == 0) texto_centrado(T("PULSA START", "PRESS START"), fila);
        else if ((t & 31) == 20) VDP_clearText(13, fila, 14);
    }
}

// ------------------------------------------------------------------ idioma
void mostrar_idioma() {
    pantalla_imagen(&bg_idioma);
    VDP_loadFont(&ts_font_menu, DMA);
    menu_paletas(&bg_idioma);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    menu_texto("IDIOMA / LANGUAGE", 11, 12, PAL1);
    menu_opcion("CASTELLANO", 13, 15, idioma == 0);
    menu_opcion("ENGLISH", 13, 17, idioma == 1);
    pantalla_sprite(&hero_sprite, 152, 160, PAL2, ANIM_ABAJO, 0, FALSE);
    musica_segun_estado(ESTADO_IDIOMA);
}

// ------------------------------------------------------------------ título
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

// ------------------------------------------------------------------ menú principal
void mostrar_menu_opciones() {
    char buf[40];
    pantalla_imagen(IMG(bg_opciones));
    VDP_loadFont(&ts_font_menu, DMA);
    menu_paletas(IMG(bg_opciones));
    musica_segun_estado(ESTADO_MENU);

    sprintf(buf, T("PARTIDAS %-5ld   RECORD %06ld", "GAMES %-5ld      RECORD %06ld"), total_partidas, puntuacion_maxima);
    menu_texto(buf, 3, 6, PAL3);
    if (victorias_j1 + victorias_j2 > 0) {
        sprintf(buf, T("ULTIMO VERSUS   J1 %ld - %ld J2", "LAST VERSUS     P1 %ld - %ld P2"), victorias_j1, victorias_j2);
        menu_texto(buf, 3, 7, PAL3);
    }

    menu_texto(T("JUGADORES", "PLAYERS"), 3, 9, PAL1);  menu_texto(T("IZQ/DER", "LEFT/RIGHT"), T(30, 27), 9, PAL3);
    menu_opcion(T("1 JUGADOR CONTRA LA CPU", "1 PLAYER VS THE CPU"), 4, 10, modo_jugadores == 0);
    menu_opcion(T("2 JUGADORES", "2 PLAYERS"), 4, 11, modo_jugadores == 1);

    menu_texto(T("MODO", "MODE"), 3, 13, PAL1);         menu_texto("A / B", 32, 13, PAL3);
    menu_opcion(T("HISTORIA  4 MUNDOS x 6 FASES", "STORY     4 WORLDS x 6 STAGES"), 4, 14, modo_juego_seleccionada == 0);
    menu_opcion(T("LIBRE     UN MAPA AL AZAR", "FREE      ONE RANDOM MAZE"), 4, 15, modo_juego_seleccionada == 1);

    menu_texto(T("DIFICULTAD", "DIFFICULTY"), 3, 18, PAL1); menu_texto(T("ARR/ABA", "UP/DOWN"), 30, 18, PAL3);
    menu_opcion(T("FACIL    2 MIN   5 POLLUELOS", "EASY     2 MIN   5 CHICKS"), 4, 19, d_sel == 0);
    menu_opcion(T("MEDIO    3 MIN  10 POLLUELOS", "NORMAL   3 MIN  10 CHICKS"), 4, 20, d_sel == 1);
    menu_opcion(T("DIFICIL  5 MIN  15 POLLUELOS", "HARD     5 MIN  15 CHICKS"), 4, 21, d_sel == 2);
    if (modo_jugadores == 1) menu_texto(T("2 JUGADORES: OPCIONES TRAS START", "2 PLAYERS: OPTIONS AFTER START"), T(4, 5), 23, PAL3);
    else menu_texto(T("C: PASSWORD (EMPEZAR EN UN JEFE)", "C: PASSWORD (START AT A BOSS)"), T(4, 5), 23, PAL3);

    menu_texto(T("START: JUGAR    MODE: IDIOMA", "START: PLAY    MODE: LANGUAGE"), T(7, 6), 25, PAL1);
}

// ------------------------------------------------------------------ cómo jugar
void mostrar_pantalla_como_jugar() {
    const Image* img = IMG(bg_como_jugar);
    pantalla_imagen(img);
    VDP_loadFont(&ts_font_menu, DMA);
    menu_paletas(img);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    u16 base = TILE_USER_INDEX + img->tileset->numTile;    // iconos de los objetos, justo detrás de la imagen
    VDP_loadTileSet(&ts_items, base, DMA);
    musica_segun_estado(ESTADO_COMO_JUGAR);

    icono_item(base, ITEM_POLLUELO, 2, 6);
    menu_texto(T("RECOGE LOS POLLUELOS Y LLEVALOS", "PICK UP THE CHICKS AND BRING"), 5, 6, PAL0);
    menu_texto(T("AL NIDO. CADA FASE DA UNA VIDA.", "THEM HOME. EACH STAGE = 1 LIFE."), 5, 7, PAL0);
    PAL_setPalette(PAL3, hunter_sprite.palette->data, CPU);
    pantalla_sprite(&hunter_sprite, 12, 70, PAL3, ANIM_ABAJO, 0, FALSE);
    menu_texto(T("SI EL CAZADOR TE PILLA, TUS", "IF THE HUNTER CATCHES YOU, YOUR"), 5, 9, PAL0);
    menu_texto(T("POLLUELOS ESCAPAN. SIN NINGUNO", "CHICKS RUN OFF. WITH NONE BEHIND"), 5, 10, PAL0);
    menu_texto(T("DETRAS, PIERDES UNA VIDA.", "YOU, YOU LOSE A LIFE."), 5, 11, PAL0);
    icono_item(base, ITEM_HUEVO, 2, 13);
    menu_texto(T("HUEVO: LANZALO CON A O B PARA", "EGG: THROW IT WITH A OR B TO"), 5, 13, PAL1);
    menu_texto(T("ATURDIR AL CAZADOR (Y SU RABIA).", "STUN THE HUNTER (AND HIS RAGE)."), 5, 14, PAL1);
    icono_item(base, ITEM_RELOJ, 2, 16);
    menu_texto(T("RELOJ: FRENA AL CAZADOR.", "CLOCK: SLOWS THE HUNTER DOWN."), 5, 16, PAL0);
    icono_item(base, ITEM_RAYO, 2, 18);
    menu_texto(T("RAYO: CORRES EL DOBLE.", "BOLT: YOU RUN TWICE AS FAST."), 5, 18, PAL0);
    icono_item(base, ITEM_GEMA, 2, 20);
    menu_texto(T("GEMA: 250 PUNTOS EXTRA.", "GEM: 250 BONUS POINTS."), 5, 20, PAL0);
    menu_texto(T("CADA MUNDO ACABA CON UN JEFE:", "EVERY WORLD ENDS WITH A BOSS:"), 5, 22, PAL1);
    menu_texto(T("VENCELO A HUEVAZOS!", "BEAT IT WITH EGGS!"), 5, 23, PAL1);
    menu_texto(T("PULSA START", "PRESS START"), 14, 25, PAL0);
}

// ------------------------------------------------------------------ versus
void mostrar_menu_versus() {
    char buf[40];
    pantalla_imagen(&bg_versus);
    VDP_loadFont(&ts_font_menu, DMA);
    menu_paletas(&bg_versus);
    musica_segun_estado(ESTADO_MENU_VERSUS);

    menu_opcion(modo_battle_royale ? T("BATTLE ROYALE      SI", "BATTLE ROYALE      ON") : T("BATTLE ROYALE      NO", "BATTLE ROYALE      OFF"), 5, 6, versus_sel == 0);
    static const u16 segs[4] = { 60, 90, 120, 180 };
    sprintf(buf, T("TIEMPO DE RONDA    %d SEG", "ROUND TIME         %d SEC"), segs[versus_tiempo_idx]);
    menu_opcion(buf, 5, 8, versus_sel == 1);
    sprintf(buf, T("AL MEJOR DE        %d", "BEST OF            %d"), versus_victorias_req);
    menu_opcion(buf, 5, 10, versus_sel == 2);
    const char* nombre;
    switch (versus_mundo_idx) {
        case 0: nombre = T("BOSQUE", "FOREST"); break;
        case 1: nombre = T("VOLCAN", "VOLCANO"); break;
        case 2: nombre = T("MAR", "SEA"); break;
        case 3: nombre = T("CIELO", "SKY"); break;
        default: nombre = T("AL AZAR", "RANDOM"); break;
    }
    sprintf(buf, T("ESCENARIO          %s", "STAGE              %s"), nombre);
    menu_opcion(buf, 5, 12, versus_sel == 3);
    menu_opcion(T("VOLVER AL MENU", "BACK TO MENU"), 5, 14, versus_sel == 4);
    menu_texto(T("RONDA 1:  J1 PAJARO  J2 CAZADOR", "ROUND 1:  P1 BIRD  P2 HUNTER"), 5, 17, PAL1);
    menu_texto(T("LOS PAPELES CAMBIAN CADA RONDA", "ROLES SWAP EVERY ROUND"), 5, 19, PAL3);
    menu_texto(T("PAJARO A/B: HUEVO  (ATURDE)", "BIRD A/B: EGG  (STUNS)"), 5, 21, PAL3);
    menu_texto(T("CAZADOR A/B: RED  (ENREDA)", "HUNTER A/B: NET  (TANGLES)"), 5, 23, PAL3);
    menu_texto(T("PULSA START PARA EMPEZAR", "PRESS START TO BEGIN"), T(8, 10), 25, PAL1);
}

// ------------------------------------------------------------------ intro de cada mundo
void mostrar_intro_mundo_actual() {
    const Image* img = IMG(bg_intro_bosque);
    if (mundo_actual == MUNDO_VOLCAN)       img = IMG(bg_intro_volcan);
    else if (mundo_actual == MUNDO_MARINO)  img = IMG(bg_intro_acuatico);
    else if (mundo_actual == MUNDO_CELESTE) img = IMG(bg_intro_celeste);
    const SpriteDefinition* enemigo = enemigo_del_mundo();
    pantalla_imagen(img);
    VDP_loadFont(&ts_font, DMA);
    VDP_setTextPalette(PAL0);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    PAL_setPalette(PAL3, enemigo->palette->data, CPU);
    pantalla_sprite(&hero_sprite, 104, 150, PAL2, ANIM_LADO, 0, FALSE);
    pantalla_sprite(&chick_sprite, 84, 150, PAL2, 0, 0, FALSE);
    pantalla_sprite(enemigo, 196, 142, PAL3, ANIM_LADO, 0, TRUE);
    musica_segun_estado(ESTADO_INTRO);
    titulo_parpadeo = TRUE;
}

// ------------------------------------------------------------------ historia (4 viñetas, texto a máquina)
static const char* const historia_es[4][2] = {
    { "EN LO MAS PROFUNDO DEL BOSQUE,",     "FLICKI CUIDABA DE SUS POLLUELOS." },
    { "PERO UNA NOCHE LLEGO EL CAZADOR...", "Y LOS POLLUELOS HUYERON ASUSTADOS!" },
    { "AHORA ESTAN PERDIDOS POR BOSQUES,",  "VOLCANES, MARES Y CIELOS." },
    { "AYUDA A FLICKI A LLEVARLOS A CASA",  "SIN QUE EL CAZADOR LOS ATRAPE!" },
};
static const char* const historia_en[4][2] = {
    { "DEEP IN THE FOREST, FLICKI LOOKED",  "AFTER HER LITTLE CHICKS." },
    { "BUT ONE NIGHT THE HUNTER CAME...",   "AND THE CHICKS FLED IN FRIGHT!" },
    { "NOW THEY ARE LOST IN FORESTS,",      "VOLCANOES, SEAS AND SKIES." },
    { "HELP FLICKI BRING THEM HOME",        "BEFORE THE HUNTER CATCHES THEM!" },
};
static u16 hist_panel, hist_t;
#define HIST_COL   3
#define HIST_FILA1 22
#define HIST_FILA2 24

static void historia_vineta(u16 n) {
    static const Image* const fondos[4] = { &bg_hist1, &bg_hist2, &bg_hist3, &bg_hist4 };
    hist_panel = n; hist_t = 0;
    pantalla_imagen(fondos[n]);
    VDP_loadFont(&ts_font_menu, DMA);
    VDP_setTextPalette(PAL0);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    PAL_setPalette(PAL3, hunter_sprite.palette->data, CPU);
    switch (n) {
        case 0:   // Flicki feliz junto al nido con sus polluelos
            pantalla_sprite(&hero_sprite, 108, 138, PAL2, ANIM_FELIZ, 0, FALSE);
            for (u16 i = 0; i < 3; i++) pantalla_sprite(&chick_sprite, 140 + i * 14, 140, PAL2, 0, 0, FALSE);
            break;
        case 1:   // llega el cazador y los polluelos salen corriendo
            pantalla_sprite(&hero_sprite, 108, 138, PAL2, ANIM_HERIDO, 0, FALSE);
            pantalla_sprite(&hunter_sprite, 330, 130, PAL3, ANIM_LADO, -1, TRUE);
            for (u16 i = 0; i < 3; i++) pantalla_sprite(&chick_sprite, 150 + i * 14, 140, PAL2, 0, 2, FALSE);
            break;
        case 2:   // Flicki recorre el mapa de los cuatro mundos
            pantalla_sprite(&hero_sprite, 40, 100, PAL2, ANIM_LADO, 1, FALSE);
            break;
        default:  // ¡a por ellos!
            pantalla_sprite(&hero_sprite, 152, 120, PAL2, ANIM_FELIZ, 0, FALSE);
            pantalla_sprite(&chick_sprite, 120, 126, PAL2, 0, 0, FALSE);
            pantalla_sprite(&chick_sprite, 188, 126, PAL2, 0, 0, FALSE);
            break;
    }
}

void mostrar_historia(bool desde_menu) {
    historia_desde_menu = desde_menu;
    estado_actual = ESTADO_HISTORIA;
    musica_segun_estado(ESTADO_HISTORIA);
    historia_vineta(0);
}

// devuelve TRUE cuando la historia ha terminado
bool historia_actualizar() {
    const char* const* lineas = idioma ? historia_en[hist_panel] : historia_es[hist_panel];
    hist_t++;
    u16 n0 = strlen(lineas[0]), n1 = strlen(lineas[1]);
    u16 c = hist_t / 2;                                   // una letra cada 2 frames
    if ((hist_t & 1) == 0 && c >= 1 && c <= n0 + n1) {
        char ch[2] = { 0, 0 };
        if (c <= n0) { ch[0] = lineas[0][c - 1]; VDP_drawText(ch, HIST_COL + c - 1, HIST_FILA1); }
        else { ch[0] = lineas[1][c - n0 - 1]; VDP_drawText(ch, HIST_COL + c - n0 - 1, HIST_FILA2); }
        if (ch[0] != ' ' && (c & 3) == 0) sonido(SFX_CLICK);
    }
    if (hist_t > (n0 + n1) * 2 + 200) {
        if (hist_panel < 3) historia_vineta(hist_panel + 1);
        else return TRUE;
    }
    return FALSE;
}

// ------------------------------------------------------------------ desenlaces
void mostrar_pantalla_continue() {
    finalizar_partida();
    estado_actual = ESTADO_CONTINUE;
    pantalla_imagen(&bg_continue);
    VDP_loadFont(&ts_font, DMA);
    VDP_setTextPalette(PAL0);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    char buf[40];
    if (vidas == 1) strcpy(buf, T("TE QUEDA 1 VIDA", "1 LIFE LEFT"));
    else sprintf(buf, T("TE QUEDAN %d VIDAS", "%d LIVES LEFT"), vidas);
    texto_centrado(buf, 12);
    if (password_mundo >= 0) {
        sprintf(buf, "PASSWORD: %s", password_de(password_mundo, d_sel));
        texto_centrado(buf, 14);
    }
    pantalla_sprite(&hero_sprite, 152, 160, PAL2, ANIM_HERIDO, 0, FALSE);
    musica_segun_estado(ESTADO_CONTINUE);
    titulo_parpadeo = TRUE;
}

void mostrar_game_over() {
    finalizar_partida();
    estado_actual = ESTADO_GAMEOVER;
    pantalla_imagen(&bg_game_over);
    VDP_loadFont(&ts_font, DMA);
    VDP_setTextPalette(PAL0);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    PAL_setPalette(PAL3, hunter_sprite.palette->data, CPU);
    char buf[40];
    sprintf(buf, T("PUNTOS %06ld   RECORD %06ld", "SCORE %06ld   RECORD %06ld"), flicky.puntuacion, puntuacion_maxima);
    texto_centrado(buf, 12);
    if (password_mundo >= 0) {
        sprintf(buf, T("PASSWORD DEL JEFE: %s", "BOSS PASSWORD: %s"), password_de(password_mundo, d_sel));
        texto_centrado(buf, 14);
    }
    pantalla_sprite(&hunter_sprite, 176, 152, PAL3, ANIM_LADO, 0, TRUE);
    pantalla_sprite(&hero_sprite, 136, 160, PAL2, ANIM_HERIDO, 0, FALSE);
    musica_segun_estado(ESTADO_GAMEOVER);
    titulo_parpadeo = TRUE;
}

void mostrar_enhorabuena() {
    estado_actual = ESTADO_FINJUEGO;
    pantalla_imagen(IMG(bg_fin_juego));
    VDP_loadFont(&ts_font, DMA);
    VDP_setTextPalette(PAL0);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    char buf[40];
    sprintf(buf, T("PUNTOS %06ld", "SCORE %06ld"), flicky.puntuacion);
    texto_centrado(buf, 15);
    pantalla_sprite(&hero_sprite, 152, 150, PAL2, ANIM_FELIZ, 0, FALSE);
    for (u16 i = 0; i < 4; i++) pantalla_sprite(&chick_sprite, 104 + i * 32 + (i >= 2 ? 16 : 0), 158, PAL2, 0, 0, FALSE);
    musica_segun_estado(ESTADO_FINJUEGO);
    titulo_parpadeo = TRUE;
}

void mostrar_ganador_versus() {
    PAL_setPalette(PAL2, pal_items.data, CPU);
    if (ganador_fue_pajaro) {                          // la imagen del papel con el que ganó el último punto
        pantalla_imagen(IMG(bg_pajaro_ganador));
        pantalla_sprite(&hero_sprite, 152, 150, PAL2, ANIM_FELIZ, 0, FALSE);
    } else {
        pantalla_imagen(IMG(bg_cazador_ganador));
        const SpriteDefinition* def = enemigo_del_mundo();   // el enemigo del último escenario
        PAL_setPalette(PAL3, def->palette->data, CPU);
        pantalla_sprite(def, 148, 144, PAL3, ANIM_ABAJO, 0, FALSE);
    }
    PAL_setPalette(PAL2, pal_items.data, CPU);
    VDP_loadFont(&ts_font, DMA);
    VDP_setTextPalette(PAL0);
    char buf[40];
    bool gana_j1 = victorias_j1 > victorias_j2;
    sprintf(buf, T("GANA EL JUGADOR %d", "PLAYER %d WINS"), gana_j1 ? 1 : 2);
    texto_centrado(buf, 11);
    sprintf(buf, T("J1  %ld - %ld  J2", "P1  %ld - %ld  P2"), victorias_j1, victorias_j2);
    texto_centrado(buf, 13);
    musica_segun_estado(ESTADO_FINJUEGO);
    titulo_parpadeo = TRUE;
}

// ------------------------------------------------------------------ password
static char clave[5] = "AAAA";
static u16 clave_pos = 0;
static s16 clave_error = 0;

static void password_dibujar() {
    char buf[2] = { 0, 0 };
    for (u16 i = 0; i < 4; i++) {
        buf[0] = clave[i];
        menu_texto(buf, 16 + i * 2, 13, i == clave_pos ? PAL1 : PAL0);
        menu_texto(i == clave_pos ? "^" : " ", 16 + i * 2, 14, PAL1);
    }
    menu_texto(clave_error ? T("PASSWORD INCORRECTO", "WRONG PASSWORD") : "                   ", T(11, 13), 17, PAL3);
}

void mostrar_password() {
    estado_actual = ESTADO_PASSWORD;
    pantalla_imagen(IMG(bg_opciones));
    VDP_loadFont(&ts_font_menu, DMA);
    menu_paletas(IMG(bg_opciones));
    musica_segun_estado(ESTADO_MENU);
    clave_pos = 0; clave_error = 0;
    menu_texto("PASSWORD", 16, 8, PAL1);
    menu_texto(T("CONTINUA DESDE EL JEFE DE UN MUNDO", "CONTINUE FROM A WORLD BOSS"), T(3, 7), 10, PAL3);
    menu_texto(T("ARR/ABA: LETRA   IZQ/DER: MOVER", "UP/DOWN: LETTER   LEFT/RIGHT: MOVE"), T(4, 3), 21, PAL3);
    menu_texto(T("START: ACEPTAR    B: VOLVER", "START: OK          B: BACK"), T(6, 6), 23, PAL1);
    password_dibujar();
}

void password_entrada(u16 pulsado) {
    if (pulsado & BUTTON_UP)         clave[clave_pos] = (clave[clave_pos] == 'Z') ? 'A' : clave[clave_pos] + 1;
    else if (pulsado & BUTTON_DOWN)  clave[clave_pos] = (clave[clave_pos] == 'A') ? 'Z' : clave[clave_pos] - 1;
    else if (pulsado & BUTTON_LEFT)  clave_pos = (clave_pos + 3) & 3;
    else if (pulsado & (BUTTON_RIGHT | BUTTON_A | BUTTON_C)) clave_pos = (clave_pos + 1) & 3;
    else if (pulsado & BUTTON_B)     { sonido(SFX_CLICK); estado_actual = ESTADO_MENU; mostrar_menu_opciones(); return; }
    else if (pulsado & BUTTON_START) {
        u16 m, d;
        if (password_validar(clave, &m, &d)) {
            sonido(SFX_META);
            // empieza la partida en el jefe de ese mundo
            total_partidas++;
            modo_jugadores = 0; modo_juego_seleccionada = 0;
            d_sel = d; mundo_actual = m; fase_actual = FASE_JEFE;
            vidas = 2; municion = 2;
            flicky.puntuacion = 0; proximo_hito_vida = PUNTOS_VIDA_EXTRA;
            max_polluelos_nivel = (d_sel == 0) ? 5 : ((d_sel == 1) ? 10 : 15);
            password_mundo = m;
            estado_actual = ESTADO_INTRO;
            mostrar_intro_mundo_actual();
            return;
        }
        sonido(SFX_ATRAPADO);
        clave_error = 1;
        password_dibujar();
        return;
    } else return;
    sonido(SFX_CLICK);
    clave_error = 0;
    password_dibujar();
}
