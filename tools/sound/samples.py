#!/usr/bin/env python3
"""Original drum samples and sound effects for Flicki's Escape (8-bit mono WAV, 13300 Hz).
Run from repo root: python3 tools/sound/samples.py"""
import os, math, random, wave, struct
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..'))
PCM = os.path.join(ROOT, 'tools', 'sound', 'pcm')        # drum kit used by the MML songs
SFX = os.path.join(ROOT, 'res', 'snd')                    # game sound effects
RATE = 13300
os.makedirs(PCM, exist_ok=True); os.makedirs(SFX, exist_ok=True)

def save(path, s, gain=0.9):
    peak = max(1e-6, max(abs(v) for v in s))
    with wave.open(path, 'wb') as w:
        w.setnchannels(1); w.setsampwidth(1); w.setframerate(RATE)
        w.writeframes(bytes(int(128 + max(-1, min(1, v / peak * gain)) * 127) for v in s))

def env(t, a, d):   # attack/decay envelope (seconds)
    if t < a: return t / a
    return math.exp(-(t - a) / d)

def tone(freqs, dur, shape='sq', a=0.002, d=0.08, duty=0.5, vib=0.0):
    out = []; ph = 0.0; n = int(dur * RATE)
    for i in range(n):
        t = i / RATE
        f = freqs(t) if callable(freqs) else freqs
        f *= 1 + vib * math.sin(2 * math.pi * 7 * t)
        ph = (ph + f / RATE) % 1.0
        if shape == 'sq': v = 1.0 if ph < duty else -1.0
        elif shape == 'tri': v = 4 * abs(ph - 0.5) - 1
        else: v = math.sin(2 * math.pi * ph)
        out.append(v * env(t, a, d))
    return out

def mix(*parts):
    n = max(len(p) for p in parts); out = [0.0] * n
    for p in parts:
        for i, v in enumerate(p): out[i] += v
    return out

def seq(*parts):
    out = []
    for p in parts: out += p
    return out

def silence(dur): return [0.0] * int(dur * RATE)

random.seed(7)
def noise(dur, d, lp=0.0):
    out = []; prev = 0.0
    for i in range(int(dur * RATE)):
        v = random.uniform(-1, 1)
        prev = prev * lp + v * (1 - lp)
        out.append(prev * env(i / RATE, 0.001, d))
    return out

def hp(s, k=0.85):
    out = []; px = 0.0; py = 0.0
    for x in s:
        y = k * (py + x - px); px, py = x, y; out.append(y)
    return out

# ---------------- drum kit (music)
kick = tone(lambda t: 40 + 110 * math.exp(-t / 0.03), 0.16, 'sine', 0.001, 0.07)
save(os.path.join(PCM, 'kick.wav'), kick)
snare = mix([v * 0.5 for v in tone(lambda t: 190 - 40 * t, 0.15, 'tri', 0.001, 0.05)], noise(0.15, 0.05, 0.2))
save(os.path.join(PCM, 'snare.wav'), snare)

# ---------------- sound effects (game)
N = lambda m: 440 * 2 ** ((m - 69) / 12)
save(os.path.join(SFX, 'sfx_click.wav'), tone(N(84), 0.05, 'sq', 0.001, 0.02, 0.25))
save(os.path.join(SFX, 'sfx_pio.wav'), tone(lambda t: 900 + 9000 * t, 0.09, 'sq', 0.002, 0.05, 0.25))          # chick picked up
for k in range(5):                                                                                          # chick into the nest
    m = 76 + [0, 4, 7, 12, 16][k]
    save(os.path.join(SFX, f'sfx_nido{k}.wav'), mix(tone(N(m), 0.16, 'sq', 0.002, 0.08, 0.5), [v * 0.4 for v in tone(N(m + 12), 0.16, 'tri', 0.002, 0.06)]))
save(os.path.join(SFX, 'sfx_gema.wav'), seq(*[tone(N(m), 0.045, 'sq', 0.001, 0.04, 0.25) for m in (84, 88, 91, 96, 100)]))
save(os.path.join(SFX, 'sfx_reloj.wav'), seq(tone(N(79), 0.06, 'tri', 0.001, 0.02), silence(0.07), tone(N(72), 0.06, 'tri', 0.001, 0.02),
                                             silence(0.07), tone(N(79), 0.06, 'tri', 0.001, 0.02), silence(0.07), tone(N(72), 0.06, 'tri', 0.001, 0.02)))
save(os.path.join(SFX, 'sfx_rayo.wav'), tone(lambda t: 200 * 2 ** (t * 24), 0.25, 'sq', 0.001, 0.2, 0.5))
save(os.path.join(SFX, 'sfx_meta.wav'), seq(*[tone(N(m), d, 'sq', 0.002, 0.1, 0.5) for m, d in ((72, 0.08), (76, 0.08), (79, 0.08), (84, 0.25))]))
save(os.path.join(SFX, 'sfx_cierre.wav'), mix(tone(lambda t: 110 - 60 * t, 0.35, 'sq', 0.002, 0.2, 0.5), noise(0.35, 0.15, 0.6)))
save(os.path.join(SFX, 'sfx_alerta.wav'), seq(tone(N(88), 0.07, 'sq', 0.001, 0.06, 0.5), silence(0.04), tone(N(88), 0.07, 'sq', 0.001, 0.06, 0.5)))
save(os.path.join(SFX, 'sfx_panico.wav'), tone(lambda t: 700 + 300 * math.sin(2 * math.pi * 14 * t), 0.4, 'sq', 0.002, 0.3, 0.5))
save(os.path.join(SFX, 'sfx_atrapado.wav'), seq(tone(lambda t: 900 * 2 ** (-t * 3.2), 0.6, 'sq', 0.002, 0.5, 0.5, 0.02),
                                                 tone(N(43), 0.25, 'tri', 0.002, 0.12)))
save(os.path.join(SFX, 'sfx_rabia.wav'), mix(tone(lambda t: 80 + 40 * math.sin(2 * math.pi * 30 * t), 0.5, 'sq', 0.01, 0.35, 0.3),
                                              [v * 0.5 for v in noise(0.5, 0.3, 0.5)]))
print('samples written')
