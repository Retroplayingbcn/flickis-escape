#!/usr/bin/env python3
"""Generate Flicki's Escape 16-bit redesign assets (indexed PNGs for SGDK rescomp).
Run from repo root:  python3 tools/art/gen_assets.py
"""
import os, sys, math
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from PIL import Image
import hero_views as HV, cast as CAST, font5x7 as F

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
 'bosque': dict(
   wall=[(60,140,50),(36,96,40),(90,170,60),(160,230,110),(24,70,30)],     # mid,dark,light,hi,edge
   nest=[(232,190,120),(176,112,48),(120,72,32)],
   hud=[(90,56,30),(240,200,120),(250,226,170)],
   floor=[(136,186,80),(150,200,90),(120,170,70)], shadow=(96,140,60), shadow2=(80,120,52),
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
def pal1(t):
    return [(0,0,0)] + t['floor'] + [t['shadow'], t['shadow2']]

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
    v = hsh(x//2 + tex*8, y//2 + tex*5, 11) % 100
    return 1 if v > 14 else (2 if v < 8 else 3)

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

def gen_world(name):
    t = WORLDS[name]
    tiles = []
    for mask in range(16):
        tiles += metatile_to_tiles(lambda x, y, m=mask: wall_pixel(m, x, y, m, name))
    # nest: two metatiles (left/right) woven straw with eggs
    def nest_px(x, y, side):
        X = x + side*16 + 0.5; Y = y + 0.5
        # eggs (3) sitting in the bowl, peeking above the rim
        for ex, ey in ((10.5, 6.5), (16, 5.5), (21.5, 6.5)):
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
    tiles += metatile_to_tiles(lambda x, y: nest_px(x, y, 0))
    tiles += metatile_to_tiles(lambda x, y: nest_px(x, y, 1))
    # HUD bar tiles: fill, bottom line
    tiles.append([[9]*8 for _ in range(8)])
    tiles.append([[9]*8 for _ in range(6)] + [[10]*8 for _ in range(2)])
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
    save_indexed(os.path.join(OUT, f'{name}_floor.png'), strip(ftiles), 8, 8*len(ftiles), pal1(t))
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
def art_tiles(art):
    return metatile_to_tiles(lambda x, y: idx_hero(art[y][x]))

def gen_items():
    tiles = []
    for art in (CHICK, CLOCK, BOLT, GEM, HV.SIDE):     # last = HUD life icon
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

def gen_hero():
    import anim
    sheet(anim.hero_rows(), 16, 16, idx_hero, HPAL, os.path.join(OUT, 'hero.png'))
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
HUD_CHARS = " 0123456789:/-xABCDEFGHIJKLMNOPQRSTUVWXYZ!"
def gen_hud():
    tiles_top, tiles_bot = [], []
    for ch in HUD_CHARS:
        g = F.G.get(ch) or F.G.get(ch.upper()) or F.G[' ']
        col = 11 if ch.isalpha() and ch != 'x' else 15
        cell = [[9]*8 for _ in range(14)] + [[10]*8 for _ in range(2)]
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

if __name__ == '__main__':
    for w in WORLDS: gen_world(w)
    gen_items(); gen_hero(); gen_hunter(); gen_enemies(); gen_font(); gen_font(True); gen_hud()
    print('assets written to', OUT)
