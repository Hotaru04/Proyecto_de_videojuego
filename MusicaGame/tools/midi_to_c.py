#!/usr/bin/env python3
"""
midi_to_c.py - Convierte un archivo MIDI en un .h de canción para el motor de audio del STM32.

Instalación:   pip install mido

Ver qué trae el MIDI:
    python midi_to_c.py cancion.mid --list

Convertir todo (una pista de salida por cada pista MIDI con notas; el instrumento se adivina):
    python midi_to_c.py cancion.mid -o song_cancion.h

Elegir y combinar pistas:   --out "nombre=selectores:INSTRUMENTO:volumen[:canal]"
    canal (1..8, opcional): agrupa pistas; cada una conserva su instrumento.
    selectores: t3 = pista MIDI 3, c10 = canal MIDI 10 (1..16), separados por coma.
    python midi_to_c.py cancion.mid -o song_cancion.h \
        --out "lead=t1:LEAD:0.40" --out "guitar=t2,t3:GUITAR:0.30" \
        --out "bass=t4:BASS:0.35" --out "drums=c10:DRUMS:0.25"

Opciones:
    --split             un .h por canal (song_x_canal1.h, song_x_canal2.h...) y song_x.h que los junta
    --name cancion      nombre C de la canción (por defecto, el del archivo)
    --grid 12           cuantiza a 12 ticks (semicorchea con PPQ=48). Por defecto 1 (sin cuantizar)
    --bpm 120           forzar BPM (si no, se toma el primer cambio de tempo del MIDI)
    --juego             escribir los .h en ../Core/Inc de MusicaGame y registrarla en canciones_tabla.h
                        (canal 1 = notas del jugador 1, canal 2 = jugador 2). Usa un --name corto.

Formato generado: {nota, duración_en_ticks, velocidad[, NOTE_CHORD]}
    - Los silencios se escriben como {REST, ticks}.
    - Notas que empiezan juntas (acordes) usan NOTE_CHORD: la siguiente arranca al mismo tiempo.
    - Todas las pistas empiezan en el tick 0 del MIDI, así que se conserva la sincronización original.
"""
import argparse
import os
import re
import sys
from collections import defaultdict

try:
    import mido
except ImportError:
    sys.exit("Falta mido:  pip install mido")

PPQ = 48                      # debe coincidir con MUSIC_PPQ en music.h
MAX_U16 = 65535
NAMES = ['C', 'CS', 'D', 'DS', 'E', 'F', 'FS', 'G', 'GS', 'A', 'AS', 'B']
INSTRUMENTS = ['LEAD', 'GUITAR', 'BASS', 'SYNTH', 'DRUMS']
DUR_NAMES = {
    192: 'WHOLE', 96: 'HALF', 48: 'QUARTER', 24: 'EIGHTH', 12: 'SIXTEENTH', 6: 'THIRTYSECOND',
    288: 'DOTTED(WHOLE)', 144: 'DOTTED(HALF)', 72: 'DOTTED(QUARTER)', 36: 'DOTTED(EIGHTH)', 18: 'DOTTED(SIXTEENTH)',
    32: 'TRIPLET(QUARTER)', 16: 'TRIPLET(EIGHTH)', 8: 'TRIPLET(SIXTEENTH)',
}


def note_name(m, drums):
    if drums:
        return str(m)
    if 21 <= m <= 108:
        return f"{NAMES[m % 12]}{m // 12 - 1}"
    return str(m)


def dur_text(t):
    return DUR_NAMES.get(t, str(t))


def c_ident(s):
    s = re.sub(r'[^0-9a-zA-Z_]', '_', s).strip('_').lower()
    return s if s and not s[0].isdigit() else 's_' + s


def load_midi(path):
    mid = mido.MidiFile(path)
    tempos = []
    tracks = []          # por pista MIDI: lista de (inicio, fin, nota, vel, canal)
    names = []
    programs = []
    for ti, tr in enumerate(mid.tracks):
        t = 0
        abiertas = defaultdict(list)   # (canal, nota) -> [(inicio, vel)]
        notas = []
        nombre = tr.name or f"track{ti}"
        prog = {}
        for msg in tr:
            t += msg.time
            if msg.type == 'set_tempo':
                tempos.append((t, msg.tempo))
            elif msg.type == 'program_change':
                prog[msg.channel] = msg.program
            elif msg.type == 'note_on' and msg.velocity > 0:
                abiertas[(msg.channel, msg.note)].append((t, msg.velocity))
            elif msg.type in ('note_off', 'note_on'):
                k = (msg.channel, msg.note)
                if abiertas[k]:
                    ini, vel = abiertas[k].pop(0)
                    notas.append((ini, t, msg.note, vel, msg.channel))
        tracks.append(notas)
        names.append(nombre)
        programs.append(prog)
    return mid, tracks, names, programs, sorted(tempos)


