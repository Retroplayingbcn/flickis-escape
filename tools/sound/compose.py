#!/usr/bin/env python3
"""Original music for Flicki's Escape, written as data and exported to ctrmml MML.
Each song = tempo + chord progression (one entry per bar, "Am D" splits a bar) + melody bars.
The script writes tools/sound/mml/<song>.mml; build_music.sh compiles them to VGM for XGM2.
Channels: A lead, B counter-melody/echo, C+D pad, E bass, F drums (PCM), G PSG arpeggio, J PSG hi-hat.
"""
import os, re

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, 'mml')
os.makedirs(OUT, exist_ok=True)

INSTRUMENTS = r'''
; ---------------- original FM patches
@1 fm ; picked bass
	  3   0
	 31   0  19   5   0  23   0   0   0   0
	 31   6   0   4   3  19   0   0   0   0
	 31  15   0   5   4  38   0   4   0   0
	 31  27   0  11   1   0   0   1   0   0
@2 fm ; bright lead
	  4   5
	 26   6   0   6   1  26   0   2   3   0
	 24   5   2   7   2   0   0   1   0   0
	 26   6   0   6   1  30   0   1   7   0
	 24   5   2   7   2   6   0   1   0   0
@3 fm ; bell / e-piano
	  4   5
	 31   8   0   6   2  32   0   7   0   0
	 31  12   3   7   4   0   1   1   0   0
	 31  10   0   6   2  38   0   1   3   0
	 31  12   3   7   4   2   1   1   0   0
@4 fm ; soft pad
	  4   3
	 16   4   0   6   2  38   0   1   3   0
	 14   2   0   6   1   0   0   1   0   0
	 16   4   0   6   2  40   0   2   7   0
	 14   2   0   6   1   4   0   1   0   0
@5 fm ; flute
	  4   1
	 20   0   0   7   0  46   0   1   0   0
	 18   3   0   7   1   0   0   1   0   0
	 20   0   0   7   0 127   0   1   0   0
	 18   3   0   7   1 127   0   1   0   0
@6 fm ; brass
	  4   5
	 20   5   0   6   1  24   0   1   0   0
	 18   4   2   7   2   0   0   1   0   0
	 20   5   0   6   1  28   0   1   3   0
	 18   4   2   7   2   2   0   1   0   0
@10 psg 11>6:10 / 5>0:20
@11 psg 12>0:4
@20 pcm "../pcm/kick.wav"
@21 pcm "../pcm/snare.wav"
@M1 0:24 | V0:0.18:6
'''

NOTE = {'c': 0, 'd': 2, 'e': 4, 'f': 5, 'g': 7, 'a': 9, 'b': 11}

def pc(name):
    """'f#' / 'bb' / 'c' -> pitch class"""
    v = NOTE[name[0]]
    for ch in name[1:]:
        v += 1 if ch == '#' else -1 if ch == 'b' else 0
    return v % 12

def chord_tones(ch):
    m = re.match(r'([A-G])([#b]?)(m?)(7?)$', ch)
    root = pc(m.group(1).lower() + m.group(2))
    third = 3 if m.group(3) else 4
    tones = [root, root + third, root + 7]
    if m.group(4): tones.append(root + 10)
    return root, tones

MML_NOTE = ['c', 'c+', 'd', 'd+', 'e', 'f', 'f+', 'g', 'g+', 'a', 'a+', 'b']

def mnote(midi, ticks):
    o, n = divmod(midi, 12)
    return f'o{o}{MML_NOTE[n]}:{ticks}'   # ctrmml: o4c = C3 (MIDI 48)

def parse_melody(bar, tpb):
    """'e5:8 g5:4. r:4' -> list of (midi or None, ticks)"""
    out = []
    for tok in bar.split():
        name, dur = tok.split(':')
        dot = dur.endswith('.'); dur = dur.rstrip('.')
        ticks = tpb // int(dur)
        if dot: ticks += ticks // 2
        if name == 'r': out.append((None, ticks)); continue
        m = re.match(r'([a-g][#b]?)(\d)$', name)
        out.append(((int(m.group(2)) + 1) * 12 + pc(m.group(1)), ticks))
    total = sum(t for _, t in out)
    assert total == tpb, (bar, total)
    return out

def seq_mml(events):
    return ' '.join(mnote(n, t) if n is not None else f'r:{t}' for n, t in events)

