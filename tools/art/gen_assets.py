#!/usr/bin/env python3
"""Generate Flicki's Escape 16-bit redesign assets (indexed PNGs for SGDK rescomp).
Run from repo root:  python3 tools/art/gen_assets.py
"""
import os, sys, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import Image
import hero_views as HV, cast as CAST, font5x7 as F, deco as DECO

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
OUT = os.path.join(ROOT, 'res', 'gen')
os.makedirs(OUT, exist_ok=True)

def md(c): return tuple(min(7, round(v/255*7))*36 for v in c)
def hsh(x, y, s=0): return ((x*73856093) ^ (y*19349663) ^ (s*83492791)) & 0xffff

def save_indexed(path, pix, w, h, pal):
    """pix: list of rows of palette indices; pal: list of up to 16 RGB (index 0 = transparent)."""
    assert len(pal) <= 16, (path, len(pal))
    im = Image.new('P', (w, h))
    flat = []
    for c in pal: flat += list(md(c))
    flat += [0]*(768-len(flat))
    im.putpalette(flat)
    im.putdata([v for row in pix for v in row])
    im.save(path)

# ------------------------------------------------------------------ WORLD THEMES
WORLDS = {
 'bosque': dict(   # setos frondosos con cara frontal y florecillas; césped con manchas de sol
   wall=[(44,124,52),(28,86,40),(76,160,60),(140,206,96),(16,52,30)],     # mid,dark,light,hi,edge
   nest=[(232,190,120),(176,112,48),(120,72,32)],
   hud=[(90,56,30),(240,200,120),(250,226,170)],
   floor=[(132,188,74),(176,214,100),(104,156,58)], shadow=(84,128,50), shadow2=(70,108,44),
 ),
 'volcan': dict(   # basalt blocks with glowing lava seams on dark ash
   wall=[(92,64,72),(56,36,48),(128,92,96),(255,140,40),(32,18,28)],
   nest=[(200,160,110),(150,96,56),(96,56,40)],
   hud=[(64,24,24),(255,150,60),(255,210,150)],
   floor=[(84,60,60),(98,70,66),(72,50,52)], shadow=(56,38,42), shadow2=(44,28,34),
 ),
 'marino': dict(   # teal reef rocks with pink coral on rippled sand
   wall=[(56,128,150),(32,84,112),(96,172,190),(255,130,150),(20,52,80)],
   nest=[(240,220,170),(200,160,100),(140,100,60)],
   hud=[(24,60,100),(120,220,240),(200,240,255)],
   floor=[(226,206,150),(238,220,170),(206,184,130)], shadow=(184,160,112), shadow2=(160,136,96),
 ),
 'celeste': dict(  # puffy clouds on open sky
   wall=[(224,236,255),(170,190,232),(255,255,255),(255,240,200),(130,150,210)],
   nest=[(240,210,140),(200,150,80),(150,100,50)],
   hud=[(56,72,140),(255,220,120),(230,236,255)],
   floor=[(128,184,240),(144,198,248),(116,170,230)], shadow=(100,150,214), shadow2=(86,132,196),
 ),
}
# PAL0 layout: 0 transp,1-5 wall,6-8 nest,9-11 hud,12 egg,13 free,14 shadow,15 white
def pal0(t):
    return [(0,0,0)] + t['wall'] + t['nest'] + t['hud'] + [(250,250,240),(200,200,190),(0,0,0),(255,255,255)]
def pal1(t, name=None):
    base = [(0,0,0)] + t['floor'] + [t['shadow'], t['shadow2']]
    return base + (DECO.DECO_PAL[name] if name else [])

