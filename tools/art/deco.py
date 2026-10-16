"""Decoración del suelo (sin colisión), 16x16, en la paleta del suelo (PAL1).

Índices: 0 transparente, 4 sombra del suelo, 6..15 colores propios de cada mundo (DECO_PAL).
Cada mundo tiene 5 adornos; se dibujan en el plano A encima del suelo.
"""
import math

DECO_PAL = {
    #           6              7              8              9              10             11             12             13             14             15
    'bosque':  [(240,120,160),(250,220,80),(250,250,240),(52,110,40),(104,176,60),(140,92,48),(92,58,32),(220,60,50),(244,224,186),(36,40,28)],
    'volcan':  [(255,140,40),(255,222,100),(120,104,104),(76,62,66),(160,146,140),(190,180,180),(236,226,206),(150,40,30),(255,96,32),(30,20,24)],
    'marino':  [(255,120,150),(196,64,108),(250,234,206),(214,174,140),(64,164,110),(30,110,80),(250,150,60),(210,244,255),(255,200,120),(70,52,66)],
    'celeste': [(255,255,255),(204,214,244),(255,232,110),(240,100,100),(250,170,80),(120,210,120),(110,150,240),(176,124,224),(255,250,210),(92,108,170)],
}

def blank(): return [[0] * 16 for _ in range(16)]

def ellipse(g, cx, cy, rx, ry, c, edge=None):
    for y in range(16):
        for x in range(16):
            d = ((x + .5 - cx) / rx) ** 2 + ((y + .5 - cy) / ry) ** 2
            if d < 1:
                g[y][x] = edge if (edge is not None and d > 0.62) else c

def shadow(g, cx, cy, rx, ry=1.6):
    for y in range(16):
        for x in range(16):
            if g[y][x] == 0 and ((x + .5 - cx) / rx) ** 2 + ((y + .5 - cy) / ry) ** 2 < 1: g[y][x] = 4

def px(g, x, y, c):
    if 0 <= x < 16 and 0 <= y < 16: g[y][x] = c

def outline(g, ink):
    """contorno de 1 px alrededor de lo dibujado (no toca la sombra)"""
    out = [r[:] for r in g]
    for y in range(16):
        for x in range(16):
            if g[y][x] not in (0, 4): continue
            for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                X, Y = x + dx, y + dy
                if 0 <= X < 16 and 0 <= Y < 16 and g[Y][X] not in (0, 4, ink):
                    out[y][x] = ink; break
    return out

# ------------------------------------------------------------------ bosque
def flores():
    g = blank()
    for (cx, cy, pet) in ((4, 6, 6), (11, 4, 7), (9, 11, 8)):
        for y in range(cy + 2, min(16, cy + 5)): px(g, cx, y, 9)
        px(g, cx + 1, cy + 3, 10)
        for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)): px(g, cx + dx, cy + dy, pet)
        px(g, cx, cy, 7 if pet != 7 else 11)
    return outline(g, 15)

def seta():
    g = blank()
    shadow(g, 8, 14, 5)
    for y in range(9, 14):
        for x in range(6, 10): g[y][x] = 14
    ellipse(g, 8, 8, 6.5, 4.2, 13)
    for y in range(9, 12): g[y][5] = 0 if g[y][5] == 13 and y > 9 else g[y][5]
    for (x, y) in ((6, 6), (10, 7), (8, 5), (4, 8), (12, 9)): px(g, x, y, 8)
    for x in range(3, 13):
        if g[10][x] == 13: g[10][x] = 12 if x in (3, 12) else g[10][x]
    return outline(g, 15)