def bars_to_chords(prog, tpb):
    res = []
    for bar in prog:
        parts = bar.split()
        for p in parts: res.append((p, tpb // len(parts)))
    return res

def bass_line(chords, style):
    ev = []
    for ch, dur in chords:
        root, tones = chord_tones(ch)
        r = 36 + root if root < 7 else 24 + root          # around E1..D2
        if style == 'oct8':
            for i in range(dur // 12): ev.append((r + (12 if i % 2 else 0), 12))
        elif style == 'root4':
            fifth = r + 7
            for i in range(dur // 24): ev.append(((r, fifth)[i % 2], 24))
        elif style == 'drive':
            for i in range(dur // 12): ev.append((r if i % 4 != 3 else r + 12, 12))
        elif style == 'sway':
            for i in range(dur // 48): ev.append(((r, r + 7)[i % 2], 48))
        elif style == 'calypso':                       # saltarín: raíz con anticipo, quinta y octava
            if dur >= 96:
                for _ in range(dur // 96): ev += [(r, 18), (r, 6), (r + 7, 24), (r + 12, 12), (r + 7, 12), (r, 24)]
            else:
                ev += [(r, 18), (r, 6), (r + 7, 12), (r + 12, 12)]
        elif style == 'hold':
            ev.append((r, dur))
    return ev

def pad_voice(chords, which):
    ev = []
    for ch, dur in chords:
        root, tones = chord_tones(ch)
        t = tones[1 + which] if which < 2 else tones[0]
        n = 60 + (t % 12)
        if n > 67: n -= 12
        ev.append((n, dur))
    return ev

def arp_line(chords, step=6, base=72):
    ev = []
    for ch, dur in chords:
        root, tones = chord_tones(ch)
        ns = [base + (t % 12) for t in tones] + [base + 12 + (tones[0] % 12)]
        pat = [0, 1, 2, 3, 2, 1] if len(ns) >= 4 else [0, 1, 2, 1]
        for i in range(dur // step): ev.append((ns[pat[i % len(pat)]], step))
    return ev

DRUMS = {   # per bar of 16 sixteenths: k kick, s snare, . nothing ; hats separately
    'rock':  ('k...s...k.k.s...', 'x.x.x.x.x.x.x.x.'),
    'drive': ('k.k.s.k.k.k.s.k.', 'xxxxxxxxxxxxxxxx'),
    'half':  ('k.......s.......', 'x...x...x...x...'),
    'light': ('k.......k.......', '..x...x...x...x.'),
    'calypso': ('k..s..k.k..s.kk.', 'x.xxx.xxx.xxx.xx'),
    'none':  ('................', '................'),
}

def drum_lines(nbars, style, tpb):
    kpat, hpat = DRUMS[style]
    st = tpb // 16
    f, j = [], []
    for _ in range(nbars):
        for c in kpat:
            if c == 'k': f.append('@20 c:%d' % st)
            elif c == 's': f.append('@21 c:%d' % st)
            else: f.append('r:%d' % st)
        for c in hpat:
            j.append('o4c:%d' % st if c == 'x' else 'r:%d' % st)
    return ' '.join(f), ' '.join(j)

def song(name, title, tempo, prog, melody, bass='oct8', drums='rock', loop=True, lead=2, pad=4,
         counter=None, arp=True, echo=True, vol_lead=13):
    tpb = 96
    assert len(prog) == len(melody), (name, len(prog), len(melody))
    chords = bars_to_chords(prog, tpb)
    mel = []
    for b in melody: mel += parse_melody(b, tpb)
    L = 'L ' if loop else ''
    lines = [f'#title {title}', '#composer RetroPlayingBCN', '#platform megadrive', INSTRUMENTS,
             f'A t{tempo}']
    lines.append(f'A {L}@{lead} v{vol_lead} M1 p3 ' + seq_mml(mel))
    if echo:   # delayed, quieter copy of the lead = cheap stereo echo
        delay = 12
        ev = [(None, delay)] + mel
        # trim the end so the track keeps the same length
        total = 0; trimmed = []
        for n, t in ev:
            if total + t > tpb * len(prog): t = tpb * len(prog) - total
            if t <= 0: break
            trimmed.append((n, t)); total += t
        lines.append(f'B {L}@{lead} v8 p1 ' + seq_mml(trimmed))
    elif counter:
        cm = []
        for b in counter: cm += parse_melody(b, tpb)
        lines.append(f'B {L}@3 v10 p2 ' + seq_mml(cm))
    lines.append(f'C {L}@{pad} v9 p2 ' + seq_mml(pad_voice(chords, 0)))
    lines.append(f'D {L}@{pad} v9 p1 ' + seq_mml(pad_voice(chords, 1)))
    lines.append(f'E {L}@1 v14 p3 ' + seq_mml(bass_line(chords, bass)))
    f, j = drum_lines(len(prog), drums, tpb)
    if drums != 'none':
        lines.append(f'F {L}v15 ' + f)
        lines.append(f'J {L}@11 v11 ' + j)
    if arp:
        lines.append(f'G {L}@10 v9 ' + seq_mml(arp_line(chords)))
    path = os.path.join(OUT, name + '.mml')
    open(path, 'w').write('\n'.join(lines) + '\n')
    return path

SONGS = []
def add(*a, **k): SONGS.append(song(*a, **k))

# ---------------------------------------------------------------- TITLE (C major, bouncy)
add('titulo', "Flicki's Escape - Title", 140,
    ['C', 'Am', 'F', 'G', 'C', 'Am', 'F G', 'C', 'F', 'G', 'Em', 'Am', 'F', 'G', 'C', 'G'],
    ['e4:8 g4:8 c5:4 b4:8 c5:8 d5:4', 'c5:8 a4:8 e4:4 a4:4 r:8 a4:8', 'f4:8 a4:8 c5:4 a4:8 c5:8 f5:4', 'e5:8 d5:8 c5:8 b4:8 g4:4 r:4',
     'e4:8 g4:8 c5:4 b4:8 c5:8 e5:4', 'd5:8 c5:8 a4:8 c5:8 e5:4 r:8 e5:8', 'f5:8 e5:8 d5:8 c5:8 b4:8 c5:8 d5:8 b4:8', 'c5:2 r:4 g4:4',
     'c5:4 a4:8 c5:8 f5:4 e5:8 d5:8', 'd5:4 b4:8 d5:8 g5:4 f5:8 e5:8', 'e5:8 d5:8 e5:8 g5:8 b5:4 g5:8 e5:8', 'a5:4. g5:8 e5:4 c5:4',
     'f5:8 e5:8 f5:8 a5:8 c6:4 a5:4', 'g5:8 f5:8 e5:8 d5:8 b4:4 g4:4', 'c5:8 e5:8 g5:8 c6:8 b5:8 g5:8 e5:8 g5:8', 'd5:2 r:4 g4:4'])

# ---------------------------------------------------------------- MENU (F major, relaxed)
add('menu', "Flicki's Escape - Menu", 108,
    ['F', 'Dm', 'Bb', 'C', 'F', 'Dm', 'Gm', 'C'],
    ['a4:4 c5:8 a4:8 f4:4 g4:4', 'a4:4. g4:8 f4:4 d4:4', 'f4:8 g4:8 a4:8 bb4:8 d5:4 c5:4', 'c5:4 bb4:8 a4:8 g4:2',
     'a4:4 c5:8 a4:8 f5:4 e5:8 d5:8', 'c5:4. a4:8 d5:4 f4:4', 'g4:8 a4:8 bb4:8 d5:8 c5:4 bb4:4', 'a4:2 g4:4 r:4'],
    bass='root4', drums='light', lead=5, vol_lead=14)

# ---------------------------------------------------------------- FOREST (G major, chase)
add('bosque', "Flicki's Escape - Forest", 150,
    ['G', 'Em', 'C', 'D', 'G', 'Em', 'Am D', 'G', 'C', 'D', 'Bm', 'Em', 'C', 'D', 'G', 'D'],
    ['d5:8 b4:8 g4:8 b4:8 d5:4 g5:4', 'e5:8 d5:8 b4:8 g4:8 e4:4 r:8 e4:8', 'c5:8 e5:8 g5:8 e5:8 c5:4 e5:4', 'd5:8 c5:8 b4:8 a4:8 f#4:4 d4:4',
     'd5:8 b4:8 g4:8 b4:8 d5:4 g5:4', 'g5:8 f#5:8 e5:8 d5:8 b4:4 g4:4', 'a4:8 c5:8 e5:8 c5:8 d5:8 f#5:8 a5:8 f#5:8', 'g5:2 r:4 d5:4',
     'e5:4 g5:8 e5:8 c5:4 e5:4', 'f#5:4 a5:8 f#5:8 d5:4 f#5:4', 'b4:8 d5:8 f#5:8 d5:8 b4:4 f#4:4', 'e5:4. d5:8 b4:4 g4:4',
     'c5:8 d5:8 e5:8 g5:8 c6:4 g5:4', 'a5:8 g5:8 f#5:8 e5:8 d5:4 a4:4', 'b4:8 d5:8 g5:8 b5:8 a5:8 g5:8 f#5:8 g5:8', 'a5:2 r:4 d5:4'])

# ---------------------------------------------------------------- VOLCANO (D minor, driving)
add('volcan', "Flicki's Escape - Volcano", 160,
    ['Dm', 'Dm', 'Bb', 'C', 'Dm', 'Dm', 'Bb A', 'Dm', 'Gm', 'Dm', 'Bb', 'A7'],
    ['d5:8 d5:8 a4:8 d5:8 f5:8 e5:8 d5:4', 'c5:8 d5:8 e5:8 f5:8 a5:4 g5:8 f5:8', 'f5:8 d5:8 bb4:8 d5:8 f5:4 bb5:4', 'g5:8 f5:8 e5:8 c5:8 g4:4 c5:4',
     'd5:8 d5:8 a4:8 d5:8 f5:8 e5:8 d5:4', 'a5:8 g5:8 f5:8 e5:8 d5:4 a4:4', 'bb4:8 d5:8 f5:8 d5:8 c#5:8 e5:8 a5:8 e5:8', 'd5:2. r:4',
     'g4:8 bb4:8 d5:8 g5:8 f5:4 d5:4', 'f5:8 e5:8 d5:8 a4:8 f4:4 a4:4', 'bb4:4 d5:4 f5:4 bb5:4', 'a5:4 e5:4 c#5:4 a4:4'],
    bass='drive', drums='drive', lead=6)

# ---------------------------------------------------------------- SEA (Eb major, calypso alegre)
add('marino', "Flicki's Escape - Sea", 144,
    ['Eb', 'Cm', 'Ab', 'Bb', 'Eb', 'Gm', 'Ab Bb', 'Eb', 'Ab', 'Bb', 'Gm', 'Cm', 'Ab', 'Bb', 'Ab Bb', 'Eb'],
    ['bb4:8 eb5:8 r:8 g5:8 r:8 f5:8 eb5:8 bb4:8', 'c5:8 eb5:8 r:8 g5:4 f5:8 eb5:8 c5:8',
     'ab4:8 c5:8 eb5:8 ab5:8 r:8 g5:8 f5:8 eb5:8', 'f5:4 d5:8 bb4:8 r:8 bb4:8 c5:8 d5:8',
     'eb5:8 g5:8 bb5:8 g5:8 r:8 bb5:8 ab5:8 g5:8', 'g5:8 f5:8 d5:8 bb4:8 r:8 d5:8 f5:8 g5:8',
     'ab5:8 g5:8 f5:8 eb5:8 d5:8 eb5:8 f5:8 d5:8', 'eb5:4 r:8 bb4:8 eb5:8 g5:8 bb5:4',
     'c6:8 bb5:8 ab5:8 r:8 eb5:8 ab5:8 r:8 c6:8', 'bb5:8 ab5:8 f5:8 r:8 d5:8 f5:8 r:8 bb5:8',
     'g5:8 f5:8 g5:8 bb5:8 r:8 g5:8 d5:8 bb4:8', 'c5:8 d5:8 eb5:8 g5:8 c6:4 r:8 bb5:8',
     'ab5:8 g5:8 ab5:8 c6:8 r:8 ab5:8 eb5:8 c5:8', 'd5:8 f5:8 bb5:8 d6:8 r:8 c6:8 bb5:8 f5:8',
     'eb5:8 g5:8 c6:8 ab5:8 f5:8 ab5:8 d6:8 bb5:8', 'eb6:4 bb5:8 g5:8 eb5:4 r:4'],
    bass='calypso', drums='calypso', lead=3, vol_lead=14)

# ---------------------------------------------------------------- SKY (A major, airy)
add('celeste', "Flicki's Escape - Sky", 132,
    ['A', 'F#m', 'D', 'E', 'A', 'C#m', 'D E', 'A'],
    ['e5:4 a5:4 c#6:8 b5:8 a5:4', 'f#5:4. e5:8 c#5:4 a4:4', 'd5:8 f#5:8 a5:8 f#5:8 d6:4 a5:4', 'g#5:4 b5:4 e5:2',
     'e5:4 a5:4 c#6:8 d6:8 e6:4', 'e6:8 d6:8 c#6:8 b5:8 g#5:4 e5:4', 'f#5:8 a5:8 d6:8 a5:8 g#5:8 b5:8 e6:8 b5:8', 'a5:2. r:4'],
    bass='root4', drums='light', lead=3)

# ---------------------------------------------------------------- JINGLES / ONE-SHOTS
add('intro', "Flicki's Escape - World start", 140,
    ['C', 'G', 'C'],
    ['c5:8 e5:8 g5:8 c6:8 b5:8 g5:8 d5:4', 'b4:8 d5:8 g5:8 b5:8 a5:8 f5:8 d5:4', 'c6:2. r:4'],
    loop=False, lead=6, drums='rock', arp=False)

add('superada', "Flicki's Escape - Stage clear", 150,
    ['C', 'C'],
    ['g4:16 c5:16 e5:16 g5:16 c6:4 r:8 a5:8 b5:4', 'c6:2 r:2'],
    loop=False, lead=6, drums='none', arp=False, bass='hold')

add('gameover', "Flicki's Escape - Game over", 84,
    ['Am', 'Dm', 'E', 'Am'],
    ['e5:4 d5:4 c5:4 b4:4', 'a4:4 f4:4 d4:2', 'e4:4 g#4:4 b4:4 d5:4', 'c5:2 a4:2'],
    loop=False, lead=5, drums='none', arp=False, bass='hold', echo=True)

add('victoria', "Flicki's Escape - Ending", 136,
    ['C', 'F', 'G', 'C', 'Am', 'F', 'G', 'C'],
    ['g4:8 g4:8 g4:8 c5:8 e5:4 g5:4', 'f5:8 e5:8 f5:8 a5:8 c6:2', 'b5:8 a5:8 g5:8 f5:8 d5:4 g5:4', 'e5:4 c5:4 g4:2',
     'a4:8 c5:8 e5:8 a5:8 g5:4 e5:4', 'f5:8 g5:8 a5:8 c6:8 a5:4 f5:4', 'g5:4 b5:4 d6:4 b5:4', 'c6:2. r:4'],
    lead=6, drums='rock')

add('continuar', "Flicki's Escape - Continue", 96,
    ['Am', 'F', 'C', 'G'],
    ['e5:2 c5:2', 'a4:2 c5:2', 'g4:2 e5:2', 'd5:1'],
    lead=5, drums='light', bass='root4', arp=True)


# ---------------------------------------------------------------- BOSS (E minor, arcade showdown)
add('jefe', "Flicki's Escape - Boss", 172,
    ['Em', 'Em', 'C', 'D', 'Em', 'Em', 'C', 'B7', 'Am', 'Em', 'C', 'B7', 'Am', 'Em', 'C D', 'B7'],
    ['e5:8 e5:8 b4:8 e5:8 g5:8 f#5:8 e5:8 b4:8', 'e5:8 g5:8 b5:8 g5:8 a5:8 g5:8 f#5:8 e5:8', 'c5:8 e5:8 g5:8 c6:8 b5:8 g5:8 e5:4', 'd5:8 f#5:8 a5:8 d6:8 c6:8 a5:8 f#5:4',
     'e5:8 e5:8 b4:8 e5:8 g5:8 f#5:8 e5:8 b4:8', 'g5:8 a5:8 b5:8 e6:8 d6:8 b5:8 g5:4', 'c6:8 b5:8 a5:8 g5:8 e5:8 g5:8 c6:4', 'b5:4 a5:8 f#5:8 d#5:4 b4:4',
     'a4:8 c5:8 e5:8 a5:8 g5:4 e5:4', 'b4:8 e5:8 g5:8 b5:8 a5:4 g5:4', 'c5:8 e5:8 g5:8 e5:8 c6:4 b5:4', 'b5:8 a5:8 f#5:8 d#5:8 b4:2',
     'a5:4 c6:4 e6:4 c6:4', 'g5:4 b5:4 e6:4 b5:4', 'c6:8 b5:8 a5:8 g5:8 d6:8 c6:8 b5:8 a5:8', 'b5:2. r:4'],
    bass='drive', drums='drive', lead=6)

print('\n'.join(os.path.basename(s) for s in SONGS))
