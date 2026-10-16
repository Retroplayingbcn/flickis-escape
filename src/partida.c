// Partida: estado global, inicio y final de cada ronda, lógica por frame, muertes y fases superadas
#include "juego.h"

Pajaro flicky;
Cazador cazador;
u8 estado_actual;

u16 d_sel = 1, max_polluelos_nivel = 10, semillas_en_fila = 0, total_salvados = 0, municion = 0;
s32 tiempo_restante_segundos = 180;
u32 frame_counter = 0;
s16 timer_cazador_lento = 0, timer_flicky_rapido = 0, timer_spawn_powerup = 0;
s16 vidas = 1;
u32 puntuacion_maxima = 0, proximo_hito_vida = PUNTOS_VIDA_EXTRA;
bool juego_pausado = FALSE, cazador_en_furia = FALSE, flicky_invulnerable = FALSE, salida_abierta = FALSE, en_jefe = FALSE;
s16 timer_duracion_furia = 0, timer_invulnerabilidad = 0;
u16 modo_juego_seleccionada = 0, mundo_actual = MUNDO_BOSQUE, fase_actual = 1, modo_jugadores = 0;
u32 total_partidas = 0, victorias_j1 = 0, victorias_j2 = 0;
u16 multiplicador_velocidad_minuto = 0;
bool modo_battle_royale = FALSE, en_preaviso_cierre = FALSE;
u16 anillo_recorte = 1;
s16 password_mundo = -1;
bool p1_es_pajaro = TRUE, ganador_fue_pajaro = TRUE;
u16 versus_ronda = 1;
u16 versus_papeles = 0;
s16 timer_enredado = 0, timer_flicky_lento = 0, timer_cazador_rapido = 0;
u16 versus_sel = 0, versus_tiempo_idx = 2, versus_victorias_req = 3, versus_mundo_idx = 4;
bool historia_desde_menu = FALSE;

static s16 tiempo_inicial = 180;

void resetear_tiempo_nivel() {
    static const s16 t_versus[4] = { 60, 90, 120, 180 };
    static const s16 t_dif[3] = { 120, 180, 300 };
    if (modo_jugadores == 1) tiempo_restante_segundos = t_versus[versus_tiempo_idx];
    else tiempo_restante_segundos = t_dif[d_sel];
    tiempo_inicial = tiempo_restante_segundos;
}

// ------------------------------------------------------------------ inicio y final de ronda
void inicializar_juego() {
    fx_fundido_salida();
    pantalla_sprites_reset();
    pantalla_olvidar_imagen();
    titulo_parpadeo = FALSE;
    VDP_setEnable(FALSE);
    VDP_clearPlane(BG_A, TRUE);
    VDP_clearPlane(BG_B, TRUE);
    fx_scroll_juego(TRUE);

    en_jefe = (modo_jugadores == 0 && modo_juego_seleccionada == 0 && fase_actual == FASE_JEFE);
    estado_actual = ESTADO_JUEGO;
    juego_pausado = FALSE;
    cargar_graficos_mundo();

    total_salvados = 0; semillas_en_fila = 0; frame_counter = 0;
    multiplicador_velocidad_minuto = 0;
    timer_cazador_lento = 0; timer_flicky_rapido = 0; timer_spawn_powerup = 150;
    anillo_recorte = 1; en_preaviso_cierre = FALSE; salida_abierta = FALSE;   // el anillo 1 es el borde exterior
    timer_enredado = 0; timer_flicky_lento = 0; timer_cazador_rapido = 0;
    cazador_en_furia = FALSE; timer_duracion_furia = 0;
    flicky_invulnerable = FALSE; timer_invulnerabilidad = 0;
    if (modo_jugadores == 1) municion = 1;              // en versus, cada ronda empieza igual
    else if (municion < 1) municion = 1;                // cada ronda empieza al menos con un huevo
    if (en_jefe) {
        tiempo_restante_segundos = 120 + (2 - d_sel) * 30; tiempo_inicial = tiempo_restante_segundos;
        if (municion < 2) municion = 2;
        password_mundo = mundo_actual;                  // desde aquí, el password de este jefe
    }
    corazones = 0;

    if (en_jefe) generar_arena(); else generar_mapa_aleatorio();
    calcular_alcanzables();
    if (!en_jefe) repartir_polluelos_seguros();
    objetos_reset();
    ambiente_reset();
    dibujar_mapa_completo();
    hud_forzar_redibujo();

    if (en_jefe) {
        jugador_crear(2, 6);
        cazador.sprite = NULL; cazador.def = NULL;
        jefe_iniciar();
    } else {
        jugador_crear(1, 1);
        s16 cx = MAP_WIDTH - 2, cy = MAP_HEIGHT - 2;
        buscar_celda_libre(&cx, &cy, 10);
        cazador_crear(cx, cy);
    }
    hud_actualizar();
    SPR_update();
    fx_fundido_entrada();
    musica_segun_estado(ESTADO_JUEGO);
    preparados();
}

