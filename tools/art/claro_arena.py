"""Arena del jefe del bosque: un claro pintado como imagen de fondo (plano B, PAL1).

Se diseña pensando en las baldosas de 8x8: el césped repite un patrón de 32x32 en tres niveles de luz
(soleado en el centro, normal y en sombra hacia el borde) y el borde de copas repite cada 24 px, para que
rescomp pueda reutilizar baldosas y quepa en la VRAM. Los muros de esta arena son: el borde, el estanque
(2x2 casillas en ESTANQUE) y los dos árboles (que se dibujan aparte, en el plano A, con su sombra aquí).
"""
import math, os
from PIL import Image

W, H = 320, 208                 # área de juego (20x13 casillas)
ESTANQUE = (4, 8)               # casilla superior izquierda del estanque (2x2)
ARBOLES = ((5, 2), (13, 9))     # casillas superiores izquierdas de los árboles (2x2)

PAL = [(0, 0, 0),
       (132, 188, 74), (176, 214, 100), (104, 156, 58),      # 1 césped, 2 sol, 3 briznas
       (88, 136, 52), (64, 104, 44),                         # 4 sombra, 5 sombra honda
       (18, 48, 28),                                         # 6 bosque oscuro
       (44, 124, 52), (28, 86, 40), (76, 160, 60), (14, 40, 24),   # 7-10 copas: medio, oscuro, claro, contorno
       (70, 140, 200), (210, 236, 252), (40, 96, 160),       # 11-13 agua, brillo/flor blanca, agua honda
       (150, 140, 120), (240, 120, 160)]                     # 14 piedra, 15 flor rosa

def periodic(x, y, p=32):
    a = 2 * math.pi / p
    return math.sin(a * x) * math.cos(a * y) + 0.6 * math.sin(a * (x + 2 * y) + 1.3) + 0.4 * math.cos(a * (2 * x - y) + 0.7)

def cesped(x, y, nivel):
    """nivel 0 = soleado, 1 = normal, 2 = sombra. Patrón de 32x32 que encaja con las baldosas."""
    v = periodic(x, y)
    briz = ((x * 7 + (y // 3) * 13) % 23) == 0 and (y % 3) != 2
    if nivel == 0:
        c = 2 if v > 0.75 else 1
        if briz: c = 3
    elif nivel == 1:
        c = 2 if v > 1.25 else 1
        if briz: c = 3
    else:
        c = 4 if v > -0.6 else 5
        if briz: c = 5
    return c

def generar(ruta):
    img = [[0] * W for _ in range(H)]
    cx, cy = W / 2, H / 2
    for y in range(H):
        for x in range(W):
            # nivel de luz por bloques de 8x8 (para reutilizar baldosas), con borde tramado
            bx, by = (x // 8) * 8 + 4, (y // 8) * 8 + 4
            r = math.hypot((bx - cx) / 128, (by - cy) / 84)
            nivel = 0 if r < 0.62 else (1 if r < 0.9 else 2)
            # transición tramada entre luz normal y sombra (evita el borde en escalera)
            if 0.9 <= r < 1.02 and (x + y) % 2 == 0: nivel = 1
            if 0.82 <= r < 0.9 and (x + y) % 4 == 0 and (y % 2) == 0: nivel = 2
            img[y][x] = cesped(x, y, nivel)
    # sombras de los árboles en el suelo
    for (tx, ty) in ARBOLES:
        ox, oy = tx * 16 + 16, ty * 16 + 30
        for y in range(oy - 5, oy + 5):
            for x in range(ox - 15, ox + 15):
                if 0 <= x < W and 0 <= y < H and ((x - ox) / 15) ** 2 + ((y - oy) / 4.5) ** 2 < 1: img[y][x] = 4
    # estanque con nenúfares
    px, py = ESTANQUE[0] * 16 + 16, ESTANQUE[1] * 16 + 16
    for y in range(py - 16, py + 16):
        for x in range(px - 16, px + 16):
            d = ((x - px + .5) / 15.5) ** 2 + ((y - py + .5) / 13) ** 2
            if d < 1:
                c = 11
                if d > 0.76: c = 14 if (x + y) % 4 else 5
                elif (y - py) < -4 and d > 0.45: c = 13
                elif (x // 2 + y) % 11 == 0: c = 12
                img[y][x] = c
    for (lx, ly) in ((px - 7, py + 3), (px + 6, py - 2)):
        for y in range(ly - 3, ly + 3):
            for x in range(lx - 4, lx + 4):
                if ((x - lx) / 4) ** 2 + ((y - ly) / 2.6) ** 2 < 1 and not (x > lx and y == ly): img[y][x] = 7 if y > ly - 1 else 9
        img[ly - 1][lx] = 15; img[ly - 1][lx + 1] = 12
    # flores junto al borde (repetidas en posiciones múltiplos de 8 para reutilizar baldosas)
    def flor(x, y, c):
        for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)): img[y + dy][x + dx] = c
        img[y][x] = 2
        img[y + 2][x] = 7; img[y + 3][x] = 7
    for i, x in enumerate(range(28, W - 24, 40)):
        flor(x, 24, 15 if i % 2 else 12); flor(x + 12, H - 22, 12 if i % 2 else 15)
    for i, y in enumerate(range(36, H - 30, 40)):
        flor(22, y, 15 if i % 2 else 12); flor(W - 24, y + 14, 12 if i % 2 else 15)
    # borde: bosque espeso con copas redondas que asoman (repiten cada 24 px)
    def copa(cx, cy, r):
        for y in range(max(0, cy - r), min(H, cy + r)):
            for x in range(max(0, cx - r), min(W, cx + r)):
                d = math.hypot(x - cx + .5, y - cy + .5) / r
                if d >= 1: continue
                luz = ((cx - x) + (cy - y)) / r
                bxx = (x - cx + 30 + (3 if ((y - cy + 30) // 5) % 2 else 0)) % 6; byy = (y - cy + 30) % 5
                bump = (bxx - 3) ** 2 + (byy - 2) ** 2
                c = 7
                if luz < -0.2 or bump >= 7: c = 8
                if luz > 0.3 and bump <= 2: c = 9
                if d > 0.92: c = 10
                img[y][x] = c
    for y in range(H):
        for x in range(W):
            if x < 10 or x >= W - 10 or y < 10 or y >= H - 10: img[y][x] = 6
    R = 15
    for x in range(-8, W + 24, 24):
        copa(x, 4, R); copa(x + 12, H - 5, R)
    for y in range(4, H + 24, 24):
        copa(4, y, R); copa(W - 5, y + 12, R)
    im = Image.new('P', (W, H)); im.putpalette([v for c in PAL for v in c] + [0] * (768 - 48))
    im.putdata([v for row in img for v in row])
    im.save(ruta)
    # recuento de baldosas únicas (con volteos), para vigilar la VRAM
    tiles = set()
    for ty in range(0, H, 8):
        for tx in range(0, W, 8):
            t = tuple(tuple(img[ty + y][tx:tx + 8]) for y in range(8))
            vs = [t, tuple(r[::-1] for r in t), t[::-1], tuple(r[::-1] for r in t[::-1])]
            tiles.add(min(vs))
    return len(tiles)

if __name__ == '__main__':
    ruta = os.path.join(os.path.dirname(__file__), '..', '..', 'res', 'gen', 'claro_arena.png')
    print('baldosas únicas:', generar(ruta))
