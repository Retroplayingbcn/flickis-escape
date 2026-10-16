// Laberinto: generación, recorridos, dibujo por metatiles, paletas y objetos del suelo
#include "juego.h"

u8 mapa[MAP_HEIGHT][MAP_WIDTH];
bool alcanzable[MAP_HEIGHT][MAP_WIDTH];
u8 distancia[MAP_HEIGHT][MAP_WIDTH];
u8 deco[MAP_HEIGHT][MAP_WIDTH];

#define META_DECO    32       // en el tileset del suelo: 32 metatiles de suelo, 6 adornos,
#define META_DOBLADO 38       //   2 adornos doblados por el viento
#define META_CAMINO  40       //   y 16 piezas de camino de tierra (bosque)
#define TILE_CLARO   98       // en el tileset del bosque, tras los 98 tiles normales: piezas del claro

// los árboles y el tronco del claro no proyectan la sombra cuadrada de los setos
static bool da_sombra(s16 x, s16 y) {
    if (!hay_muro(x, y)) return FALSE;
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return TRUE;
    u8 d = deco[y][x];
    return !(d & DECO_ESPECIAL) || (d & 7) >= PIEZA_FOLLAJE1;
}

static bool camino(s16 x, s16 y) {
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return FALSE;
    return deco[y][x] == DECO_CAMINO;
}
u16 base_maze, base_floor, base_items, base_hud;

static const Palette* pal_maze_actual = &pal_bosque_maze;
static const Palette* pal_floor_actual = &pal_bosque_floor;
static s16 cola_x[MAP_WIDTH * MAP_HEIGHT], cola_y[MAP_WIDTH * MAP_HEIGHT];
static u16 nivel_alerta = 0;
static bool arena_pintada = FALSE;                // arena del bosque: el suelo es una imagen
static const TileSet* ts_floor_actual = &ts_bosque_floor;

bool muros_en_alerta() { return nivel_alerta != 0; }

// ráfaga de viento (bosque): las flores y la hierba del suelo se doblan cambiando sus tiles en VRAM
void deco_viento(bool doblado) {
    if (mundo_actual != MUNDO_BOSQUE || arena_pintada) return;
    const u32* t = ts_floor_actual->tiles;
    VDP_loadTileData(t + (doblado ? META_DOBLADO : META_DECO) * 4 * 8, base_floor + META_DECO * 4, 4, DMA);
    VDP_loadTileData(t + (doblado ? META_DOBLADO + 1 : META_DECO + 2) * 4 * 8, base_floor + (META_DECO + 2) * 4, 4, DMA);
}

#define META_NIDO 16          // metatiles 16..23: nido con 0..3 huevos (izquierda, derecha)
static const s16 dx4[4] = { 1, -1, 0, 0 };
static const s16 dy4[4] = { 0, 0, 1, -1 };

// ------------------------------------------------------------------ consultas
bool hay_muro(s16 x, s16 y) {
    if (x < 0 || y < 0 || x >= MAP_WIDTH || y >= MAP_HEIGHT) return TRUE;   // fuera = muro (borde continuo)
    return mapa[y][x] == TILE_MURO;
}

static bool es_celda_nido(s16 x, s16 y) {
    return !en_jefe && y == NIDO_Y && x >= NIDO_X && x < NIDO_X + NIDO_ANCHO;
}

bool en_zona_nido(s16 tx, s16 ty) {
    return !en_jefe && ty >= NIDO_Y - 1 && ty <= NIDO_Y + 1 && tx >= NIDO_X - 1 && tx <= NIDO_X + NIDO_ANCHO;
}

