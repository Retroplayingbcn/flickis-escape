// Flicki's Escape - Mega Drive (SGDK)
// Bucle principal: máquina de estados de pantallas y lectura del mando (por flancos, sin esperas)
#include "juego.h"

u16 idioma = 0;

static void empezar_partida() {
    total_partidas++;
    fase_actual = 1;
    mundo_actual = MUNDO_BOSQUE;
    vidas = 1;
    proximo_hito_vida = PUNTOS_VIDA_EXTRA;
    password_mundo = -1;
    flicky.puntuacion = 0;
    municion = 0;
    victorias_j1 = 0;
    victorias_j2 = 0;
    max_polluelos_nivel = (d_sel == 0) ? 5 : ((d_sel == 1) ? 10 : 15);
}

static void ir_al_menu() {
    estado_actual = ESTADO_MENU;
    mostrar_menu_opciones();
}

static void ir_a_como_jugar() {
    estado_actual = ESTADO_COMO_JUGAR;
    mostrar_pantalla_como_jugar();
}

static void menu_principal(u16 pulsado) {
    bool cambio = TRUE;
    if (pulsado & BUTTON_UP)         { if (d_sel > 0) d_sel--; else cambio = FALSE; }
    else if (pulsado & BUTTON_DOWN)  { if (d_sel < 2) d_sel++; else cambio = FALSE; }
    else if (pulsado & BUTTON_LEFT)  { if (modo_jugadores > 0) modo_jugadores--; else cambio = FALSE; }
    else if (pulsado & BUTTON_RIGHT) { if (modo_jugadores < 1) modo_jugadores++; else cambio = FALSE; }
    else if (pulsado & BUTTON_A)     modo_juego_seleccionada = 0;
    else if (pulsado & BUTTON_B)     modo_juego_seleccionada = 1;
    else if (pulsado & BUTTON_MODE)  { idioma ^= 1; guardar_config(); pantalla_olvidar_imagen(); }
    else if ((pulsado & BUTTON_C) && modo_jugadores == 0) { sonido(SFX_CLICK); mostrar_password(); return; }
    else if (pulsado & BUTTON_START) {
        sonido(SFX_CLICK);
        empezar_partida();
        if (modo_jugadores == 1) {
            estado_actual = ESTADO_MENU_VERSUS;
            versus_sel = 0;
            mostrar_menu_versus();
        } else if (modo_juego_seleccionada == 0) {
            mostrar_historia(TRUE);                    // en modo historia, la historia va antes de jugar
        } else {
            mundo_actual = random() % 4;
            ir_a_como_jugar();
        }
        return;
    } else cambio = FALSE;
    if (cambio) { sonido(SFX_CLICK); mostrar_menu_opciones(); }
}

static void menu_versus(u16 pulsado) {
    if (pulsado & BUTTON_UP)        { if (versus_sel > 0) versus_sel--; }
    else if (pulsado & BUTTON_DOWN) { if (versus_sel < 5) versus_sel++; }
    else if (pulsado & (BUTTON_LEFT | BUTTON_A | BUTTON_RIGHT | BUTTON_B)) {
        bool mas = (pulsado & (BUTTON_RIGHT | BUTTON_B)) != 0;
        switch (versus_sel) {
            case 0: modo_battle_royale = !modo_battle_royale; break;
            case 1: versus_papeles = mas ? (versus_papeles + 1) % 3 : (versus_papeles + 2) % 3; break;
            case 2: if (mas) { if (versus_tiempo_idx < 3) versus_tiempo_idx++; } else if (versus_tiempo_idx > 0) versus_tiempo_idx--; break;
            case 3: if (mas) { if (versus_victorias_req < 5) versus_victorias_req += 2; } else if (versus_victorias_req > 1) versus_victorias_req -= 2; break;
            case 4: versus_mundo_idx = mas ? (versus_mundo_idx + 1) % 5 : (versus_mundo_idx + 4) % 5; break;
            case 5: sonido(SFX_CLICK); ir_al_menu(); return;
        }
    } else if (pulsado & BUTTON_START) {
        sonido(SFX_CLICK);
        if (versus_sel == 5) { ir_al_menu(); return; }
        victorias_j1 = 0; victorias_j2 = 0;
        versus_ronda = 1; p1_es_pajaro = (versus_papeles != 1);
        resetear_tiempo_nivel();
        max_polluelos_nivel = 10;
        mundo_actual = (versus_mundo_idx == 4) ? (random() % 4) : versus_mundo_idx;
        inicializar_juego();
        return;
    } else return;
    sonido(SFX_CLICK);
    mostrar_menu_versus();
}

