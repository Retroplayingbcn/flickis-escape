"""Phase 2 animation frames (original designs). All frames are lists of strings.
Hero 16x16: rows = walk down, walk up, walk side, hurt, happy (4 frames each).
"""
import hero_views as HV

def G(art): return [list(r) for r in art]
def S(g): return [''.join(r) for r in g]
def blank(w=16, h=16): return [['.']*w for _ in range(h)]

def shift_v(g, dy, keep_bottom=1):
    """shift everything except the last keep_bottom rows by dy (neg=up)."""
    h = len(g); w = len(g[0]); body = g[:h-keep_bottom]; feet = g[h-keep_bottom:]
    out = blank(w, h-keep_bottom)
    for y in range(len(body)):
        ny = y + dy
        if 0 <= ny < len(body): out[ny] = list(body[y])
    return out + [list(r) for r in feet]

def paste(g, art, ox, oy, transparent='.'):
    for y, r in enumerate(art):
        for x, c in enumerate(r):
            if c != transparent and 0 <= oy+y < len(g) and 0 <= ox+x < len(g[0]): g[oy+y][ox+x] = c
    return g

def erase(g, x0, y0, x1, y1, keep_fn=None):
    for y in range(y0, y1+1):
        for x in range(x0, x1+1): g[y][x] = '.'
    return g

FEET_A = '.....OO..OO.....'
FEET_L = '....OO....O.....'   # left foot forward
FEET_R = '.....O....OO....'
SFEET_0 = '......OO.OO.....'
SFEET_1 = '.....OO...OO....'
SFEET_2 = '.......OOO......'

# small wings drawn outside the round body (front/back views)
WING_UP_L = ["K.", "rK", "rK"]
WING_DN_L = ["rK", "rK", "K."]
def mirror(art): return [r[::-1] for r in art]

def front_like(base, frame):
    g = G(base)
    if frame in (1, 3):
        g = shift_v(g, -1)
        g[-1] = list(FEET_L if frame == 1 else FEET_R)
        # wings raised: little feathers above the shoulder line
        paste(g, ["K", "r"], 0, 4); paste(g, ["K", "r"], 15, 4)
    else:
        g[-1] = list(FEET_A)
        paste(g, ["r", "K"], 0, 10); paste(g, ["r", "K"], 15, 10)
    return S(g)

def side(frame):
    g = G(HV.SIDE)
    # wing = the darker 'r' patch on the back (cols 1-5, rows 7-11). Move it for a flap.
    wing = [[g[y][x] if g[y][x] in 'r' else '.' for x in range(0, 6)] for y in range(7, 12)]
    if frame in (1, 3):
        for y in range(7, 12):
            for x in range(0, 6):
                if g[y][x] == 'r': g[y][x] = 'R'
        paste(g, [''.join(r) for r in wing], 0, 4)            # wing lifted 3px
        paste(g, ["..K", ".Kr", "Krr"], 0, 3)
        g = shift_v(g, -1)
        g[-1] = list(SFEET_1 if frame == 1 else SFEET_2)
    else:
        g[-1] = list(SFEET_0)
    return S(g)

def hurt(frame):
    g = G(HV.FRONT)
    # X eyes over rows 6-8, cols 4-6 and 9-11
    for ex in (4, 9):
        for y in range(6, 9):
            for x in range(ex, ex+3): g[y][x] = 'C'
        for d in range(3):
            g[6+d][ex+d] = 'K'; g[6+d][ex+2-d] = 'K'
    g[9][6:10] = list('KYYK'); g[10][7:9] = list('yy')   # open beak
    # orbiting stars around the head
    pos = [(1, 1), (12, 0), (14, 4), (0, 5)]
    for i in range(2):
        x, y = pos[(frame + i*2) % 4]
        paste(g, [".Y.", "YWY", ".Y."], x, y)
    if frame & 1: g = shift_v(g, 0)
    g[-1] = list(FEET_A)
    return S(g)

def happy(frame):
    """celebration: ^^ eyes, wings flapping (the hop is done in code with the sprite y)."""
    g = G(HV.FRONT)
    for ex in (4, 9):     # ^ ^ eyes
        for y in range(6, 9):
            for x in range(ex, ex+3): g[y][x] = 'C'
        g[6][ex+1] = 'K'; g[7][ex] = 'K'; g[7][ex+2] = 'K'
    g[9][6:10] = list('KYYK'); g[10][6:10] = list('CyyC')
    if frame & 1:
        paste(g, ["K", "r", "r"], 0, 3); paste(g, ["K", "r", "r"], 15, 3)
    else:
        paste(g, ["r", "r", "K"], 0, 9); paste(g, ["r", "r", "K"], 15, 9)
    g[-1] = list(FEET_A)
    return S(g)

def hero_rows():
    return [[front_like(HV.FRONT, f) for f in range(4)],
            [front_like(HV.BACK, f) for f in range(4)],
            [side(f) for f in range(4)],
            [hurt(f) for f in range(4)],
            [happy(f) for f in range(4)]]

# ---------------- chick: 4-frame idle/hop used both as follower sprite and map tile
def chick_frames(CHICK):
    f0 = CHICK
    f1 = S(shift_v(G(CHICK), -1))
    blink = G(CHICK)
    for y in range(len(blink)):
        for x in range(16):
            if blink[y][x] in 'BW' and 4 <= y <= 9: blink[y][x] = 'Y'
    return [f0, f1, f0, S(blink)]

# ---------------- sparkle effect (16x16, 4 frames)
SPARK = [
 ["................","................","................","................","................","................",
  ".......W........","......WYW.......",".......W........","................","................","................",
  "................","................","................","................"],
 ["................","................","................","................",".......Y........",".......Y........",
  "......YWY.......","....YYWWWYY.....","......YWY.......",".......Y........",".......Y........","................",
  "................","................","................","................"],
 ["................","................",".......Y........","..Y....Y....Y...","...Y...W...Y....","....Y.....Y.....",
  "................","..YW...W...WY...","................","....Y.....Y.....","...Y...W...Y....","..Y....Y....Y...",
  ".......Y........","................","................","................"],
 ["................",".......y........","................",".y.....W.....y..","................","................",
  "................","y..W.......W..y.","................","................","................",".y.....W.....y..",
  "................",".......y........","................","................"],
]