void finalizar_partida() {
    muros_alerta(FALSE);
    if (flicky.sprite)  { SPR_releaseSprite(flicky.sprite);  flicky.sprite = NULL; }
    if (cazador.sprite) { SPR_releaseSprite(cazador.sprite); cazador.sprite = NULL; }
    cazador_liberar();
    jefe_reset();
    ambiente_reset();
    polluelos_reset();
    proyectiles_reset();
    fx_reset();
    pantalla_sprites_reset();
    flicky_invulnerable = FALSE;
    fx_scroll_juego(FALSE);
    SPR_update();
    SYS_doVBlankProcess();
}

// "¡Preparados!": el jugador ve el laberinto antes de que nada se mueva
void preparados() {
    hud_actualizar();
    char buf[32];
    u16 total = 110;
    if (modo_jugadores == 1) {
        // versus: número de ronda y quién lleva al pájaro
        sprintf(buf, T("RONDA %d", "ROUND %d"), versus_ronda);
        banner(buf);
        total = 190;
    } else if (en_jefe) {
        // los polluelos dan su fuerza a Flicki, y el jefe cae del cielo
        banner(T("TUS POLLUELOS TE DAN FUERZA!", "YOUR CHICKS GIVE YOU POWER!"));
        polluelos_poder();
        dibujar_mapa_completo();
        banner(jefe_nombre());
        for (u16 f = 0; f < 16; f++) { fx_scroll_actualizar(); SPR_update(); SYS_doVBlankProcess(); }
        while (!jefe_caer()) { fx_actualizar(); fx_scroll_actualizar(); SPR_update(); SYS_doVBlankProcess(); }
        total = 64;
    } else banner(T("PREPARADOS...", "GET READY..."));
    for (u16 f = 0; f < total; f++) {
        fx_actualizar();
        if (modo_jugadores == 1 && f == 70) {
            dibujar_mapa_completo();
            sprintf(buf, T("PAJARO: J%d   CAZADOR: J%d", "BIRD: P%d   HUNTER: P%d"), p1_es_pajaro ? 1 : 2, p1_es_pajaro ? 2 : 1);
            banner(buf);
        }
        if (f == total - 30) { dibujar_mapa_completo(); banner(T("YA!", "GO!")); }
        fx_scroll_actualizar();
        SPR_update();
        SYS_doVBlankProcess();
    }
    dibujar_mapa_completo();
    hud_forzar_redibujo();
    hud_actualizar();
}

// ------------------------------------------------------------------ desenlaces
static void guardar_record() {
    if (flicky.puntuacion > puntuacion_maxima) { puntuacion_maxima = flicky.puntuacion; guardar_config(); }
}

void jugador_muere(const char* motivo) {
    fx_estallido(flicky.x, flicky.y);
    fx_destello();
    efecto_shake(20, 3);
    banner(motivo);
    musica_detener();
    sonido(SFX_ATRAPADO);
    SPR_setVisibility(flicky.sprite, VISIBLE);
    SPR_setAnim(flicky.sprite, ANIM_HERIDO); flicky.anim = ANIM_HERIDO; SPR_setHFlip(flicky.sprite, FALSE);
    for (u16 f = 0; f < 110; f++) {
        SPR_setFrame(flicky.sprite, (f >> 3) & 3);
        SPR_setPosition(flicky.sprite, FLICKY_SPR_X + shake_x, FLICKY_SPR_Y + PLAY_Y);
        fx_actualizar();
        fx_scroll_actualizar();
        SPR_update();
        SYS_doVBlankProcess();
    }
    if (vidas > 0) mostrar_pantalla_continue();
    else { guardar_record(); mostrar_game_over(); }
}