def guess_instrument(notas, prog):
    if any(ch == 9 for *_, ch in notas):
        return 'DRUMS'
    p = next(iter(prog.values()), 0)
    if 32 <= p <= 39:
        return 'BASS'
    if 24 <= p <= 31:
        return 'GUITAR'
    if 80 <= p <= 87:
        return 'SYNTH'
    if notas and sum(n for _, _, n, _, _ in notas) / len(notas) < 48:
        return 'BASS'
    return 'LEAD'


def select(tracks, selectores):
    out = []
    for sel in selectores:
        sel = sel.strip().lower()
        if sel.startswith('t'):
            out += tracks[int(sel[1:])]
        elif sel.startswith('c'):
            ch = int(sel[1:]) - 1
            out += [n for tr in tracks for n in tr if n[4] == ch]
        else:
            sys.exit(f"Selector inválido: {sel} (usa t<pista> o c<canal>)")
    return out


def to_ticks(t, tpb, grid):
    v = round(t * PPQ / tpb)
    if grid > 1:
        v = round(v / grid) * grid
    return v


def build_sequence(notas, tpb, grid, drums):
    """Convierte (inicio, fin, nota, vel) a la lista de Note con silencios y acordes."""
    evs = []
    for ini, fin, n, vel, _ in notas:
        a = to_ticks(ini, tpb, grid)
        b = to_ticks(fin, tpb, grid)
        d = max(b - a, grid if grid > 1 else 1)
        if not drums:
            while n < 21: n += 12            # transponer por octavas al rango del piano
            while n > 108: n -= 12
        evs.append((a, n, min(d, MAX_U16), vel))
    evs.sort()
    # quitar duplicados exactos (misma nota al mismo tiempo)
    uniq = []
    for e in evs:
        if not uniq or (uniq[-1][0], uniq[-1][1]) != (e[0], e[1]):
            uniq.append(e)
    evs = uniq

    seq = []   # (nota|'REST', dur, vel, chord)

    def rest(ticks):
        while ticks > 0:
            r = min(ticks, MAX_U16)
            seq.append(('REST', r, 0, False))
            ticks -= r

    if not evs:
        return seq, 0
    rest(evs[0][0])
    grupos = defaultdict(list)
    for a, n, d, v in evs:
        grupos[a].append((n, d, v))
    inicios = sorted(grupos)
    max_poli = 0
    for i, a in enumerate(inicios):
        g = sorted(grupos[a])
        max_poli = max(max_poli, len(g))
        hueco = (inicios[i + 1] - a) if i + 1 < len(inicios) else max(d for _, d, _ in g)
        for n, d, v in g[:-1]:
            seq.append((n, d, v, True))
        n, d, v = g[-1]
        if d == hueco:
            seq.append((n, d, v, False))
        elif d < hueco:
            seq.append((n, d, v, False))
            rest(hueco - d)
        else:
            seq.append((n, d, v, True))
            rest(hueco)
    return seq, max_poli