int main(bool hard) {
    VDP_init();
    SYS_disableInts();
    VDP_setScreenWidth320();
    VDP_setScreenHeight224();
    SYS_enableInts();
    SPR_initEx(520);                                    // memoria de sprites: el jefe mide 48x48

    // driver XGM2: todas las canciones en un único bloque (comparten muestras PCM)
    XGM2_loadDriver(TRUE);
    XGM2_load(musica);
    JOY_init();
    VDP_loadFont(&ts_font, DMA);

    cargar_config();
    estado_actual = ESTADO_IDIOMA;
    mostrar_idioma();

    u16 joy_prev = JOY_readJoypad(JOY_1);
    u16 titulo_espera = 0;

    while (1) {
        u16 joy = JOY_readJoypad(JOY_1);
        u16 pulsado = joy & ~joy_prev;                  // solo lo recién pulsado
        joy_prev = joy;

        switch (estado_actual) {
            case ESTADO_IDIOMA:
                if (pulsado & (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)) { idioma ^= 1; sonido(SFX_CLICK); mostrar_idioma(); }
                else if (pulsado & (BUTTON_START | BUTTON_A | BUTTON_C)) {
                    sonido(SFX_CLICK); guardar_config();
                    estado_actual = ESTADO_TITULO; titulo_espera = 0; mostrar_pantalla_titulo();
                }
                break;

            case ESTADO_HISTORIA:
                if (historia_actualizar() || (pulsado & BUTTON_START)) {
                    if (historia_desde_menu) ir_a_como_jugar();
                    else { estado_actual = ESTADO_TITULO; titulo_espera = 0; mostrar_pantalla_titulo(); }
                }
                break;

            case ESTADO_TITULO:
                // modo "attract": si nadie toca el mando un rato, se cuenta la historia
                if (joy) titulo_espera = 0;
                else if (++titulo_espera > 1500) { titulo_espera = 0; mostrar_historia(FALSE); break; }
                if (pulsado & BUTTON_START) { sonido(SFX_CLICK); ir_al_menu(); }
                break;

            case ESTADO_MENU:        menu_principal(pulsado); break;
            case ESTADO_MENU_VERSUS: menu_versus(pulsado); break;
            case ESTADO_PASSWORD:    password_entrada(pulsado); break;

            case ESTADO_COMO_JUGAR:
                if (pulsado & BUTTON_START) { sonido(SFX_CLICK); estado_actual = ESTADO_INTRO; mostrar_intro_mundo_actual(); }
                break;

            case ESTADO_INTRO:
                if (pulsado & BUTTON_START) { sonido(SFX_CLICK); resetear_tiempo_nivel(); inicializar_juego(); }
                break;

            case ESTADO_CONTINUE:
                if (pulsado & BUTTON_START) { sonido(SFX_CLICK); vidas--; resetear_tiempo_nivel(); inicializar_juego(); }
                break;

            case ESTADO_GAMEOVER:
            case ESTADO_FINJUEGO:
                if (pulsado & BUTTON_START) { sonido(SFX_CLICK); ir_al_menu(); }
                break;

            case ESTADO_JUEGO: {
                static u16 joy2_prev = 0;
                u16 joy2 = (modo_jugadores == 1) ? JOY_readJoypad(JOY_2) : 0;
                u16 pulsado2 = joy2 & ~joy2_prev;
                joy2_prev = joy2;
                if ((pulsado | pulsado2) & BUTTON_START) {
                    sonido(SFX_CLICK);
                    juego_pausado = !juego_pausado;
                    if (juego_pausado) { hud_mensaje(T("PAUSA", "PAUSE")); musica_pausar(); }
                    else { hud_forzar_redibujo(); musica_reanudar(); }
                }
                if (!juego_pausado) {
                    if (modo_jugadores == 1) {
                        // los papeles se alternan: cada ronda un mando lleva al pájaro
                        if (p1_es_pajaro) { jugador_entrada(joy, pulsado); cazador_entrada_humana(joy2, pulsado2); }
                        else { jugador_entrada(joy2, pulsado2); cazador_entrada_humana(joy, pulsado); }
                    } else jugador_entrada(joy, pulsado);
                    actualizar_logica();
                }
                break;
            }
        }

        if (estado_actual != ESTADO_JUEGO) pantalla_actualizar();
        pantalla_presentar();
        SPR_update();
        SYS_doVBlankProcess();
    }
    return 0;
}