void fase_superada(bool fue_jefe) {
    muros_alerta(FALSE);
    cazador_en_furia = FALSE; timer_cazador_lento = 0;
    if (!fue_jefe) cazador_tinte_actualizar(TRUE);
    proyectiles_reset();
    musica_detener();
    musica_reiniciar(PISTA_SUPERADA);
    banner(fue_jefe ? T("MUNDO SUPERADO!", "WORLD CLEAR!") : T("FASE SUPERADA!", "STAGE CLEAR!"));
    ganar_vida();                                       // cada fase completada da una vida
    secuencia_heroe(ANIM_FELIZ, 110, TRUE);
    {   // bonus por el tiempo que sobra
        char buf[32];
        u32 bonus = tiempo_restante_segundos * (fue_jefe ? 20 : 10);
        sprintf(buf, T("BONUS TIEMPO %ld", "TIME BONUS %ld"), bonus);
        dibujar_mapa_completo();
        banner(buf);
        sumar_puntos(bonus);
        secuencia_heroe(ANIM_FELIZ, 80, FALSE);
    }
    if (modo_juego_seleccionada == 0 && fase_actual + 1 == FASE_JEFE) {
        // antes del jefe, el password para poder volver aquí
        char buf[32];
        sprintf(buf, T("PASSWORD DEL JEFE: %s", "BOSS PASSWORD: %s"), password_de(mundo_actual, d_sel));
        dibujar_mapa_completo();
        // se queda en pantalla hasta pulsar START, para que dé tiempo a apuntarlo
        u16 prev = JOY_readJoypad(JOY_1);
        for (u16 f = 0; ; f++) {
            if ((f & 31) == 0) banner_doble(buf, T("PULSA START", "PRESS START"));
            else if ((f & 31) == 20) banner_doble(buf, "");
            u16 j = JOY_readJoypad(JOY_1);
            if ((j & ~prev) & BUTTON_START) break;
            prev = j;
            SPR_setFrame(flicky.sprite, (f >> 3) & 3);
            fx_actualizar(); fx_scroll_actualizar(); SPR_update(); SYS_doVBlankProcess();
        }
        sonido(SFX_CLICK);
    }
    if (modo_juego_seleccionada == 1) {                 // modo libre: una sola fase
        banner(T("HAS ESCAPADO CON EXITO!", "YOU ESCAPED!"));
        secuencia_heroe(ANIM_FELIZ, 60, FALSE);
        guardar_record();
        finalizar_partida();
        estado_actual = ESTADO_MENU;
        mostrar_menu_opciones();
        return;
    }
    fase_actual++;
    if (fase_actual > FASE_JEFE) { fase_actual = 1; mundo_actual++; }
    finalizar_partida();
    if (mundo_actual > MUNDO_CELESTE) { guardar_record(); mostrar_enhorabuena(); return; }
    if (fase_actual == 1) {
        estado_actual = ESTADO_INTRO;
        mostrar_intro_mundo_actual();
    } else {
        resetear_tiempo_nivel();
        inicializar_juego();
    }
}

// gano_pajaro: quién ganó la ronda por papel; el punto va al jugador que lo llevaba
void verificar_fin_ronda_versus(bool gano_pajaro) {
    bool gana_j1 = (gano_pajaro == p1_es_pajaro);
    if (gana_j1) victorias_j1++; else victorias_j2++;
    ganador_fue_pajaro = gano_pajaro;
    fx_estallido(gano_pajaro ? cazador.x : flicky.x, gano_pajaro ? cazador.y : flicky.y);
    efecto_shake(16, 3);
    musica_detener();
    sonido(gano_pajaro ? SFX_META : SFX_ATRAPADO);
    muros_alerta(FALSE);
    SPR_setVisibility(cazador.sprite, VISIBLE);
    char buf[32];
    banner(gana_j1 ? T("PUNTO PARA J1!", "POINT TO P1!") : T("PUNTO PARA J2!", "POINT TO P2!"));
    for (u16 f = 0; f < 150; f++) {
        if (f == 75) {
            dibujar_mapa_completo();
            sprintf(buf, T("J1  %d - %d  J2", "P1  %d - %d  P2"), (u16)victorias_j1, (u16)victorias_j2);
            banner(buf);
        }
        fx_actualizar(); fx_scroll_actualizar(); SPR_update(); SYS_doVBlankProcess();
    }
    finalizar_partida();
    if (victorias_j1 >= VICTORIAS_PARA_GANAR || victorias_j2 >= VICTORIAS_PARA_GANAR) {
        estado_actual = ESTADO_FINJUEGO;
        mostrar_ganador_versus();
        return;
    }
    versus_ronda++;
    if (versus_papeles == 2) p1_es_pajaro = !p1_es_pajaro;   // si se eligió alternar, se cambian los papeles
    resetear_tiempo_nivel();
    if (versus_mundo_idx == 4) mundo_actual = random() % 4;
    inicializar_juego();
}

