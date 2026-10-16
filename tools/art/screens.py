#!/usr/bin/env python3
"""Full-screen 16-colour backgrounds for Flicki's Escape (title, menus, world intros, endings).
Characters are NOT baked in: the game puts them on top as sprites (own palettes), so each image
keeps a single 16-colour palette. Reserved indices in every image:
  13 = panel interior (also the opaque menu-font background), 14 = black/shadow, 15 = white.
Run from repo root: python3 tools/art/screens.py
"""
import os, sys, math
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import Image
import font5x7 as F

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
OUT = os.path.join(ROOT, 'res', 'gfx')
W, H = 320, 224
BAYER = np.array([[0, 8, 2, 10], [12, 4, 14, 6], [3, 11, 1, 9], [15, 7, 13, 5]]) / 16.0

def md(c): return tuple(min(7, round(v / 255 * 7)) * 36 for v in c)
def hsh(x, y, s=0): return ((x * 73856093) ^ (y * 19349663) ^ (s * 83492791)) & 0xffff

def save(img, pal, name):
    assert len(pal) == 16
    im = Image.fromarray(img.astype(np.uint8), 'P')
    flat = []
    for c in pal: flat += list(md(c))
    im.putpalette(flat + [0] * (768 - len(flat)))
    im.save(os.path.join(OUT, name))

def canvas(c=1): return np.full((H, W), c, dtype=np.int16)

def vgrad(img, y0, y1, cols):
    """vertical gradient through palette indices cols with ordered dithering."""
    n = len(cols) - 1
    for y in range(y0, y1):
        t = (y - y0) / max(1, (y1 - y0 - 1)) * n
        i = min(int(t), n - 1); f = t - i
        for x in range(W):
            img[y, x] = cols[i + 1] if f > BAYER[y % 4, x % 4] else cols[i]

def silhouette(img, fn, color, y_bottom=H):
    for x in range(W):
        top = int(fn(x))
        img[max(0, top):y_bottom, x] = color

def disc(img, cx, cy, r, color, squash=1.0):
    for y in range(int(cy - r * squash) - 1, int(cy + r * squash) + 2):
        for x in range(int(cx - r) - 1, int(cx + r) + 2):
            if 0 <= x < W and 0 <= y < H and ((x - cx) ** 2 + ((y - cy) / squash) ** 2) <= r * r:
                img[y, x] = color

def rect(img, x0, y0, x1, y1, color):
    img[max(0, y0):min(H, y1), max(0, x0):min(W, x1)] = color

def panel(img, x0, y0, x1, y1, border=9, light=10, inner=13, r=6):
    """rounded panel: dark outline, 2px coloured border with highlight, flat interior."""
    for y in range(y0, y1):
        for x in range(x0, x1):
            dx = max(x0 + r - x, 0, x - (x1 - 1 - r)); dy = max(y0 + r - y, 0, y - (y1 - 1 - r))
            d = math.hypot(dx, dy)
            if d > r: continue
            if d > r - 1.2 or x in (x0, x1 - 1) and dy == 0 or y in (y0, y1 - 1) and dx == 0: img[y, x] = 14
            elif d > r - 3.4 or x < x0 + 3 or x > x1 - 4 or y < y0 + 3 or y > y1 - 4:
                img[y, x] = light if (y < y0 + 2 or x < x0 + 2) else border
            else: img[y, x] = inner
    # drop shadow
    for y in range(y0 + 3, min(H, y1 + 3)):
        for x in range(x1, min(W, x1 + 3)):
            if img[y, x] not in (14,): img[y, x] = 14
    for x in range(x0 + 3, min(W, x1 + 3)):
        for y in range(y1, min(H, y1 + 3)):
            img[y, x] = 14

def glyph(ch):
    g = F.G.get(ch) or F.G.get(ch.upper()) or F.G[' ']
    return g[:7]

def text_width(txt, s, gap=1): return len(txt) * (5 + gap) * s - gap * s