// ------------------------------------------------------------------ generación
// Laberinto trenzado y simétrico: backtracker en la mitad izquierda, se quitan la mayoría de
// callejones sin salida, columna central y plaza del nido abiertas y espejo horizontal.
void generar_mapa_aleatorio() {
    static s16 pila_x[64], pila_y[64];
    static bool visitada[MAP_HEIGHT][10];
    const s16 ddx[4] = {2, -2, 0, 0};
    const s16 ddy[4] = {0, 0, 2, -2};
    {   // cada nivel distinto (depende del tiempo en menús); nunca semilla nula para el xorshift
        u16 semilla = random() ^ (u16)vtimer ^ 0xD94B;
        setRandomSeed(semilla ? (semilla ^ 0xD94B) : 0x1234);
    }
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
    // trenzado: quitar callejones sin salida (70%)
    for (s16 y = 1; y <= MAP_HEIGHT - 2; y += 2) {
        for (s16 x = 1; x <= 7; x += 2) {
            u16 salidas = 0;
            for (u16 d = 0; d < 4; d++) if (mapa[y + dy4[d]][x + dx4[d]] == TILE_SUELO) salidas++;
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
    // columna central abierta con algún tope, conexiones desde la columna 8, y plaza del nido
    for (s16 y = 1; y <= MAP_HEIGHT - 2; y++) {
        mapa[y][9] = ((y % 4) == 2 && (random() & 1)) ? TILE_MURO : TILE_SUELO;
        if (y & 1) mapa[y][8] = TILE_SUELO;
    }
    for (s16 y = NIDO_Y - 1; y <= NIDO_Y + 1; y++) { mapa[y][8] = TILE_SUELO; mapa[y][9] = TILE_SUELO; }
    // espejo y borde
    for (s16 y = 0; y < MAP_HEIGHT; y++)
        for (s16 x = 0; x < 10; x++) mapa[y][MAP_WIDTH - 1 - x] = mapa[y][x];
    for (s16 x = 0; x < MAP_WIDTH; x++) { mapa[0][x] = TILE_MURO; mapa[MAP_HEIGHT - 1][x] = TILE_MURO; }
    for (s16 y = 0; y < MAP_HEIGHT; y++) { mapa[y][0] = TILE_MURO; mapa[y][MAP_WIDTH - 1] = TILE_MURO; }
    decorar(mundo_actual == MUNDO_VOLCAN ? 12 : 9, FALSE);
}

// adornos del suelo al azar (no estorban: solo se dibujan). Los dos últimos del bosque y del volcán
// tienen volumen (tocón, roca...): en un pasillo parecerían un muro, así que solo van en la arena.
static u16 adornos_planos() { return (mundo_actual == MUNDO_BOSQUE || mundo_actual == MUNDO_VOLCAN) ? 4 : 6; }

void decorar(u16 n, bool volumen) {
    memset(deco, 0, sizeof(deco));
    u16 tipos = volumen ? 6 : adornos_planos();
    for (u16 i = 0, intentos = 0; i < n && intentos < 200; intentos++) {
        s16 x = (random() % (MAP_WIDTH - 2)) + 1, y = (random() % (MAP_HEIGHT - 2)) + 1;
        if (mapa[y][x] != TILE_SUELO || en_zona_nido(x, y) || deco[y][x] || (x <= 2 && y <= 2)) continue;
        deco[y][x] = 1 + (random() % tipos);
        i++;
    }
}

static void pieza(s16 x, s16 y, u8 p) { mapa[y][x] = TILE_MURO; deco[y][x] = DECO_ESPECIAL | p; }

static void arbol(s16 x, s16 y) {
    pieza(x, y, PIEZA_ARBOL_TL); pieza(x + 1, y, PIEZA_ARBOL_TR);
    pieza(x, y + 1, PIEZA_ARBOL_BL); pieza(x + 1, y + 1, PIEZA_ARBOL_BR);
}

// Arena del bosque: un claro pintado como imagen (img_claro). Los muros son el borde de árboles y el
// estanque, que ya están en la imagen, y dos árboles grandes que se dibujan en el plano A.
static void generar_claro() {
    memset(deco, 0, sizeof(deco));
    for (s16 y = 0; y < MAP_HEIGHT; y++)
        for (s16 x = 0; x < MAP_WIDTH; x++) {
            bool borde = (x == 0 || y == 0 || x == MAP_WIDTH - 1 || y == MAP_HEIGHT - 1);
            mapa[y][x] = borde ? TILE_MURO : TILE_SUELO;
            if (borde) deco[y][x] = DECO_PINTADO;
        }
    for (s16 y = 8; y <= 9; y++) for (s16 x = 4; x <= 5; x++) { mapa[y][x] = TILE_MURO; deco[y][x] = DECO_PINTADO; }   // estanque
    arbol(5, 2); arbol(13, 9);
}

// Arena del jefe: espacio abierto con columnas para cubrirse (en el bosque, un claro con árboles)
void generar_arena() {
    if (mundo_actual == MUNDO_BOSQUE) { generar_claro(); return; }
    for (s16 y = 0; y < MAP_HEIGHT; y++)
        for (s16 x = 0; x < MAP_WIDTH; x++)
            mapa[y][x] = (x == 0 || y == 0 || x == MAP_WIDTH - 1 || y == MAP_HEIGHT - 1) ? TILE_MURO : TILE_SUELO;
    // tres disposiciones de columnas; siempre con pasillos de 2 casillas para que el jefe quepa
    static const s8 disp[3][10][2] = {
        { {5, 3}, {14, 3}, {5, 9}, {14, 9}, {9, 6}, {10, 6}, {-1, 0} },
        { {4, 3}, {5, 3}, {14, 3}, {15, 3}, {4, 9}, {5, 9}, {14, 9}, {15, 9}, {9, 6}, {10, 6} },
        { {9, 3}, {10, 3}, {9, 9}, {10, 9}, {5, 6}, {16, 3}, {16, 9}, {3, 3}, {3, 9}, {-1, 0} },
    };
    u16 d = random() % 3;
    for (u16 i = 0; i < 10 && disp[d][i][0] >= 0; i++) mapa[(u16)disp[d][i][1]][(u16)disp[d][i][0]] = TILE_MURO;
    decorar(12, TRUE);
}

// ------------------------------------------------------------------ recorridos (BFS)
static void bfs(s16 sx, s16 sy, bool solo_alcance) {
    u16 inicio = 0, fin = 0;
    for (u16 y = 0; y < MAP_HEIGHT; y++)
        for (u16 x = 0; x < MAP_WIDTH; x++) { distancia[y][x] = 255; if (solo_alcance) alcanzable[y][x] = FALSE; }
    if (hay_muro(sx, sy)) return;
    cola_x[fin] = sx; cola_y[fin] = sy; fin++;
    distancia[sy][sx] = 0;
    if (solo_alcance) alcanzable[sy][sx] = TRUE;
    while (inicio < fin) {
        s16 cx = cola_x[inicio], cy = cola_y[inicio]; inicio++;
        u8 nd = distancia[cy][cx] + 1;
        for (u16 d = 0; d < 4; d++) {
            s16 nx = cx + dx4[d], ny = cy + dy4[d];
            if (!hay_muro(nx, ny) && distancia[ny][nx] == 255) {
                distancia[ny][nx] = nd;
                if (solo_alcance) alcanzable[ny][nx] = TRUE;
                cola_x[fin] = nx; cola_y[fin] = ny; fin++;
            }
        }
    }
}
// alcanzable desde el nido (o desde la esquina en la arena del jefe, donde el centro es una columna)
void calcular_alcanzables() {
    if (mapa[NIDO_Y][NIDO_X] != TILE_MURO) bfs(NIDO_X, NIDO_Y, TRUE);
    else bfs(1, 1, TRUE);
}
void calcular_distancias(s16 cx, s16 cy) { bfs(cx, cy, FALSE); }

void repartir_polluelos_seguros() {
    static s16 px[MAX_POLLUELOS], py[MAX_POLLUELOS];
    u16 repartidos = 0, intentos = 0;
    while (repartidos < max_polluelos_nivel && intentos < 600) {
        intentos++;
        u16 rx = (random() % (MAP_WIDTH - 2)) + 1;
        u16 ry = (random() % (MAP_HEIGHT - 2)) + 1;
        if (mapa[ry][rx] != TILE_SUELO || en_zona_nido(rx, ry) || !alcanzable[ry][rx] || (rx <= 2 && ry <= 2)) continue;
        bool cerca = FALSE;
        for (u16 i = 0; i < repartidos; i++) if (abs(px[i] - (s16)rx) < 2 && abs(py[i] - (s16)ry) < 2) cerca = TRUE;
        if (cerca && intentos < 400) continue;
        mapa[ry][rx] = TILE_POLLUELO;
        px[repartidos] = rx; py[repartidos] = ry; repartidos++;
    }
}

bool buscar_celda_libre(s16* out_x, s16* out_y, s16 dist_min_jugador) {
    s16 jx = (flicky.x + 8) >> CELDA_SHIFT, jy = (flicky.y + 8) >> CELDA_SHIFT;
    for (u16 intentos = 0; intentos < 300; intentos++) {
        s16 rx = (random() % (MAP_WIDTH - 2)) + 1;
        s16 ry = (random() % (MAP_HEIGHT - 2)) + 1;
        if (mapa[ry][rx] != TILE_SUELO || en_zona_nido(rx, ry) || !alcanzable[ry][rx]) continue;
        if (abs(rx - jx) + abs(ry - jy) < dist_min_jugador) continue;
        *out_x = rx; *out_y = ry;
        return TRUE;
    }
    return FALSE;
}

// ------------------------------------------------------------------ dibujo
static void poner_meta(VDPPlane plano, u16 pal, u16 tile_base, s16 cx, s16 cy) {
    u16 x = cx << 1, y = (cy << 1) + PLAY_TY;
    VDP_setTileMapXY(plano, TILE_ATTR_FULL(pal, 0, 0, 0, tile_base),     x,     y);
    VDP_setTileMapXY(plano, TILE_ATTR_FULL(pal, 0, 0, 0, tile_base + 1), x + 1, y);
    VDP_setTileMapXY(plano, TILE_ATTR_FULL(pal, 0, 0, 0, tile_base + 2), x,     y + 1);
    VDP_setTileMapXY(plano, TILE_ATTR_FULL(pal, 0, 0, 0, tile_base + 3), x + 1, y + 1);
}

static void borrar_plano_a(s16 cx, s16 cy) {
    u16 x = cx << 1, y = (cy << 1) + PLAY_TY;
    VDP_setTileMapXY(BG_A, 0, x, y);     VDP_setTileMapXY(BG_A, 0, x + 1, y);
    VDP_setTileMapXY(BG_A, 0, x, y + 1); VDP_setTileMapXY(BG_A, 0, x + 1, y + 1);
}

static s16 item_de_tile(u8 t) {
    switch (t) {
        case TILE_POLLUELO: return ITEM_POLLUELO;
        case TILE_RELOJ:    return ITEM_RELOJ;
        case TILE_RAYO:     return ITEM_RAYO;
        case TILE_GEMA:     return ITEM_GEMA;
        case TILE_HUEVO:    return ITEM_HUEVO;
    }
    return -1;
}

void dibujar_celda(s16 cx, s16 cy) {
    if (cx < 0 || cy < 0 || cx >= MAP_WIDTH || cy >= MAP_HEIGHT) return;
    u8 t = mapa[cy][cx];
    // plano A: muros (autotile por vecinos), nido u objetos
    if (t == TILE_MURO && deco[cy][cx] == DECO_PINTADO) {
        borrar_plano_a(cx, cy);
    } else if (t == TILE_MURO && (deco[cy][cx] & DECO_ESPECIAL)) {
        poner_meta(BG_A, PAL0, base_maze + TILE_CLARO + (deco[cy][cx] & 7) * 4, cx, cy);   // árbol, tronco, follaje
    } else if (t == TILE_MURO) {
        u16 mask = (hay_muro(cx, cy - 1) ? 1 : 0) | (hay_muro(cx + 1, cy) ? 2 : 0) |
                   (hay_muro(cx, cy + 1) ? 4 : 0) | (hay_muro(cx - 1, cy) ? 8 : 0);
        poner_meta(BG_A, PAL0, base_maze + mask * 4, cx, cy);
    } else if (es_celda_nido(cx, cy)) {
        u16 lleno = max_polluelos_nivel ? (total_salvados * 3) / max_polluelos_nivel : 0;
        if (lleno > 3) lleno = 3;
        poner_meta(BG_A, PAL0, base_maze + (META_NIDO + lleno * 2 + cx - NIDO_X) * 4, cx, cy);
    } else {
        s16 item = item_de_tile(t);
        if (item >= 0) poner_meta(BG_A, PAL2, base_items + item * 4, cx, cy);
        else if (t == TILE_SUELO && deco[cy][cx] && deco[cy][cx] < DECO_CAMINO)
            poner_meta(BG_A, PAL1, base_floor + (META_DECO + deco[cy][cx] - 1) * 4, cx, cy);
        else borrar_plano_a(cx, cy);
    }
    if (arena_pintada) return;                            // el suelo es la imagen: no se toca el plano B
    if (deco[cy][cx] == DECO_CAMINO) {                    // camino de tierra: autotile con sus vecinos
        u16 m = (camino(cx, cy - 1) ? 1 : 0) | (camino(cx + 1, cy) ? 2 : 0) | (camino(cx, cy + 1) ? 4 : 0) | (camino(cx - 1, cy) ? 8 : 0);
        poner_meta(BG_B, PAL1, base_floor + (META_CAMINO + m) * 4, cx, cy);
        return;
    }
    // plano B: suelo con sombra proyectada por los muros de arriba/izquierda
    u16 sombra = (da_sombra(cx, cy - 1) ? 1 : 0) | (da_sombra(cx - 1, cy) ? 2 : 0) | (da_sombra(cx - 1, cy - 1) ? 4 : 0);
    // el césped del bosque es un patrón continuo de 32x32; los demás suelos varían al azar
    u16 textura = (mundo_actual == MUNDO_BOSQUE) ? ((cx & 1) | ((cy & 1) << 1)) : (((cx * 7) + (cy * 13)) & 3);
    poner_meta(BG_B, PAL1, base_floor + (textura * 8 + sombra) * 4, cx, cy);
}

void dibujar_mapa_completo() {
    if (arena_pintada)                                    // repinta la imagen (por si un cartel la tapó)
        VDP_setTileMapEx(BG_B, img_claro.tilemap, TILE_ATTR_FULL(PAL1, 0, 0, 0, base_floor), 0, PLAY_TY, 0, 0, 40, 26, DMA);
    for (s16 y = 0; y < MAP_HEIGHT; y++)
        for (s16 x = 0; x < MAP_WIDTH; x++) dibujar_celda(x, y);
}

void dibujar_nido() {
    for (s16 x = NIDO_X; x < NIDO_X + NIDO_ANCHO; x++) dibujar_celda(x, NIDO_Y);
}

// ------------------------------------------------------------------ gráficos y paletas del mundo
void cargar_graficos_mundo() {
    const TileSet* ts_maze = &ts_bosque_maze;   const TileSet* ts_floor = &ts_bosque_floor;
    const Palette* p_maze  = &pal_bosque_maze;  const Palette* p_floor  = &pal_bosque_floor;
    if (mundo_actual == MUNDO_VOLCAN)       { ts_maze = &ts_volcan_maze;  ts_floor = &ts_volcan_floor;  p_maze = &pal_volcan_maze;  p_floor = &pal_volcan_floor; }
    else if (mundo_actual == MUNDO_MARINO)  { ts_maze = &ts_marino_maze;  ts_floor = &ts_marino_floor;  p_maze = &pal_marino_maze;  p_floor = &pal_marino_floor; }
    else if (mundo_actual == MUNDO_CELESTE) { ts_maze = &ts_celeste_maze; ts_floor = &ts_celeste_floor; p_maze = &pal_celeste_maze; p_floor = &pal_celeste_floor; }
    pal_maze_actual = p_maze; pal_floor_actual = p_floor; ts_floor_actual = ts_floor;
    base_maze  = TILE_USER_INDEX;
    // la arena del jefe del bosque es una imagen pintada: sus baldosas ocupan el sitio del suelo
    arena_pintada = (en_jefe && mundo_actual == MUNDO_BOSQUE);
    if (arena_pintada) { ts_floor = img_claro.tileset; p_floor = img_claro.palette; pal_floor_actual = p_floor; }
    base_floor = base_maze + ts_maze->numTile;
    base_items = base_floor + ts_floor->numTile;
    base_hud   = base_items + ts_items.numTile;
    VDP_loadTileSet(ts_maze, base_maze, DMA);
    VDP_loadTileSet(ts_floor, base_floor, DMA);
    if (arena_pintada)
        VDP_setTileMapEx(BG_B, img_claro.tilemap, TILE_ATTR_FULL(PAL1, 0, 0, 0, base_floor), 0, PLAY_TY, 0, 0, 40, 26, DMA);
    VDP_loadTileSet(&ts_items, base_items, DMA);
    VDP_loadTileSet(&ts_hud, base_hud, DMA);
    VDP_loadFont(&ts_font, DMA);
    PAL_setPalette(PAL0, p_maze->data, CPU);
    PAL_setPalette(PAL1, p_floor->data, CPU);
    PAL_setPalette(PAL2, pal_items.data, CPU);
    VDP_setTextPalette(PAL0);
    nivel_alerta = 0;
}

// Muros en alerta (rabia / cierre): mezcla los 5 tonos del muro con rojo (nivel 0..8)
static const u16 rojos_alerta[5] = { RGB24_TO_VDPCOLOR(0xD83020), RGB24_TO_VDPCOLOR(0x902018), RGB24_TO_VDPCOLOR(0xF06040),
                                     RGB24_TO_VDPCOLOR(0xFFA080), RGB24_TO_VDPCOLOR(0x601010) };
void muros_mezcla(u16 nivel) {
    if (nivel == nivel_alerta) return;
    nivel_alerta = nivel;
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

// Ciclo de colores por mundo: la lava de las vetas del volcán palpita, las estrellas del cielo titilan
void mapa_paleta_animar() {
    if (PAL_isDoingFade()) return;
    if (mundo_actual == MUNDO_VOLCAN && nivel_alerta == 0 && (frame_counter & 7) == 0) {
        static const u16 lava[6] = { RGB24_TO_VDPCOLOR(0xFF8C28), RGB24_TO_VDPCOLOR(0xFFB040), RGB24_TO_VDPCOLOR(0xFFD860),
                                     RGB24_TO_VDPCOLOR(0xFFB040), RGB24_TO_VDPCOLOR(0xFF8C28), RGB24_TO_VDPCOLOR(0xE05020) };
        PAL_setColor(4, lava[(frame_counter >> 3) % 6]);
        PAL_setColor(16 + 6, lava[((frame_counter >> 3) + 2) % 6]);       // la lava de los adornos, a contratiempo
    } else if (mundo_actual == MUNDO_CELESTE && (frame_counter & 15) == 0) {
        PAL_setColor(16 + 3, ((frame_counter >> 4) & 1) ? pal_floor_actual->data[1] : RGB24_TO_VDPCOLOR(0xE8F4FF));
        PAL_setColor(16 + 8, ((frame_counter >> 4) & 1) ? pal_floor_actual->data[8] : RGB24_TO_VDPCOLOR(0xFFFFFF));   // estrellas que titilan
    }
}

// Los polluelos del mapa se animan cambiando sus 4 tiles en VRAM
void animar_polluelos_mapa() {
    static u8 t = 0, f = 0;
    if (++t < 10) return;
    t = 0; f = (f + 1) & 3;
    VDP_loadTileData(ts_chick_anim.tiles + f * 4 * 8, base_items, 4, DMA);
}

// ------------------------------------------------------------------ objetos que aparecen (y caducan)
#define MAX_OBJETOS 8
typedef struct { s8 x, y; u8 tipo; s16 t; } Objeto;
static Objeto objetos[MAX_OBJETOS];

void objetos_reset() { for (u16 i = 0; i < MAX_OBJETOS; i++) objetos[i].t = 0; }

void objeto_colocar(s16 x, s16 y, u8 tipo, bool caduca) {
    mapa[y][x] = tipo;
    dibujar_celda(x, y);
    fx_chispa(x * CELDA, y * CELDA);                       // aparece con un destello
    if (!caduca) return;
    for (u16 i = 0; i < MAX_OBJETOS; i++)
        if (objetos[i].t == 0) { objetos[i].x = x; objetos[i].y = y; objetos[i].tipo = tipo; objetos[i].t = 600; return; }
}

void objetos_actualizar() {
    for (u16 i = 0; i < MAX_OBJETOS; i++) {
        Objeto* o = &objetos[i];
        if (o->t == 0) continue;
        if (mapa[(u16)o->y][(u16)o->x] != o->tipo) { o->t = 0; continue; }   // ya recogido
        o->t--;
        if (o->t == 0) { mapa[(u16)o->y][(u16)o->x] = TILE_SUELO; dibujar_celda(o->x, o->y); continue; }
        if (o->t < 120) {                                                   // parpadea antes de desaparecer
            if ((o->t & 7) == 0) borrar_plano_a(o->x, o->y);
            else if ((o->t & 7) == 4) dibujar_celda(o->x, o->y);
        }
    }
}

void generar_powerup_aleatorio() {
    s16 rx, ry;
    if (!buscar_celda_libre(&rx, &ry, 3)) return;
    u16 a = anillo_recorte + 1;                       // nada en el anillo que se va a cerrar
    if (BR_ACTIVO && (rx < a || rx >= MAP_WIDTH - a || ry < a || ry >= MAP_HEIGHT - a)) return;
    u16 r = random() % 100;
    u8 tipo;
    if (multiplicador_velocidad_minuto > 0 || cazador_en_furia) {
        tipo = (r < 35) ? TILE_RELOJ : (r < 50) ? TILE_RAYO : (r < 65) ? TILE_GEMA : TILE_HUEVO;
    } else {
        tipo = (r < 22) ? TILE_RELOJ : (r < 44) ? TILE_RAYO : (r < 78) ? TILE_GEMA : TILE_HUEVO;
    }
    objeto_colocar(rx, ry, tipo, TRUE);
}