// ------------------------------------------------------------------ battle royale: el laberinto se cierra
// El anillo 1 es el borde exterior; cada cierre tapia el siguiente anillo (2, 3 y 4).
// Los polluelos que pilla el muro saltan hacia dentro, y se recalcula qué queda conectado
// para que nada (polluelos, jugadores, objetos) se quede encerrado en un hueco sin salida.
static bool fuera_del_anillo(s16 x, s16 y) {
    return x < anillo_recorte || x >= MAP_WIDTH - anillo_recorte || y < anillo_recorte || y >= MAP_HEIGHT - anillo_recorte;
}

static void aplicar_recorte_battle_royale() {
    anillo_recorte++;
    en_preaviso_cierre = FALSE;
    muros_alerta(FALSE);
    calcular_distancias(NIDO_X, NIDO_Y);
    for (s16 y = 0; y < MAP_HEIGHT; y++) {
        for (s16 x = 0; x < MAP_WIDTH; x++) {
            if (!fuera_del_anillo(x, y)) continue;
            u8 t = mapa[y][x];
            mapa[y][x] = TILE_MURO;
            deco[y][x] = 0;
            if (t == TILE_POLLUELO) polluelo_volar(x * CELDA, y * CELDA);
        }
    }
    // lo que haya quedado aislado del nido también se rescata
    calcular_alcanzables();
    calcular_distancias(NIDO_X, NIDO_Y);
    for (s16 y = 1; y < MAP_HEIGHT - 1; y++) {
        for (s16 x = 1; x < MAP_WIDTH - 1; x++) {
            if (alcanzable[y][x]) continue;
            if (mapa[y][x] == TILE_POLLUELO) { mapa[y][x] = TILE_SUELO; polluelo_volar(x * CELDA, y * CELDA); }
            else if (mapa[y][x] != TILE_MURO) mapa[y][x] = TILE_SUELO;
        }
    }
    dibujar_mapa_completo();
    sonido(SFX_CIERRE);
    efecto_shake(12, 2);
    // si el cierre atrapa a alguien, se le lleva al pasillo libre más cercano
    s16 cx, cy;
    s16 fx = (flicky.x + 8) >> CELDA_SHIFT, fy = (flicky.y + 8) >> CELDA_SHIFT;
    if ((hay_muro(fx, fy) || !alcanzable[fy][fx]) && buscar_celda_libre(&cx, &cy, 0)) {
        fx_polvo(flicky.x, flicky.y);
        flicky.x = cx * CELDA; flicky.y = cy * CELDA; flicky.x_vel = flicky.y_vel = 0;
    }
    s16 hx = (cazador.x + 8) >> CELDA_SHIFT, hy = (cazador.y + 8) >> CELDA_SHIFT;
    if (cazador.sprite && (hay_muro(hx, hy) || !alcanzable[hy][hx]) && buscar_celda_libre(&cx, &cy, 4)) {
        fx_polvo(cazador.x, cazador.y);
        cazador.x = cx * CELDA; cazador.y = cy * CELDA; cazador.x_vel = cazador.y_vel = 0;
    }
}

// ------------------------------------------------------------------ entrega en el nido
static bool en_nido() {
    s16 cx = (flicky.x + 8) >> CELDA_SHIFT, cy = (flicky.y + 8) >> CELDA_SHIFT;
    return cy == NIDO_Y && cx >= NIDO_X && cx < NIDO_X + NIDO_ANCHO;
}

static void entregar_polluelos() {
    u16 c = semillas_en_fila;
    for (u16 i = 0; i < c; i++) {
        semillas_en_fila--;
        if (sprites_seguidores[semillas_en_fila]) { SPR_releaseSprite(sprites_seguidores[semillas_en_fila]); sprites_seguidores[semillas_en_fila] = NULL; }
        total_salvados++;
        // cadena: cada polluelo de la misma entrega vale más (100, 200, 300...)
        u16 n = (i < 9) ? i : 9;
        sumar_puntos(100 * (n + 1));
        fx_popup(NIDO_X * CELDA + 8, NIDO_Y * CELDA - 8 - (i & 1) * 6, POP_100 + n);
        fx_chispa((NIDO_X * CELDA) + ((i & 1) ? 16 : 0), NIDO_Y * CELDA - 4);
        sonido_nido(i);
        dibujar_nido();
        hud_actualizar();
        for (u16 d = 0; d < 7; d++) { fx_actualizar(); fx_scroll_actualizar(); SPR_update(); SYS_doVBlankProcess(); }
    }
    efecto_shake(8, 1);
}