# ------------------------------------------------------------------ WALL AUTOTILES (PAL0)
def wall_pixel(mask, xx, yy, variant, world='bosque'):
    up, ri, dn, le = mask & 1, mask & 2, mask & 4, mask & 8
    def out(cx, cy, r2=30): return (xx-cx)**2 + (yy-cy)**2 > r2
    r2 = 42 if world == 'celeste' else 30
    if (not up and not le and xx < 6 and yy < 6 and out(6, 6, r2)) or (not up and not ri and xx > 9 and yy < 6 and out(9, 6, r2)) \
       or (not dn and not le and xx < 6 and yy > 9 and out(6, 9, r2)) or (not dn and not ri and xx > 9 and yy > 9 and out(9, 9, r2)):
        return 0
    v = hsh(xx+variant*17, yy+variant*31, 3) % 100
    if world == 'volcan':
        # stone blocks, staggered; seams glow with lava now and then
        row = yy // 8; jx = 4 if row % 2 else 12
        seam_h = (yy % 8) == 7; seam_v = (xx % 16) == jx
        if seam_h or seam_v:
            c = 4 if hsh(xx // 4, yy // 4 + variant, 7) % 100 < 45 else 5
        else:
            c = 3 if (yy % 8) == 0 else (2 if v < 22 else 1)
            if (xx % 16) == (jx + 1) % 16: c = 3
        if not up and yy < 2: c = 3
        if not dn and yy > 13: c = 5
        return c
    if world == 'marino':
        c = 1 if v < 55 else (2 if v < 75 else 3)
        # little coral polyps and bubbles of light
        if hsh(xx // 3, yy // 3, variant + 9) % 100 < 9 and (xx % 3) and (yy % 3): c = 4
        if not up and yy < 3: c = 3
        if not dn and yy > 12: c = 5 if yy > 13 else 2
        return c
    if world == 'celeste':
        # puffy cloud: scalloped outline on exposed sides, bright top, lilac underside
        def bump(t): return int(round(2.6 * math.sin((t + 0.5) * math.pi / 16)))
        if not up and yy < 3 - bump(xx): return 0
        if not dn and yy > 12 + bump(xx): return 0
        if not le and xx < 3 - bump(yy): return 0
        if not ri and xx > 12 + bump(yy): return 0
        c = 1
        if not up and yy < 6 - bump(xx): c = 3
        if not dn and yy > 9 + bump(xx): c = 2
        if not dn and yy > 11 + bump(xx): c = 5
        if not le and xx < 4 - bump(yy) and c == 1: c = 3
        return c
    if world == 'bosque':
        # seto frondoso: bultos de hojas, cara frontal oscura abajo (da altura) y alguna florecilla
        if not dn and yy >= 11:
            if yy == 15: return 5
            return 2 if (xx + yy // 2) % 4 else 5
        bx = (xx + (3 if (yy // 5) % 2 else 0)) % 6; by = yy % 5
        d = (bx - 3) ** 2 + (by - 2) ** 2
        c = 1
        if d <= 2: c = 3
        if d <= 0: c = 4
        if d >= 7: c = 2
        if not up and yy < 2: c = 4 if xx % 3 else 3
        if not le and xx < 1: c = 2
        if not ri and xx > 14: c = 5
        k = hsh(xx // 3 + variant * 7, yy // 3 + variant * 3, 21) % 100
        if k < 4 and 2 < yy < 10 and (xx + yy) % 3 == 0: c = 12 if k < 2 else 10
        return c
    c = 2 if v < 18 else (1 if v < 55 else 3)          # dark / mid / light
    if v > 94: c = 4                                     # highlight sparkle
    if not up and yy < 3: c = 4 if v < 50 else 3
    if not dn and yy > 12: c = 5 if yy > 13 else 2
    return c

def floor_pixel(world, x, y, tex):
    if world == 'marino':       # sand ripples
        w = (y + int(2*math.sin((x + tex*5) / 3.0))) % 6
        c = 2 if w == 0 else (3 if w == 1 else 1)
        if hsh(x, y, tex+3) % 100 < 4: c = 3
        return c
    if world == 'celeste':      # open sky, faint sparkles
        v = hsh(x//2 + tex*8, y//2 + tex*5, 11) % 100
        c = 1 if v > 10 else (2 if v < 6 else 3)
        return c
    if world == 'volcan':       # ash with scattered pebbles
        v = hsh(x + tex*8, y + tex*5, 13) % 100
        return 1 if v > 18 else (3 if v < 9 else 2)
    # bosque: patrón continuo de 32x32 (la textura 0..3 es el cuarto de 16x16 que toca) con manchas de sol
    X = x + (tex & 1) * 16; Y = y + (tex >> 1) * 16
    a = 2 * math.pi / 32
    v = math.sin(a * X) * math.cos(a * Y) + 0.6 * math.sin(a * (X + 2 * Y) + 1.3) + 0.4 * math.cos(a * (2 * X - Y) + 0.7)
    c = 1
    if v > 1.15 or (v > 1.0 and (X + Y) % 2): c = 2
    if ((X * 7 + (Y // 3) * 13) % 23) == 0 and (Y % 3) != 2: c = 3
    return c

def metatile_to_tiles(get):
    """get(x,y)->index for 16x16; returns 4 tiles (TL,TR,BL,BR) each 8 rows of 8."""
    tiles = []
    for ty in (0, 8):
        for tx in (0, 8):
            tiles.append([[get(tx+x, ty+y) for x in range(8)] for y in range(8)])
    return tiles

def strip(tiles):
    rows = []
    for t in tiles: rows += t
    return rows

# Arena del bosque ("claro"): árboles grandes, tronco caído y follaje para el borde (PAL0 del bosque:
# 1-5 verdes del seto, 6-8 marrones del nido)
def claro_tiles():
    def arbol(X, Y):
        dx, dy = X - 15.5, Y - 12.5
        d = (dx / 14.5) ** 2 + (dy / 12.0) ** 2
        # tronco
        if 12 <= X <= 19 and Y >= 20:
            if Y > 29 and (X < 13 or X > 18): return 0
            return 6 if X < 14 else (8 if X > 17 else 7)
        if 9 <= X <= 22 and Y >= 29 and abs(X - 15.5) < 6.5 - (31 - Y) * 2: return 8      # raíces
        if d >= 1: return 0
        # copa: bultos de hojas con luz arriba a la izquierda
        n = hsh(X // 3, Y // 3, 5) % 100
        bump = ((X % 6) - 3) ** 2 + ((Y % 5) - 2) ** 2
        if d > 0.86: return 5
        luz = (-dx - dy) / 20.0
        c = 1
        if luz > 0.25 or (luz > 0 and bump < 3): c = 3
        if luz < -0.35 or (bump > 7 and n < 40): c = 2
        if luz > 0.45 and n < 30: c = 4
        return c
    def tronco(X, Y):
        if Y < 4 or Y > 13: return 0
        if X < 5:                                            # corte con anillos
            r = math.hypot(X - 4, Y - 8.5)
            if r > 4.6: return 0
            return 5 if r > 4 else (8 if int(r) % 2 else 6)
        if X > 30 and (Y < 5 or Y > 12): return 0
        if Y == 4 or Y == 13: return 5
        c = 7 if (X + (Y // 3) * 5) % 9 else 8
        if Y < 7: c = 6 if (X % 7) else 7
        if hsh(X // 2, Y // 2, 3) % 100 < 10: c = 3           # musgo
        return c
    def follaje(X, Y, v):
        n = hsh(X // 2 + v * 5, Y // 2 + v * 3, 9) % 100
        bump = ((X + v * 3) % 8 - 4) ** 2 + ((Y + v * 5) % 7 - 3) ** 2
        if bump < 3: return 3 if n < 70 else 4
        if bump > 10: return 5 if n < 50 else 2
        return 1 if n < 70 else 2
    t = []
    for (ox, oy) in ((0, 0), (16, 0), (0, 16), (16, 16)):
        t += metatile_to_tiles(lambda x, y, ox=ox, oy=oy: arbol(x + ox, y + oy))
    for ox in (0, 16):
        t += metatile_to_tiles(lambda x, y, ox=ox: tronco(x + ox, y))
    for v in (0, 1):
        t += metatile_to_tiles(lambda x, y, v=v: follaje(x, y, v))
    return t

def gen_world(name):
    t = WORLDS[name]
    tiles = []
    for mask in range(16):
        tiles += metatile_to_tiles(lambda x, y, m=mask: wall_pixel(m, x, y, m, name))
    # nest: two metatiles (left/right) woven straw with eggs
    def nest_px(x, y, side, n):
        X = x + side*16 + 0.5; Y = y + 0.5
        # eggs sitting in the bowl (0..3: the nest fills up as chicks come home)
        eggs = {0: (), 1: ((16, 5.5),), 2: ((10.5, 6.5), (21.5, 6.5)), 3: ((10.5, 6.5), (16, 5.5), (21.5, 6.5))}[n]
        for ex, ey in eggs:
            d = ((X-ex)/3.2)**2 + ((Y-ey)/3.8)**2
            if d < 1 and Y < 9.5:
                if d > 0.72: return 13                     # egg outline/shade
                if (X-ex) < -0.8 and (Y-ey) < -0.8: return 15  # highlight
                return 13 if (X-ex) > 1.2 else 12
        # woven bowl: outer ellipse
        d = ((X-16)/15.5)**2 + ((Y-10)/5.8)**2
        if d >= 1 or Y < 6: return 0
        inner = ((X-16)/11.5)**2 + ((Y-8.2)/2.3)**2
        if inner < 1: return 8                             # hollow
        if d > 0.8: return 8                               # dark edge
        # twig weave: diagonal strands, lighter on top rows
        k = (int(X) + int(Y)*2) % 5
        if Y < 9.5: return 6 if k else 7
        return 7 if k else (6 if Y < 12 else 8)
    for n in range(4):                                     # metatiles 16..23: nest with n eggs (left, right)
        tiles += metatile_to_tiles(lambda x, y, n=n: nest_px(x, y, 0, n))
        tiles += metatile_to_tiles(lambda x, y, n=n: nest_px(x, y, 1, n))
    # HUD bar tiles: fill, bottom line
    tiles.append([[9]*8 for _ in range(8)])
    tiles.append([[9]*8 for _ in range(6)] + [[10]*8 for _ in range(2)])
    if name == 'bosque':
        tiles += claro_tiles()                             # metatiles desde el tile 98: árbol 2x2, tronco 2x1, follaje x2
    save_indexed(os.path.join(OUT, f'{name}_maze.png'), strip(tiles), 8, 8*len(tiles), pal0(t))
    # floor (PAL1): 8 shadow variants x 4 textures
    ftiles = []
    for tex in range(4):
        for sh in range(8):           # bit0 wall up, bit1 wall left, bit2 wall up-left
            def fpx(x, y, tex=tex, sh=sh):
                c = floor_pixel(name, x, y, tex)
                up, le, ul = sh & 1, sh & 2, sh & 4
                if (up and y < 5) or (le and x < 4) or (ul and x < 4 and y < 5):
                    c = 4
                    if (up and y < 2) or (le and x < 2): c = 5
                return c
            ftiles += metatile_to_tiles(fpx)
    # metatiles 32..37: adornos del suelo (se pintan en el plano A, sin colisión)
    # 38..39: flores y hierba dobladas por el viento; 40..55: camino de tierra (solo bosque)
    for fn in DECO.DECOS[name] + DECO.DOBLADOS.get(name, [DECO.DECOS[name][0], DECO.DECOS[name][2]]):
        g = fn()
        ftiles += metatile_to_tiles(lambda x, y, g=g: g[y][x])
    if name == 'bosque':
        for mask in range(16):
            ftiles += metatile_to_tiles(lambda x, y, m=mask: DECO.camino(m, x, y) or floor_pixel(name, x, y, 0))
    save_indexed(os.path.join(OUT, f'{name}_floor.png'), strip(ftiles), 8, 8*len(ftiles), pal1(t, name))
    print(name, 'maze tiles', len(tiles), 'floor tiles', len(ftiles))

# ------------------------------------------------------------------ SPRITES & ITEMS (PAL2 = hero palette)
HP = HV.PAL
HKEYS = ['.', 'K','R','r','L','C','c','W','B','b','P','Y','y','G','g','O']   # index order in PAL2
HPAL = [(0,0,0)] + [HP[k] for k in HKEYS[1:]]
def idx_hero(ch):
    if ch == '.': return 0
    return HKEYS.index(ch)

CHICK_MAP = {'K':'K','Y':'Y','y':'y','L':'C','W':'W','B':'B','O':'O','P':'P','E':'W','e':'c'}
CHICK = [''.join(CHICK_MAP.get(c, c) if c != '.' else '.' for c in r) for r in CAST.CHICK]
CLOCK = ["................",".....KKKKKK.....","...KKWWWWWWKK...","..KWWWWKWWWWWK..","..KWWWWKWWWWWK..",".KWWWWWKWWWWWWK.",
         ".KWWWWWKWWWWWWK.",".KWWWWWKKKKWWWK.",".KWWWWWWWWWWWWK.",".KWWWWWWWWWWWWK.","..KWWWWWWWWWWK..","..KWWWWWWWWWWK..",
         "...KKWWWWWWKK...",".....KKKKKK.....","..RR........RR..","................"]
BOLT  = ["................","........KKKK....",".......KYYYK....","......KYYYK.....",".....KYYyK......","....KYYYKKKK....",
         "...KYYYYYYYK....","...KKKKYYyK.....","......KYYK......",".....KYyK.......","....KYYK........","....KYK.........",
         "...KYK..........","...KK...........","................","................"]
GEM   = ["................","................",".....KKKKKK.....","....KbWbbbbK....","...KbWbbbbbBK...","..KbbbbbbbbbBK..",
         "..KKKKKKKKKKKK..","..KBbbbWbbbBBK..","...KBbbWbbBBK...","....KBbbbBBK....",".....KBbBBK.....","......KBBK......",
         ".......KK.......","................","................","................"]
EGG   = ["................","......KKKK......",".....KWWWWK.....","....KWWWWWWK....","...KWWWWWWWWK...","...KWCWWWWWWK...",
         "..KWWCWWWWWWWK..","..KWWWWWWWWWcK..","..KWWWWWWWWWcK..","..KWWWWWWWWccK..","..KcWWWWWWWccK..","...KcWWWWWccK...",
         "....KccccccK....",".....KKKKKK.....","................","................"]
def art_tiles(art):
    return metatile_to_tiles(lambda x, y: idx_hero(art[y][x]))

def gen_items():
    tiles = []
    import anim
    net = anim.projectile_frames()[0][0]
    net_gris = [r.replace('c', 'K').replace('W', 'c') for r in net]
    # 4 = icono de vidas, 5 = huevo, 6 = red lista, 7 = red recargando (marcador del versus)
    for art in (CHICK, CLOCK, BOLT, GEM, HV.SIDE, EGG, net, net_gris):
        tiles += art_tiles(art)
    save_indexed(os.path.join(OUT, 'items.png'), strip(tiles), 8, 8*len(tiles), HPAL)

def bob(art, dy, feet):
    """walk frame: shift body up by dy (feet row stays), replace feet row."""
    body = art[:-1]; out = body[dy:] + ['.'*16]*dy
    return out + [feet]

def sheet(frames_rows, fw, fh, pal_index_fn, pal, path):
    W = fw*len(frames_rows[0]); H = fh*len(frames_rows)
    pix = [[0]*W for _ in range(H)]
    for r, row in enumerate(frames_rows):
        for c, art in enumerate(row):
            for y, line in enumerate(art):
                for x, ch in enumerate(line):
                    pix[r*fh+y][c*fw+x] = pal_index_fn(ch)
    save_indexed(path, pix, W, H, pal)

# Flicki grande (24x24) para los jefes: Scale3x y reducción a la mitad, sin perder rasgos finos
def scale3x(g):
    h=len(g); w=len(g[0]); o=[['.']*(w*3) for _ in range(h*3)]
    P=lambda x,y: g[y][x] if 0<=x<w and 0<=y<h else '.'
    for y in range(h):
        for x in range(w):
            A,B,C,D,E,F,G,H,I=P(x-1,y-1),P(x,y-1),P(x+1,y-1),P(x-1,y),P(x,y),P(x+1,y),P(x-1,y+1),P(x,y+1),P(x+1,y+1)
            if B!=H and D!=F:
                e=[D if D==B else E, E if (D==B and E!=C) or (B==F and E!=A) else E, F if B==F else E,
                   D if (D==B and E!=G) or (D==H and E!=A) else E, E, F if (B==F and E!=I) or (H==F and E!=C) else E,
                   D if D==H else E, E if (D==H and E!=I) or (H==F and E!=G) else E, F if H==F else E]
                # scale3x proper: E1 = (D==B&&E!=C)||(B==F&&E!=A) ? B : E
                e[1]=B if ((D==B and E!=C) or (B==F and E!=A)) else E
                e[3]=D if ((D==B and E!=G) or (D==H and E!=A)) else E
                e[5]=F if ((B==F and E!=I) or (H==F and E!=C)) else E
                e[7]=H if ((D==H and E!=I) or (H==F and E!=G)) else E
            else: e=[E]*9
            for k in range(9): o[y*3+k//3][x*3+k%3]=e[k]
    return o
def down2(g):
    h=len(g)//2; w=len(g[0])//2; o=[['.']*w for _ in range(h)]
    for y in range(h):
        for x in range(w):
            c=[g[2*y][2*x],g[2*y][2*x+1],g[2*y+1][2*x],g[2*y+1][2*x+1]]
            if 'K' in c and c.count('K')>=2: o[y][x]='K'; continue
            nz=[k for k in c if k!='.']
            best=max(set(nz),key=lambda k:(nz.count(k), k in 'KWBYO')) if len(nz)>=2 or (nz and nz[0] in 'YOGgWB') else '.'
            o[y][x]=best
    return o
def hero_big(art):
    g=[list(r) for r in art]
    b=down2(scale3x(g))
    # outline: transparent next to color but not K -> keep; ensure colored pixel next to transparent edge gets K
    h=len(b); w=len(b[0]); o=[r[:] for r in b]
    for y in range(h):
        for x in range(w):
            if b[y][x]=='.': continue
            for dx,dy in ((1,0),(-1,0),(0,1),(0,-1)):
                X,Y=x+dx,y+dy
                if not(0<=X<w and 0<=Y<h) or b[Y][X]=='.':
                    pass
    return [''.join(r) for r in o]

def gen_hero():
    import anim
    sheet(anim.hero_rows(), 16, 16, idx_hero, HPAL, os.path.join(OUT, 'hero.png'))
    sheet([[hero_big(a) for a in row] for row in anim.hero_rows()], 24, 24, idx_hero, HPAL, os.path.join(OUT, 'hero_grande.png'))
    ch = anim.chick_frames(CHICK)
    sheet([ch], 16, 16, idx_hero, HPAL, os.path.join(OUT, 'chick.png'))
    # same frames as BG tiles (4 tiles per frame) to animate the chicks lying on the map
    tiles = []
    for art in ch: tiles += art_tiles(art)
    save_indexed(os.path.join(OUT, 'chick_tiles.png'), strip(tiles), 8, 8*len(tiles), HPAL)
    sheet([anim.SPARK], 16, 16, idx_hero, HPAL, os.path.join(OUT, 'spark.png'))

# hunter: front / back / side drawn here (original design)
HUNTER_FRONT = CAST.HUNTER
HUNTER_BACK = [
"........................",
"........KKKKKKKK........",
"......KKhHHHHHHhKK......",
".....KhHHHHHHHHHHhK.....",
"....KhHHHHHHHHHHHHhK....",
"..KKKKbbbbbbbbbbbbKKKK..",
".KhhhhhhhhhhhhhhhhhhhhK.",
"..KKKmmmmmmmmmmmmmmKKK..",
"....KmmmmmmmmmmmmmmK....",
"....KmmmmmmmmmmmmmmK....",
"....KsmmmmmmmmmmmmsK....",
".....KsSSSSSSSSSSsK.....",
"......KSSSSSSSSSSK......",
"......KKKKKKKKKKKK......",
".....KJJKjjjjjjKJJK.....",
"....KJJJJjjjjjjJJJJK....",
"...KSKJJJJJJJJJJJJKSK...",
"...KSKjJJJJJJJJJJjKSK...",
"....KKKjjjjjjjjjjjKK....",
".......KPPPPKPPPPK......",
"......KPPPPK.KPPPPK.....",
".....KhhhhhK.KhhhhhK....",
".....KKKKKK...KKKKKK....",
"........................",
]
HUNTER_SIDE = [  # facing right
"........................",
"........KKKKKKK.........",
"......KKhHHHHHHKK.......",
".....KhHHlHHHHHHhK......",
"....KhHHHHHHHHHHHhK.....",
"...KKbbbbbbbbbbbbbKKKK..",
"..KhhhhhhhhhhhhhhhhhhhK.",
"...KKKmmmSSSSSSSSKKKK...",
"......KmmSSSSKKSSSK.....",
".....KmmmSSSKWBKSSK.....",
".....KmmSSSSKBBKSSSK....",
".....KsmSSSSSSSSSSSSK...",
"......KsSSSSSSmmmmmK....",
".......KKSSSSmmKKKK.....",
"......KJJKKKKKKK....NNN.",
".....KJJJJJJJJJJK..N...N",
"....KJJJJJJJJJJSSKKN...N",
"....KJJJJJJJJJJKSSKKNNN.",
".....KjjjjbbjjjjKKt.....",
".......KPPPPPPPK.t......",
"......KPPPKKPPPPKt......",
".....KhhhhK.KhhhhhK.....",
".....KKKKK...KKKKKK.....",
"........................",
]
def gen_hunter():
    p = dict(CAST.HUNTER_PAL); p['l'] = p['H']; p['n'] = p['P']; p['t'] = p['h']; p['R'] = p['s']
    keys = ['.'] + sorted(set(k for k in p if p[k] is not None and k not in ('l','n','t','R')))
    pal = [(0,0,0)] + [p[k] for k in keys[1:]]
    remap = {'l':'H','n':'P','t':'h','R':'s'}
    def idx(ch):
        if ch == '.': return 0
        return keys.index(remap.get(ch, ch))
    def region_shift(rows, y0, y1, x0, x1, dx, dy):
        g = [list(r) for r in rows]
        src = [[rows[y][x] for x in range(x0, x1+1)] for y in range(y0, y1+1)]
        for y in range(y0, y1+1):
            for x in range(x0, x1+1): g[y][x] = '.'
        for j, r in enumerate(src):
            for i, c in enumerate(r):
                ny, nx = y0+j+dy, x0+i+dx
                if c != '.' and 0 <= ny < 24 and 0 <= nx < 24: g[ny][nx] = c
        return [''.join(r) for r in g]
    def bob(rows, upto):
        return region_shift(rows, 1, upto, 0, 23, 0, -1)
    def walk_front(base, legs_y):
        f1 = region_shift(bob(base, legs_y-1), legs_y, 22, 0, 11, 0, -1)
        f3 = region_shift(bob(base, legs_y-1), legs_y, 22, 12, 23, 0, -1)
        return [base, f1, base, f3]
    def walk_side(base):
        wide = region_shift(region_shift(base, 19, 22, 0, 11, -1, 0), 19, 22, 12, 23, 1, 0)
        tight = region_shift(region_shift(base, 19, 22, 0, 11, 1, 0), 19, 22, 12, 23, -1, 0)
        return [base, bob(wide, 18), base, bob(tight, 18)]
    rows = [walk_front(HUNTER_FRONT, 20), walk_front(HUNTER_BACK, 20), walk_side(HUNTER_SIDE)]
    sheet(rows, 24, 24, idx, pal, os.path.join(OUT, 'hunter.png'))


def gen_enemies():
    import anim
    specs = [('flame', 'fire', CAST.FIRE, CAST.FIRE_PAL, 'R', 10, 16),
             ('shark', 'shark', CAST.SHARK, CAST.SHARK_PAL, 'M', 8, 17),
             ('imp', 'imp', CAST.ONI, CAST.ONI_PAL, 'V', 8, 16)]
    for name, kind, art, pal_d, body, y0, y1 in specs:
        keys = ['.'] + sorted(pal_d.keys())
        assert len(keys) <= 16, name
        pal = [(0,0,0)] + [pal_d[k] for k in keys[1:]]
        rows = anim.enemy_rows(kind, art, body, y0, y1)
        sheet(rows, 24, 24, lambda ch, keys=keys: 0 if ch == '.' else keys.index(ch), pal, os.path.join(OUT, f'{name}.png'))

# ------------------------------------------------------------------ FONT (96 chars, PAL index 15 white / 14 shadow)
EXTRA_GLYPHS = {
 '>': ["#....",".#...","..#..","...#.","..#..",".#...","#...."],
 '^': ["..#..",".###.","#####",".....",".....",".....","....."],
 '<': ["....#","...#.","..#..",".#...","..#..","...#.","....#"],
 '|': ["..#..","..#..","..#..","..#..","..#..","..#..","..#.."],
 '=': [".....",".....","#####",".....","#####",".....","....."],
 '+': [".....","..#..","..#..","#####","..#..","..#..","....."],
 '%': ["##..#","##.#.","..#..",".#...","#.##.","#..##","....."],
 '#': [".#.#.","#####",".#.#.",".#.#.","#####",".#.#.","....."],
 '[': [".###.",".#...",".#...",".#...",".#...",".#...",".###."],
 ']': [".###.","...#.","...#.","...#.","...#.","...#.",".###."],
 '_': [".....",".....",".....",".....",".....",".....","#####"],
 '$': ["..#..",".####","#.#..",".###.","..#.#","####.","..#.."],
 '@': [".###.","#...#","#.###","#.#.#","#.###","#....",".###."],
}
def gen_font(opaque=False):
    tiles = []
    for code in range(32, 128):
        ch = chr(code)
        g = EXTRA_GLYPHS.get(ch) or F.G.get(ch) or F.G.get(ch.upper()) or F.G.get(' ')
        t = [[13 if opaque else 0]*8 for _ in range(8)]
        for y in range(7):
            for x in range(5):
                if g[y][x] == '#':
                    if y+1 < 8 and x+1 < 8 and t[y+1][x+1] == 0: t[y+1][x+1] = 14
        for y in range(7):
            for x in range(5):
                if g[y][x] == '#': t[y][x+1] = 15
        tiles.append(t)
    # 16 chars per row, ROW ordering -> tile index = code-32
    W, H = 128, 48
    pix = [[0]*W for _ in range(H)]
    for i, t in enumerate(tiles):
        cx, cy = (i % 16)*8, (i // 16)*8
        for y in range(8):
            for x in range(8): pix[cy+y][cx+x] = t[y][x]
    pal = [(0,0,0)] + [(0,0,0)]*13 + [(0,0,0), (255,255,255)]
    save_indexed(os.path.join(OUT, 'font_menu.png' if opaque else 'font.png'), pix, W, H, pal)

# ------------------------------------------------------------------ HUD FONT (8x16 cells, bar background baked in, PAL0)
HUD_CHARS = " 0123456789:/-xABCDEFGHIJKLMNOPQRSTUVWXYZ!#_+"
def gen_hud():
    tiles_top, tiles_bot = [], []
    for ch in HUD_CHARS:
        g = EXTRA_GLYPHS.get(ch) or F.G.get(ch) or F.G.get(ch.upper()) or F.G[' ']
        col = 11 if ch.isalpha() and ch != 'x' else 15
        cell = [[9]*8 for _ in range(14)] + [[10]*8 for _ in range(2)]
        if ch in '#_':      # boss life bar segment: full (cream with white top) / empty (outline)
            for y in range(4, 11):
                for x in range(1, 7):
                    if ch == '#': cell[y][x] = 15 if y == 4 else 11
                    elif y in (4, 10) or x in (1, 6): cell[y][x] = 10
            for x in range(1, 7): cell[11][x] = 14
            tiles_top.append(cell[:8]); tiles_bot.append(cell[8:]); continue
        for y in range(7):
            for x in range(5):
                if g[y][x] == '#': cell[5+y][x+2] = 14
        for y in range(7):
            for x in range(5):
                if g[y][x] == '#': cell[4+y][x+1] = col
        tiles_top.append(cell[:8]); tiles_bot.append(cell[8:])
    # order: for char i -> tile 2*i (top), 2*i+1 (bottom)
    tiles = []
    for a, b in zip(tiles_top, tiles_bot): tiles += [a, b]
    save_indexed(os.path.join(OUT, 'hud.png'), strip(tiles), 8, 8*len(tiles), pal0(WORLDS['bosque']))
    with open(os.path.join(ROOT, 'src', 'hud_chars.h'), 'w') as f:
        f.write('// generado por tools/art/gen_assets.py\n#define HUD_CHARS "%s"\n' % HUD_CHARS)

def gen_fx():
    import anim
    font = dict(F.G); font.update(EXTRA_GLYPHS)
    sheet(anim.egg_frames(), 16, 16, idx_hero, HPAL, os.path.join(OUT, 'huevo.png'))
    sheet(anim.star_frames(), 24, 8, idx_hero, HPAL, os.path.join(OUT, 'estrellas.png'))
    sheet(anim.popup_frames(font), 32, 8, idx_hero, HPAL, os.path.join(OUT, 'popup.png'))
    sheet(anim.dust_frames(), 16, 16, idx_hero, HPAL, os.path.join(OUT, 'polvo.png'))
    sheet(anim.projectile_frames(), 16, 16, idx_hero, HPAL, os.path.join(OUT, 'proyectil.png'))
    sheet(anim.ambient_frames(), 16, 16, idx_hero, HPAL, os.path.join(OUT, 'ambiente.png'))
    sheet(anim.reload_frames(), 16, 8, idx_hero, HPAL, os.path.join(OUT, 'recarga.png'))

def gen_bosses():
    # end-of-world bosses: the world enemy at double size (48x48)
    for src, dst in (('hunter', 'jefe_cazador'), ('flame', 'jefe_llama'), ('shark', 'jefe_tiburon'), ('imp', 'jefe_diablillo')):
        im = Image.open(os.path.join(OUT, src + '.png'))
        big = im.resize((im.width * 2, im.height * 2), Image.NEAREST)
        big.save(os.path.join(OUT, dst + '.png'))

if __name__ == '__main__':
    for w in WORLDS: gen_world(w)
    gen_items(); gen_hero(); gen_hunter(); gen_enemies(); gen_font(); gen_font(True); gen_hud(); gen_fx(); gen_bosses()
    import claro_arena
    print('arena del bosque, baldosas:', claro_arena.generar(os.path.join(OUT, 'claro_arena.png')))
    print('assets written to', OUT)