def main():
    ap = argparse.ArgumentParser(description="MIDI -> .h para el motor de audio STM32")
    ap.add_argument('midi')
    ap.add_argument('-o', '--output')
    ap.add_argument('--name')
    ap.add_argument('--list', action='store_true')
    ap.add_argument('--out', action='append', default=[], help='nombre=t1,c10:INSTRUMENTO:volumen')
    ap.add_argument('--grid', type=int, default=1)
    ap.add_argument('--bpm', type=float)
    ap.add_argument('--split', action='store_true',
                    help='un .h por canal (song_x_canalN.h) + song_x.h que los junta')
    ap.add_argument('--juego', action='store_true',
                    help='escribir en ../Core/Inc de MusicaGame y registrar la canción (canciones_tabla.h)')
    args = ap.parse_args()

    mid, tracks, names, programs, tempos = load_midi(args.midi)
    tpb = mid.ticks_per_beat

    if args.list:
        print(f"{args.midi}: tipo {mid.type}, {tpb} ticks/negra, {len(tracks)} pistas")
        for t, tempo in tempos[:5]:
            print(f"  tempo en tick {t}: {mido.tempo2bpm(tempo):.2f} BPM")
        for i, (tr, nm) in enumerate(zip(tracks, names)):
            if not tr:
                print(f"  t{i}: '{nm}' (sin notas)")
                continue
            chs = sorted({c + 1 for *_, c in tr})
            lo, hi = min(n for _, _, n, _, _ in tr), max(n for _, _, n, _, _ in tr)
            print(f"  t{i}: '{nm}', {len(tr)} notas, canales {chs}, rango {note_name(lo, 0)}..{note_name(hi, 0)}, "
                  f"instrumento sugerido {guess_instrument(tr, programs[i])}")
        return

    if args.bpm:
        bpm = args.bpm
    elif tempos:
        bpm = mido.tempo2bpm(tempos[0][1])
        if len({tp for _, tp in tempos}) > 1:
            print("AVISO: el MIDI tiene varios cambios de tempo; se usa el primero "
                  f"({bpm:.1f} BPM). Las duraciones musicales se conservan.", file=sys.stderr)
    else:
        bpm = 120.0
    bpm = int(round(bpm))

    # qué pistas de salida generar
    salidas = []
    if args.out:
        for spec in args.out:
            m = re.match(r'^\s*([^=]+)=([^:]+)(?::([A-Za-z]+))?(?::([0-9.]+))?(?::([0-9]+))?\s*$', spec)
            if not m:
                sys.exit(f"--out inválido: {spec}")
            nombre, sels, inst, vol, canal = m.groups()
            notas = select(tracks, sels.split(','))
            inst = (inst or guess_instrument(notas, {})).upper()
            if inst not in INSTRUMENTS:
                sys.exit(f"Instrumento '{inst}' no existe. Opciones: {INSTRUMENTS}")
            salidas.append((c_ident(nombre), notas, inst, float(vol) if vol else 0.3, int(canal) if canal else 1))
    else:
        for i, tr in enumerate(tracks):
            if tr:
                salidas.append((c_ident(names[i]) or f"t{i}", tr, guess_instrument(tr, programs[i]), 0.3))

    song = c_ident(args.name or os.path.splitext(os.path.basename(args.midi))[0])
    out = args.output or f"song_{song}.h"
    if args.juego:
        out = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'Core', 'Inc', f"song_{song}.h")
    files, stats = generate_files(os.path.basename(args.midi), tpb, salidas, song, bpm, args.grid,
                                  args.split, os.path.basename(out))
    outdir = os.path.dirname(os.path.abspath(out))
    for fn, txt in files:
        with open(os.path.join(outdir, fn), 'w', encoding='utf-8', newline='') as fh:
            fh.write(txt.replace('\r\n', '\n').replace('\n', '\r\n'))
        print(f"  -> {fn}")
    for st in stats:
        print(f"  canal {st['canal']} · {st['nombre']}: {st['eventos']} eventos, polifonía máx. {st['poli']}, {st['inst']}, vol {st['vol']}")
    total = sum(st['bytes'] for st in stats)
    print(f"-> {out}  ({total} bytes de flash, {bpm} BPM, PPQ {PPQ})")
    if len(stats) > 16:
        print("AVISO: más de 16 pistas (MUSIC_MAX_TRACKS)", file=sys.stderr)
    if args.juego:
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        import registrar_canciones
        registrar_canciones.registrar(outdir)


def seq_to_c(seq, drums):
    """Lista de Note en texto C, 6 por línea."""
    A, linea = [], []
    for n, d, v, ch in seq:
        if n == 'REST':
            txt = f"{{REST, {dur_text(d)}}}"
        else:
            txt = f"{{{note_name(n, drums)}, {dur_text(d)}, {v}{', NOTE_CHORD' if ch else ''}}}"
        linea.append(txt)
        if len(linea) == 6:
            A.append("\t" + ", ".join(linea) + ",")
            linea = []
    if linea:
        A.append("\t" + ", ".join(linea) + ",")
    if not seq:
        A.append("\t{REST, 1},")
    return A