// ------------------------------------------------------------------ lógica por frame
void actualizar_logica() {
    frame_counter++;
    fx_actualizar();
    fx_scroll_actualizar();
    mapa_paleta_animar();
    animar_polluelos_mapa();
    objetos_actualizar();
    polluelos_voladores_actualizar();
    ambiente_actualizar();

    if (timer_cazador_lento > 0) timer_cazador_lento--;
    if (timer_flicky_rapido > 0) timer_flicky_rapido--;
    if (!en_jefe && --timer_spawn_powerup <= 0) {
        generar_powerup_aleatorio();
        timer_spawn_powerup = 240 + (random() % 240);
    }

    // pulso suave de los muros en rabia o antes de un cierre (sin parpadeo brusco)
    if (cazador_en_furia || (BR_ACTIVO && en_preaviso_cierre)) {
        u16 f = (frame_counter >> 1) & 15;
        muros_mezcla(f < 8 ? f : 16 - f);
    }

    // reloj de la ronda
    if ((frame_counter % 60) == 0) {
        if (tiempo_restante_segundos > 0) {
            tiempo_restante_segundos--;
            s32 jugados = tiempo_inicial - tiempo_restante_segundos;
            if (modo_jugadores == 0 && !en_jefe && jugados >= 15) {
                if (!cazador_en_furia) {
                    if ((random() % 100) < 12 && cazador.aturdido == 0) {
                        cazador_en_furia = TRUE;
                        timer_duracion_furia = 8;
                        sonido(SFX_RABIA);
                        efecto_shake(15, 2);
                    }
                } else if (--timer_duracion_furia <= 0) {
                    cazador_en_furia = FALSE;
                    muros_alerta(FALSE);
                }
            }
            if (BR_ACTIVO) {
                // tres cierres repartidos por la ronda (a 2/3, 2/5 y 1/6 del tiempo), con 5 s de aviso
                s32 t = tiempo_restante_segundos;
                s32 c1 = tiempo_inicial * 2 / 3, c2 = tiempo_inicial * 2 / 5, c3 = tiempo_inicial / 6;
                if (t == c1 + 5 || t == c2 + 5 || t == c3 + 5) en_preaviso_cierre = TRUE;
                if (t == c1 || t == c2 || t == c3) {
                    aplicar_recorte_battle_royale();
                    if (estado_actual != ESTADO_JUEGO) return;
                }
                if (en_preaviso_cierre) sonido(SFX_ALERTA);
            }
            if (tiempo_restante_segundos <= 10 && !en_preaviso_cierre) sonido(SFX_ALERTA);
            if (jugados > 0 && (jugados % 60) == 0 && modo_jugadores == 0) multiplicador_velocidad_minuto++;
        } else {
            if (modo_jugadores == 1) verificar_fin_ronda_versus(FALSE);
            else jugador_muere(T("SE ACABO EL TIEMPO!", "TIME IS UP!"));
            return;
        }
    }

    jugador_actualizar();

    if (!en_jefe && en_nido()) {
        if (semillas_en_fila > 0) entregar_polluelos();
        if (total_salvados >= max_polluelos_nivel && !salida_abierta) {
            salida_abierta = TRUE;
            sonido(SFX_META);
            efecto_shake(15, 2);
        }
        if (salida_abierta && semillas_en_fila == 0) {
            if (modo_jugadores == 1) verificar_fin_ronda_versus(TRUE);
            else fase_superada(FALSE);
            return;
        }
    }

    jugador_animar();
    proyectiles_actualizar();
    if (estado_actual != ESTADO_JUEGO) return;

    if (en_jefe) {
        jefe_actualizar();
        if (estado_actual != ESTADO_JUEGO) return;
        if (!flicky_invulnerable && jefe_toca(flicky.x + 8, flicky.y + 8, 3)) {
            jugador_herido_jefe();
            if (estado_actual != ESTADO_JUEGO) return;
        }
        hud_actualizar();
        return;
    }

    // con poco tiempo el enemigo parpadea (aviso)
    if (cazador.oculto == 0 && modo_jugadores == 0)
        SPR_setVisibility(cazador.sprite, ((tiempo_restante_segundos <= 15 || en_preaviso_cierre) && (frame_counter & 8)) ? HIDDEN : VISIBLE);
    cazador_actualizar();

    if (abs(flicky.x - cazador.x) < 12 && abs(flicky.y - cazador.y) < 12 && cazador_peligroso() && !flicky_invulnerable) {
        if (modo_jugadores == 1) { verificar_fin_ronda_versus(FALSE); return; }
        if (cazador_en_furia) { muros_alerta(TRUE); jugador_muere(T("LA RABIA TE DESTRUYO!", "THE HUNTER WENT BERSERK!")); return; }
        if (semillas_en_fila > 0) jugador_golpeado();
        else { jugador_muere(T("EL CAZADOR TE ATRAPO!", "THE HUNTER CAUGHT YOU!")); return; }
    }
    hud_actualizar();
}