def hierba():
    g = blank()
    shadow(g, 8, 14, 6)
    for i, (x, h) in enumerate(((3, 5), (5, 8), (7, 6), (8, 10), (10, 7), (12, 9), (13, 5))):
        for k in range(h):
            xx = x + (1 if k > h * 0.6 and i % 2 else 0) - (1 if k > h * 0.7 and not i % 2 else 0)
            px(g, xx, 14 - k, 10 if k > h // 2 else 9)
    return outline(g, 15)

def tocon():
    g = blank()
    shadow(g, 8, 14, 7)
    for y in range(6, 14):
        for x in range(3, 13): g[y][x] = 11 if (x + y // 3) % 5 else 12
    ellipse(g, 8, 6, 5.5, 2.6, 14)
    ellipse(g, 8, 6, 3.2, 1.4, 11)
    px(g, 8, 6, 12)
    for x in (2, 13): px(g, x, 13, 11)
    px(g, 12, 9, 10); px(g, 13, 8, 10)              # brote
    return outline(g, 15)

def piedras():
    g = blank()
    shadow(g, 6, 12, 4); shadow(g, 12, 9, 3)
    ellipse(g, 6, 10.5, 3.6, 2.4, 14); ellipse(g, 12, 7.8, 2.6, 2, 14)
    ellipse(g, 9, 13.5, 1.8, 1.3, 14)
    for (x, y) in ((5, 9), (11, 7)): px(g, x, y, 8)
    for (x, y) in ((7, 12), (13, 9)): px(g, x, y, 12)
    px(g, 3, 4, 10); px(g, 4, 3, 10); px(g, 4, 4, 9)  # trébol
    return outline(g, 15)

# ------------------------------------------------------------------ volcán (6 se anima: lava)
def grieta():
    g = blank()
    pts = [(1, 9), (4, 7), (6, 9), (9, 6), (11, 8), (14, 6)]
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        n = max(abs(x1 - x0), abs(y1 - y0))
        for k in range(n + 1):
            x = round(x0 + (x1 - x0) * k / n); y = round(y0 + (y1 - y0) * k / n)
            px(g, x, y - 1, 13); px(g, x, y + 1, 13); px(g, x, y, 6)
    for (x, y) in ((4, 7), (9, 6)): px(g, x, y, 7)
    return g

def roca_humo():
    g = blank()
    shadow(g, 8, 14, 6)
    ellipse(g, 8, 11, 5.5, 3.4, 9)
    ellipse(g, 7, 10, 3.6, 2.0, 8)
    px(g, 6, 9, 10); px(g, 7, 9, 10)
    for (x, y, r) in ((9, 5, 1.8), (11, 3, 1.4), (8, 2, 1.0)): ellipse(g, x, y, r, r, 11)
    return outline(g, 15)

def huesos():
    g = blank()
    shadow(g, 8, 12, 6)
    for k in range(10):
        px(g, 3 + k, 6 + k // 2, 12); px(g, 3 + k, 7 + k // 2, 12)
    for (x, y) in ((2, 5), (2, 8), (13, 10), (13, 13)): ellipse(g, x + .5, y + .5, 1.4, 1.4, 12)
    ellipse(g, 11, 4, 2.4, 2, 12); px(g, 10, 4, 15); px(g, 12, 4, 15)
    return outline(g, 15)

def cristales():
    # cristales de obsidiana con un brillo cálido (no parecen peligrosos, a diferencia de la lava)
    g = blank()
    shadow(g, 8, 14, 6)
    for (x0, h, w) in ((5, 8, 2), (8, 11, 3), (11, 7, 2)):
        for k in range(h):
            half = w if k < h - 2 else (1 if k < h - 1 else 0)
            for dx in range(-half, half + 1):
                px(g, x0 + dx, 13 - k, 10 if dx < 0 else (9 if dx > 0 else 8))
        px(g, x0, 13 - h + 2, 7)
    return outline(g, 15)

def piedras_v():
    g = blank()
    shadow(g, 6, 12, 4); shadow(g, 11, 8, 3)
    ellipse(g, 6, 10.5, 3.4, 2.2, 9); ellipse(g, 11, 7, 2.6, 1.8, 9)
    px(g, 5, 9, 10); px(g, 10, 6, 10)
    px(g, 4, 3, 6); px(g, 13, 13, 6)                # chispas de lava
    return outline(g, 15)

# ------------------------------------------------------------------ mar
def coral():
    g = blank()
    shadow(g, 8, 14, 5)
    def rama(x, y, h, lado):
        for k in range(h):
            px(g, x, y - k, 6)
            if k == h // 2: px(g, x + lado, y - k, 6); px(g, x + 2 * lado, y - k - 1, 6)
        px(g, x, y - h, 7)
    rama(8, 14, 9, 1); rama(6, 14, 6, -1); rama(10, 14, 5, 1)
    for (x, y) in ((8, 5), (4, 7), (12, 8)): px(g, x, y, 14)
    return outline(g, 15)

def concha():
    g = blank()
    shadow(g, 8, 13, 6)
    for y in range(16):
        for x in range(16):
            dx, dy = x + .5 - 8, y + .5 - 12
            r = math.hypot(dx, dy); a = math.atan2(dy, dx)
            if r < 7 and -math.pi < a < 0:
                g[y][x] = 9 if int((a + math.pi) / (math.pi / 6)) % 2 else 8
    ellipse(g, 8, 12.5, 2, 1.2, 9)
    return outline(g, 15)

def algas():
    g = blank()
    for i, x0 in enumerate((4, 8, 11)):
        h = (11, 13, 9)[i]
        for k in range(h):
            x = x0 + round(1.2 * math.sin(k / 2.0 + i))
            px(g, x, 15 - k, 10 if k % 3 else 11)
    return outline(g, 15)

def estrella_mar():
    g = blank()
    shadow(g, 8, 12, 6)
    for k in range(5):
        a = -math.pi / 2 + k * 2 * math.pi / 5
        for r in range(7):
            x = 8 + r * math.cos(a); y = 9 + r * math.sin(a)
            px(g, round(x), round(y), 12)
            if r < 4: px(g, round(x + .6), round(y + .3), 12)
    px(g, 8, 9, 14); px(g, 8, 6, 14); px(g, 11, 9, 14)
    return outline(g, 15)

def burbujas():
    g = blank()
    for (x, y, r) in ((5, 11, 2.6), (10, 6, 2.0), (12, 12, 1.5), (6, 4, 1.2)):
        ellipse(g, x, y, r, r, 0, edge=13)
        px(g, round(x - r / 2), round(y - r / 2), 13)
    return g

# ------------------------------------------------------------------ cielo (8 parpadea)
def nubecita():
    g = blank()
    for (x, y, rx, ry) in ((6, 10, 4, 2.6), (10, 9, 4, 3), (8, 7.5, 3, 2.6)): ellipse(g, x, y, rx, ry, 6)
    for x in range(16):
        for y in range(11, 16):
            if g[y][x] == 6: g[y][x] = 7
    return outline(g, 15)

def estrella():
    g = blank()
    for k in range(-5, 6):
        w = 2 if abs(k) < 2 else 1
        for d in range(-w + 1, w):
            px(g, 8 + k, 8 + d, 8); px(g, 8 + d, 8 + k, 8)
    px(g, 8, 8, 14)
    for (x, y) in ((3, 3), (13, 12), (13, 3)): px(g, x, y, 14)
    return g

def arcoiris():
    g = blank()
    for y in range(16):
        for x in range(16):
            r = math.hypot(x + .5 - 8, y + .5 - 15)
            if y < 15 and 4 <= r < 9:
                g[y][x] = (9, 10, 8, 11, 12)[min(4, int((r - 4)))]
    for (x, y, rx) in ((2, 14, 2.6), (14, 14, 2.6)): ellipse(g, x, y, rx, 1.8, 6)
    return g

def nubes_mini():
    g = blank()
    for (x, y) in ((5, 6), (11, 11)):
        for (dx, dy, rx, ry) in ((-1.5, .5, 2.4, 1.6), (1.5, .3, 2.4, 1.8), (0, -.8, 1.8, 1.6)):
            ellipse(g, x + dx, y + dy, rx, ry, 6)
        for xx in range(16):
            if g[min(15, y + 2)][xx] == 6: g[min(15, y + 2)][xx] = 7
    return outline(g, 15)

def estrellitas():
    g = blank()
    for (x, y) in ((4, 5), (11, 3), (8, 10), (13, 12), (3, 13)):
        px(g, x, y, 8); px(g, x - 1, y, 14); px(g, x + 1, y, 14); px(g, x, y - 1, 14); px(g, x, y + 1, 14)
    return g

DECOS = {
    'bosque':  [flores, seta, hierba, tocon, piedras],
    'volcan':  [grieta, roca_humo, huesos, cristales, piedras_v],
    'marino':  [coral, concha, algas, estrella_mar, burbujas],
    'celeste': [nubecita, estrella, arcoiris, nubes_mini, estrellitas],
}
NUM_DECO = 5