def generate_files(midi_name, tpb, salidas, song, bpm, grid, split, out_name):
    """salidas: [(nombre, notas_midi, INSTRUMENTO, volumen[, canal])]. Devuelve ([(archivo, texto)], [stats]).
    Canal: agrupa pistas (cada una conserva su instrumento). Las pistas se ordenan por canal y la canción
    trae, por canal, la lista de índices de sus pistas. Con split se genera un .h por canal."""
    guard = f"SONG_{song.upper()}_H_"
    U = song.upper()
    salidas = [tuple(s) + (1,) if len(s) == 4 else tuple(s) for s in salidas]
    salidas = sorted(salidas, key=lambda s: s[4])                 # orden estable por canal
    canales = sorted({s[4] for s in salidas})
    L = [f"/*", f" * {out_name}", f" * Generado por midi_to_c.py desde {midi_name}",
         f" * Incluir en UN solo .c. Los arreglos son static const (flash).", f" */",
         f"#ifndef {guard}", f"#define {guard}", "", '#include "music.h"', '#include "synth.h"', "",
         f"#define SONG_{U}_BPM {bpm}", ""]
    files, stats, defs = [], [], []
    por_canal = {c: [] for c in canales}          # canal -> [(texto del arreglo)]
    idx_canal = {c: [] for c in canales}          # canal -> índices en *_tracks[]
    for i, (nombre, notas, inst, vol, canal) in enumerate(salidas):
        drums = inst == 'DRUMS'
        seq, poli = build_sequence(notas, tpb, grid, drums)
        arr = f"{song}_{nombre}"
        A = [f"/* {nombre}: canal {canal}, {len(seq)} eventos, polifonía máx. {poli}, instrumento {inst} */",
             f"static const Note {arr}[] = {{"] + seq_to_c(seq, drums) + ["};", ""]
        por_canal[canal] += A
        idx_canal[canal].append(i)
        defs.append((canal, f"\tTRACK({arr}, INST_{inst}, VOL({vol:.2f})),   /* pista {i} */"))
        stats.append(dict(nombre=nombre, eventos=len(seq), poli=poli, inst=inst, vol=vol, canal=canal,
                          bytes=6 * max(1, len(seq)), seq=seq, drums=drums))
    for c in canales:
        if split:
            fn = f"song_{song}_canal{c}.h"
            g = f"SONG_{U}_CANAL{c}_H_"
            nombres = ", ".join(stats[i]['nombre'] for i in idx_canal[c])
            P = [f"/*", f" * {fn}", f" * Canal {c} de {midi_name}: {nombres}  (generado por midi_to_c.py)", f" */",
                 f"#ifndef {g}", f"#define {g}", "", '#include "music.h"', ""] + por_canal[c] + [f"#endif /* {g} */", ""]
            files.append((fn, "\n".join(P)))
            L.append(f'#include "{fn}"')
        else:
            L += [f"/* ======================== Canal {c} ======================== */"] + por_canal[c]
    if split:
        L.append("")
    L.append(f"static const TrackDef {song}_tracks[] = {{")
    ultimo = None
    for c, d in defs:
        if c != ultimo:
            L.append(f"\t/* canal {c} */")
            ultimo = c
        L.append(d)
    L += ["};", "", f'static const Song song_{song} = {{ "{song}", SONG_{U}_BPM, {len(defs)}, {song}_tracks }};', ""]
    # índices de pista por canal: para subir/bajar/silenciar un canal completo con Music_SetTrackVolume
    L += [f"/* Canales: índices de pista (en {song}_tracks[]) que pertenecen a cada canal.",
          f" * Ej.: for (i = 0; i < SONG_{U}_CANAL{canales[0]}_N; i++) Music_SetTrackVolume({song}_canal{canales[0]}[i], 0); */",
          f"#define SONG_{U}_NUM_CANALES {len(canales)}"]
    for c in canales:
        L.append(f"#define SONG_{U}_CANAL{c}_N {len(idx_canal[c])}")
        L.append(f"static const uint8_t {song}_canal{c}[] __attribute__((unused)) = {{ {', '.join(map(str, idx_canal[c]))} }};")
    L += ["", f"#endif /* {guard} */", ""]
    files.append((out_name, "\n".join(L)))
    return files, stats


if __name__ == '__main__':
    main()