def big_text(img, txt, x, y, s, fills, outline=9, shadow=14, hi=15, center=False):
    """chunky logo text: glyph pixels scaled by s, banded fill, thick outline, drop shadow, top highlight."""
    if center: x = (W - text_width(txt, s)) // 2
    mask = np.zeros((H, W), bool)
    band = np.zeros((H, W), np.int16)
    cx = x
    for ch in txt:
        g = glyph(ch)
        for gy, row in enumerate(g):
            for gx, c in enumerate(row):
                if c == '#':
                    y0, x0 = y + gy * s, cx + gx * s
                    mask[max(0, y0):y0 + s, max(0, x0):x0 + s] = True
                    band[max(0, y0):y0 + s, max(0, x0):x0 + s] = min(len(fills) - 1, gy * len(fills) // 7)
        cx += 6 * s
    o = max(2, s // 2)
    dil = np.zeros_like(mask)
    for dy in range(-o, o + 1):
        for dx in range(-o, o + 1):
            if dx * dx + dy * dy <= o * o + 1:
                dil |= np.roll(np.roll(mask, dy, 0), dx, 1)
    sh = np.roll(np.roll(dil, o + 1, 0), o + 1, 1)
    img[sh & ~dil] = shadow
    img[dil & ~mask] = outline
    for b, col in enumerate(fills):
        img[mask & (band == b)] = col
    # 1px highlight on the top edge of every stroke
    top = mask & ~np.roll(mask, 1, 0)
    img[top] = hi

def small_text(img, txt, x, y, color=15, shadow=14, center=False):
    if center: x = (W - text_width(txt, 1)) // 2
    for i, ch in enumerate(txt):
        g = glyph(ch)
        for gy, row in enumerate(g):
            for gx, c in enumerate(row):
                if c == '#':
                    px, py = x + i * 6 + gx, y + gy
                    if 0 <= px + 1 < W and 0 <= py + 1 < H: img[py + 1, px + 1] = shadow
        for gy, row in enumerate(g):
            for gx, c in enumerate(row):
                if c == '#':
                    px, py = x + i * 6 + gx, y + gy
                    if 0 <= px < W and 0 <= py < H: img[py, px] = color

def stars(img, region, color, n, seed):
    x0, y0, x1, y1 = region
    for i in range(n):
        x = x0 + hsh(i, 1, seed) % (x1 - x0); y = y0 + hsh(i, 2, seed) % (y1 - y0)
        img[y, x] = color

# ----------------------------------------------------------------------------- scenes
def forest_scene(img, ground_y=168):
    vgrad(img, 0, 120, [1, 2, 3])
    disc(img, 262, 40, 16, 4)                                   # soft sun
    disc(img, 262, 40, 12, 15)
    silhouette(img, lambda x: 112 + 10 * math.sin(x / 37.0) + 6 * math.sin(x / 13.0 + 1), 5)          # far hills
    # tree line: round canopies
    for i in range(-1, 18):
        cx = i * 20 + (hsh(i, 3) % 10); r = 14 + hsh(i, 4) % 6
        disc(img, cx, 132 - (hsh(i, 5) % 8), r, 6)
    rect(img, 0, 138, W, ground_y, 6)
    # hedge row with highlights
    for x in range(0, W, 12):
        disc(img, x + 6, ground_y - 6, 9, 7)
    for x in range(0, W, 12):
        for k in range(3):
            px = x + 2 + hsh(x, k) % 9; py = ground_y - 12 + hsh(x, k + 9) % 6
            if px < W: img[py, px] = 8
    rect(img, 0, ground_y, W, H, 8)
    for y in range(ground_y, H):
        for x in range(W):
            if hsh(x // 2, y // 2, 7) % 100 < 12: img[y, x] = 7
    rect(img, 0, ground_y, W, ground_y + 2, 7)
    # little flowers
    for i in range(40):
        x = hsh(i, 11) % W; y = ground_y + 6 + hsh(i, 12) % (H - ground_y - 8)
        img[y, x] = 15 if i % 3 else 10

def volcano_scene(img, ground_y=168):
    vgrad(img, 0, 150, [1, 2, 3])
    stars(img, (0, 0, W, 60), 15, 25, 3)
    # volcano cone
    silhouette(img, lambda x: max(70 + abs(x - 230) * 1.15, 70), 4)
    # crater glow + lava rivers
    disc(img, 230, 72, 16, 12, 0.35)
    for k, (sx, dx) in enumerate(((226, -1), (236, 1))):
        x = sx
        for y in range(74, ground_y):
            x += dx * (0.5 + 0.4 * math.sin(y / 9.0 + k))
            w = 2 + (y - 74) // 30
            rect(img, int(x) - w // 2, y, int(x) + w // 2 + 1, y + 1, 11 if (y // 3) % 3 else 10)
    # smoke puffs
    for i, (cx, cy, r) in enumerate(((222, 52, 9), (234, 40, 12), (222, 24, 14), (244, 14, 10))):
        disc(img, cx, cy, r, 5); disc(img, cx - 3, cy - 3, r - 4, 6)
    # foreground rocks
    silhouette(img, lambda x: ground_y - 8 + 5 * math.sin(x / 11.0) + 3 * math.sin(x / 5.0), 7)
    rect(img, 0, ground_y, W, H, 7)
    for y in range(ground_y - 12, H):
        for x in range(W):
            if img[y, x] == 7 and hsh(x // 3, y // 3, 5) % 100 < 18: img[y, x] = 4
    for i in range(30):   # embers
        x = hsh(i, 21) % W; y = 90 + hsh(i, 22) % 70
        if img[y, x] in (1, 2, 3): img[y, x] = 10 if i % 2 else 11

def sea_scene(img, ground_y=172):
    vgrad(img, 0, ground_y, [3, 2, 1])
    # light rays
    for k in range(5):
        x0 = 30 + k * 70
        for y in range(0, ground_y - 20):
            xs = x0 + y // 3
            for x in range(xs, xs + 10 - y // 30):
                if 0 <= x < W and BAYER[y % 4, x % 4] < 0.35 - y / 600: img[y, x] = 4
    # sand
    silhouette(img, lambda x: ground_y - 6 + 4 * math.sin(x / 23.0), 5)
    for y in range(ground_y - 10, H):
        w = (y + int(2 * math.sin(y))) % 7
        for x in range(W):
            if img[y, x] == 5 and (y + int(2 * math.sin(x / 4.0))) % 7 == 0: img[y, x] = 6
    # seaweed
    for i in range(9):
        bx = 12 + i * 36 + hsh(i, 31) % 14; h = 30 + hsh(i, 32) % 40
        for y in range(h):
            x = bx + int(3 * math.sin((y + i * 7) / 6.0))
            yy = ground_y - y
            rect(img, x, yy, x + 3, yy + 1, 7 if y % 6 < 4 else 8)
    # coral clumps
    for i in range(5):
        cx = 40 + i * 64 + hsh(i, 41) % 20
        for r, c in ((9, 9), (6, 10)):
            disc(img, cx, ground_y - 2, r, c, 0.8)
    # bubbles
    for i in range(26):
        x = hsh(i, 51) % W; y = 20 + hsh(i, 52) % (ground_y - 40); r = 1 + hsh(i, 53) % 3
        disc(img, x, y, r, 4); img[int(y - r // 2), int(x - r // 2)] = 15

def sky_scene(img, ground_y=176):
    vgrad(img, 0, H, [1, 2, 3, 4])
    disc(img, 56, 46, 22, 10); disc(img, 56, 46, 17, 11); disc(img, 52, 42, 6, 15)
    # cloud banks
    def cloud(cx, cy, s):
        for k, (dx, dy, r) in enumerate(((-2, 0, 1.0), (-1, -0.5, 1.2), (0, -0.8, 1.4), (1, -0.4, 1.1), (2, 0, 0.9))):
            disc(img, cx + dx * 10 * s, cy + dy * 10 * s, r * 10 * s, 6)
        for k, (dx, dy, r) in enumerate(((-1, -0.7, 1.0), (0, -1.0, 1.2), (1, -0.6, 0.9))):
            disc(img, cx + dx * 10 * s - 2, cy + dy * 10 * s - 3, r * 9 * s, 15)
        rect(img, int(cx - 25 * s), int(cy), int(cx + 25 * s), int(cy + 6 * s), 6)
    cloud(250, 60, 1.0); cloud(160, 108, 0.7); cloud(40, 130, 0.8)
    # ground of clouds at the bottom
    for x in range(-10, W + 20, 22):
        disc(img, x, ground_y + 4, 16 + hsh(x, 1) % 6, 6)
    for x in range(-10, W + 20, 22):
        disc(img, x - 3, ground_y, 11 + hsh(x, 2) % 5, 15)
    rect(img, 0, ground_y + 12, W, H, 6)
    stars(img, (0, 0, W, 90), 15, 18, 9)

def night_scene(img, ground_y=176):
    vgrad(img, 0, ground_y, [1, 2, 3])
    stars(img, (0, 0, W, 110), 15, 60, 4)
    disc(img, 286, 20, 12, 4); disc(img, 291, 17, 10, 1)          # crescent moon
    for i in range(-1, 18):
        cx = i * 20 + (hsh(i, 3) % 10); r = 14 + hsh(i, 4) % 8
        disc(img, cx, ground_y - 22 - (hsh(i, 5) % 12), r, 5)
    rect(img, 0, ground_y - 22, W, ground_y, 5)
    rect(img, 0, ground_y, W, H, 6)
    for y in range(ground_y, H):
        for x in range(W):
            if hsh(x // 2, y // 2, 7) % 100 < 10: img[y, x] = 7

def pattern_bg(img, a, b, c):
    """menu backdrop: diagonal stripes with a subtle leaf motif."""
    for y in range(H):
        for x in range(W):
            img[y, x] = a if ((x + y) // 8) % 2 else b
    for i in range(70):
        x = hsh(i, 61) % W; y = hsh(i, 62) % H
        disc(img, x, y, 2, c, 0.6)

# ----------------------------------------------------------------------------- palettes
GOLD = [(120, 60, 20), (255, 240, 120), (255, 200, 60), (232, 120, 32)]      # outline, fills (3)
def pal(scene, logo=GOLD, panel_bg=(24, 32, 56)):
    p = [(0, 0, 0)] + scene                 # 1..8 scene
    while len(p) < 9: p.append((0, 0, 0))
    p = p[:9] + logo                        # 9 outline, 10..12 fills
    return p + [panel_bg, (0, 0, 0), (255, 255, 255)]

FOREST = [(120, 184, 248), (160, 210, 252), (200, 232, 255), (255, 240, 180), (110, 160, 150),
          (40, 110, 60), (70, 150, 60), (110, 190, 80)]
VOLC = [(40, 16, 40), (96, 24, 32), (160, 48, 40), (60, 36, 40), (90, 80, 90), (130, 120, 130),
        (40, 28, 32), (0, 0, 0)]
SEA = [(16, 48, 110), (32, 96, 160), (64, 150, 200), (130, 200, 230), (226, 206, 150), (200, 176, 120),
       (40, 140, 80), (24, 100, 60)]
SKY = [(56, 96, 200), (90, 140, 230), (140, 186, 250), (190, 220, 255), (0, 0, 0), (210, 220, 245),
       (0, 0, 0), (0, 0, 0)]
NIGHT = [(16, 16, 48), (32, 32, 80), (56, 56, 112), (255, 240, 190), (16, 40, 40), (24, 56, 40),
         (40, 80, 50), (0, 0, 0)]

def nest(img, cx, cy, weave=(11, 12), dark=9, egg=15, shade=4):
    """woven nest with three eggs, drawn with scene palette indices."""
    for y in range(cy - 14, cy + 8):
        for x in range(cx - 24, cx + 25):
            d = ((x - cx) / 24.0) ** 2 + ((y - cy) / 8.0) ** 2
            if d < 1 and y >= cy - 4:
                img[y, x] = dark if d > 0.78 else (weave[0] if (x + 2 * y) % 5 else weave[1])
    for ex in (cx - 9, cx, cx + 9):
        disc(img, ex, cy - 7 - (2 if ex == cx else 0), 5, egg, 1.25)
        img[cy - 10, ex - 2] = 15

def story_panel(img):
    # aligned to the 8x8 tile grid: columns 1-38, rows 21-26 (text goes on rows 22 and 24, columns 3-36)
    panel(img, 8, 168, 312, 216)

def variants(es, en):
    return (('', es), ('_en', en))

def main():
    # ---------------- title (same in both languages)
    img = canvas(); forest_scene(img, ground_y=176)
    big_text(img, "FLICKI'S", 0, 18, 5, [10, 11, 12], center=True)
    big_text(img, "ESCAPE", 0, 72, 5, [10, 11, 12], center=True)
    save(img, pal(FOREST), 'bg_title.png')

    # ---------------- language select
    img = canvas(); forest_scene(img, ground_y=176)
    big_text(img, "FLICKI'S ESCAPE", 0, 20, 3, [10, 11, 12], center=True)
    panel(img, 64, 80, 256, 160)
    save(img, pal(FOREST), 'bg_idioma.png')

    # ---------------- menus
    MENU = [(40, 92, 64), (48, 104, 72), (64, 128, 84), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0), (0, 0, 0)]
    for suf, t in variants("OPCIONES", "OPTIONS"):
        img = canvas(); pattern_bg(img, 1, 2, 3)
        panel(img, 8, 38, 312, 218)
        big_text(img, t, 0, 9, 3, [10, 11, 12], center=True)
        save(img, pal(MENU), f'bg_opciones{suf}.png')

    img = canvas(); pattern_bg(img, 1, 2, 3)
    panel(img, 16, 38, 304, 218)
    big_text(img, "VERSUS", 0, 9, 3, [10, 11, 12], center=True)
    save(img, pal([(80, 30, 40), (100, 40, 52), (130, 56, 64)] + [(0, 0, 0)] * 5), 'bg_versus.png')

    for suf, t in variants("COMO JUGAR", "HOW TO PLAY"):
        img = canvas(); pattern_bg(img, 1, 2, 3)
        panel(img, 8, 38, 312, 218)
        big_text(img, t, 0, 9, 3, [10, 11, 12], center=True)
        save(img, pal(MENU), f'bg_como_jugar{suf}.png')

    # ---------------- world intros
    WORLD = ("MUNDO", "WORLD")
    for k, (suf, _) in enumerate(variants(0, 1)):
        w = WORLD[k]
        img = canvas(); forest_scene(img)
        big_text(img, w + " 1", 0, 14, 3, [10, 11, 12], center=True)
        big_text(img, ("BOSQUE", "FOREST")[k], 0, 44, 5, [10, 11, 12], center=True)
        save(img, pal(FOREST), f'bg_intro_bosque{suf}.png')

        img = canvas(); volcano_scene(img)
        big_text(img, w + " 2", 0, 14, 3, [10, 11, 12], outline=14, center=True)
        big_text(img, ("VOLCAN", "VOLCANO")[k], 0, 44, 5, [10, 11, 12], outline=14, center=True)
        save(img, pal(VOLC, logo=[(60, 10, 10), (255, 240, 120), (255, 160, 40), (232, 64, 24)]), f'bg_intro_volcan{suf}.png')

        img = canvas(); sea_scene(img)
        big_text(img, w + " 3", 0, 14, 3, [15, 11, 12], center=True)
        big_text(img, ("MAR", "SEA")[k], 0, 44, 5, [15, 11, 12], center=True)
        save(img, pal(SEA, logo=[(16, 32, 80), (255, 140, 160), (180, 230, 255), (90, 180, 240)]), f'bg_intro_acuatico{suf}.png')

        img = canvas(); sky_scene(img)
        big_text(img, w + " 4", 0, 14, 3, [12, 10, 11], center=True)
        big_text(img, ("CIELO", "SKY")[k], 0, 44, 5, [12, 10, 11], center=True)
        save(img, pal(SKY, logo=[(60, 40, 120), (255, 220, 100), (255, 180, 60), (255, 250, 220)]), f'bg_intro_celeste{suf}.png')

    # ---------------- endings
    img = canvas(); night_scene(img)
    big_text(img, "GAME OVER", 0, 40, 5, [10, 11, 12], center=True)
    save(img, pal(NIGHT, logo=[(60, 10, 20), (255, 140, 140), (232, 64, 64), (160, 24, 40)]), 'bg_game_over.png')

    img = canvas(); night_scene(img)
    big_text(img, "CONTINUE?", 0, 40, 5, [10, 11, 12], center=True)
    save(img, pal(NIGHT, logo=GOLD), 'bg_continue.png')

    SKYLOGO = [(60, 40, 120), (255, 220, 100), (255, 180, 60), (255, 250, 220)]
    for suf, (t, sub, sc) in variants(("ENHORABUENA!", "HAS SALVADO A TODOS LOS POLLUELOS", 4),
                                     ("CONGRATULATIONS!", "YOU SAVED EVERY SINGLE CHICK", 3)):
        img = canvas(); sky_scene(img)
        big_text(img, t, 0, 24 + (4 if sc == 3 else 0), sc, [12, 10, 11], center=True)
        small_text(img, sub, 0, 72, center=True)
        save(img, pal(SKY, logo=SKYLOGO), f'bg_fin_juego{suf}.png')

    for suf, (a, b) in variants(("GANA EL", "PAJARO!"), ("THE BIRD", "WINS!")):
        img = canvas(); forest_scene(img)
        big_text(img, a, 0, 16, 3, [10, 11, 12], center=True)
        big_text(img, b, 0, 44, 5, [10, 11, 12], center=True)
        save(img, pal(FOREST), f'bg_pajaro_ganador{suf}.png')
    for suf, (a, b) in variants(("GANA EL", "CAZADOR!"), ("THE HUNTER", "WINS!")):
        img = canvas(); night_scene(img)
        big_text(img, a, 0, 16, 3, [10, 11, 12], center=True)
        big_text(img, b, 0, 44, 5, [10, 11, 12], center=True)
        save(img, pal(NIGHT, logo=[(60, 10, 20), (255, 200, 120), (232, 120, 48), (160, 60, 30)]), f'bg_cazador_ganador{suf}.png')

    # ---------------- arcade story (text is drawn by the game inside the bottom panel)
    img = canvas(); forest_scene(img, ground_y=168)
    nest(img, 160, 160)
    story_panel(img)
    save(img, pal(FOREST), 'bg_hist1.png')

    img = canvas(); night_scene(img, ground_y=168)
    nest(img, 160, 160, weave=(11, 12), dark=9, egg=15)
    story_panel(img)
    save(img, pal(NIGHT, logo=[(80, 50, 30), (255, 230, 150), (150, 100, 60), (110, 70, 40)]), 'bg_hist2.png')

    MAP = [(236, 214, 160), (214, 186, 128), (160, 200, 240), (60, 110, 200), (200, 60, 40), (40, 110, 60),
           (90, 170, 70), (150, 140, 130)]
    img = canvas(1)
    for y in range(H):
        for x in range(W):
            if hsh(x // 3, y // 3, 17) % 100 < 22: img[y, x] = 2
    rect(img, 6, 6, W - 6, 8, 9); rect(img, 6, 164, W - 6, 166, 9); rect(img, 6, 6, 8, 166, 9); rect(img, W - 8, 6, W - 6, 166, 9)
    # dotted path linking the four worlds
    pts = [(54, 120), (130, 64), (200, 118), (270, 58)]
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        for k in range(0, 40):
            if k % 3 == 0: continue
            t = k / 40.0
            x = int(x0 + (x1 - x0) * t); y = int(y0 + (y1 - y0) * t - 18 * math.sin(math.pi * t))
            rect(img, x, y, x + 2, y + 2, 9)
    # forest: three trees
    for dx in (-14, 0, 14):
        rect(img, 54 + dx - 1, 124, 54 + dx + 2, 134, 9)
        disc(img, 54 + dx, 116, 9, 6); disc(img, 52 + dx, 113, 5, 7)
    # volcano
    silhouette_pts = [(x, 74 - max(0, 22 - abs(x - 130))) for x in range(100, 161)]
    for x, top in silhouette_pts:
        rect(img, x, top + 18, x + 1, 86, 8)
    disc(img, 130, 54, 5, 12); disc(img, 126, 46, 4, 11); rect(img, 128, 60, 132, 76, 5)
    # sea: waves
    for k in range(3):
        for x in range(176, 226):
            y = 112 + k * 7 + int(2 * math.sin((x + k * 4) / 3.0))
            rect(img, x, y, x + 1, y + 2, 4 if k % 2 == 0 else 3)
    # sky: cloud
    disc(img, 262, 58, 10, 15); disc(img, 276, 60, 8, 15); disc(img, 250, 62, 7, 15)
    rect(img, 246, 62, 284, 68, 15); rect(img, 246, 68, 284, 69, 3)
    big_text(img, "?", 0, 26, 3, [10, 11, 12], center=True) if False else None
    story_panel(img)
    save(img, pal(MAP, logo=[(90, 50, 20), (255, 230, 120), (255, 160, 60), (232, 96, 32)]), 'bg_hist3.png')

    img = canvas(); sky_scene(img, ground_y=150)
    story_panel(img)
    save(img, pal(SKY, logo=SKYLOGO), 'bg_hist4.png')
    print('screens written to', OUT)

if __name__ == '__main__':
    main()
