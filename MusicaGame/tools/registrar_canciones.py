#!/usr/bin/env python3
r"""
registrar_canciones.py - Biblioteca de canciones de MusicaGame y exportación a la microSD.

Las canciones YA NO van en la flash del STM: sus .h (los que genera midi_to_c.py) se guardan en
MusicaGame/canciones/ (fuera de Core: no se compilan) y de ahí se convierten a .ghs para la microSD.
MusicaGame solo lee la tarjeta.

Uso:
    python registrar_canciones.py                     (lista la biblioteca: canciones, notas, avisos)
    python registrar_canciones.py --sd E:\            (escribe TODAS a la microSD: E:\CANCIONES\*.ghs)
    python registrar_canciones.py --sd carpeta         (o a una carpeta, para copiarla después a la tarjeta)
    python registrar_canciones.py --renombrar viejo nuevo     (cambia el nombre C de una canción)
    python registrar_canciones.py --quitar nombre             (borra los .h de una canción de la biblioteca)
    python registrar_canciones.py --biblioteca RUTA           (otra carpeta de canciones)

Opcional, dentro del song_<nombre>.h (el script los respeta):
    #define SONG_<NOMBRE>_TITULO "Texto del menu"     (si no, se usa el nombre en mayúsculas)
    #define SONG_<NOMBRE>_MASTER 160                  (volumen maestro si la canción recorta; 0 = normal)
"""
import argparse
import glob
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
BIBLIOTECA = os.path.normpath(os.path.join(AQUI, '..', 'canciones'))           # los song_*.h
NOTAS_H = os.path.normpath(os.path.join(AQUI, '..', 'Core', 'Inc', 'notes.h'))  # nombres de las notas (C4 = 60...)
MAX_NOTAS = 14000      # = CANCIONES_SD_MAX_NOTAS en Core/Inc/canciones_sd.h
MAX_CANCIONES = 32     # = CANCIONES_SD_MAX


def leer(path):
    with open(path, encoding='utf-8', errors='replace', newline='') as f:
        return f.read()


def escribir(path, txt):
    """Escribe con fin de línea de Windows (como el resto del proyecto en STM32CubeIDE)."""
    txt = txt.replace('\r\n', '\n').replace('\n', '\r\n')
    with open(path, 'w', encoding='utf-8', newline='') as f:
        f.write(txt)


def canciones(inc):
    """[(nombre, datos)] de cada song_<nombre>.h generado por midi_to_c.py (no los _canalN.h)."""
    res = []
    for path in sorted(glob.glob(os.path.join(inc, 'song_*.h'))):
        fn = os.path.basename(path)
        if re.search(r'_canal\d+\.h$', fn):
            continue
        txt = leer(path)
        m = re.search(r'static\s+const\s+Song\s+song_(\w+)\s*=', txt)
        if not m or 'midi_to_c.py' not in txt:
            continue
        nombre = m.group(1)
        U = nombre.upper()
        d = dict(archivo=fn, U=U,
                 canal1=f'{nombre}_canal1' if re.search(rf'\b{nombre}_canal1\[\]', txt) else None,
                 canal2=f'{nombre}_canal2' if re.search(rf'\b{nombre}_canal2\[\]', txt) else None)
        t = re.search(rf'#define\s+SONG_{U}_TITULO\s+"([^"]*)"', txt)
        d['titulo'] = t.group(1) if t else nombre.replace('_', ' ').upper()
        t = re.search(rf'#define\s+SONG_{U}_MASTER\s+(\d+)', txt)
        d['master'] = int(t.group(1)) if t else 0
        t = re.search(rf'#define\s+SONG_{U}_ORDEN\s+(\d+)', txt)
        d['orden'] = int(t.group(1)) if t else 1000
        res.append((nombre, d))
    res.sort(key=lambda x: (x[1]['orden'], x[0]))
    return res


def listar(bib):
    lista = canciones(bib)
    print(f"Biblioteca {bib}: {len(lista)} canciones")
    for i, (nombre, d) in enumerate(lista):
        avisos = []
        if not d['canal1']:
            avisos.append("sin canal 1: el jugador 1 no tiene notas")
        if not d['canal2']:
            avisos.append("sin canal 2: el jugador 2 no tiene notas")
        print(f"  {i:2}  {nombre:<36} \"{d['titulo']}\"" + (f"   ({'; '.join(avisos)})" if avisos else ""))
    if len(lista) > MAX_CANCIONES:
        print(f"  ! El menú muestra máximo {MAX_CANCIONES} (CANCIONES_SD_MAX)")
    return lista


def renombrar(inc, viejo, nuevo):
    """song_viejo*.h -> song_nuevo*.h, cambiando también los nombres dentro de los archivos."""
    archivos = sorted(glob.glob(os.path.join(inc, f'song_{viejo}.h')) +
                      glob.glob(os.path.join(inc, f'song_{viejo}_canal*.h')))
    if not archivos:
        sys.exit(f"No hay song_{viejo}.h en {inc}")
    pat = re.compile(rf'(?<![A-Za-z0-9]){re.escape(viejo)}(?=_|\b)')
    patU = re.compile(rf'(?<![A-Za-z0-9]){re.escape(viejo.upper())}(?=_|\b)')
    for path in archivos:
        txt = leer(path)
        txt = patU.sub(nuevo.upper(), pat.sub(nuevo, txt))
        nuevo_fn = os.path.basename(path).replace(f'song_{viejo}', f'song_{nuevo}', 1)
        escribir(os.path.join(inc, nuevo_fn), txt)
        if nuevo_fn != os.path.basename(path):
            os.remove(path)
        print(f"  {os.path.basename(path)} -> {nuevo_fn}")


def quitar(inc, nombre):
    archivos = glob.glob(os.path.join(inc, f'song_{nombre}.h')) + glob.glob(os.path.join(inc, f'song_{nombre}_*.h'))
    for path in archivos:
        os.remove(path)
        print(f"  borrado {os.path.basename(path)}")
    if not archivos:
        print(f"  no había archivos de {nombre}")


# ----------------------------------------------------------------------------------------------
# Exportar a microSD (.ghs): lee los song_<nombre>.h del proyecto y escribe un archivo binario por
# canción que MusicaGame carga a RAM al elegirla (ver Core/Inc/canciones_sd.h para el formato).
# ----------------------------------------------------------------------------------------------
INSTR = {'LEAD': 0, 'GUITAR': 1, 'BASS': 2, 'SYNTH': 3, 'DRUMS': 4}
PPQ = 48
DUR = {'WHOLE': PPQ * 4, 'HALF': PPQ * 2, 'QUARTER': PPQ, 'EIGHTH': PPQ // 2, 'SIXTEENTH': PPQ // 4,
       'THIRTYSECOND': PPQ // 8}


def _notas_h():
    m = {'REST': 0}
    for nombre, val in re.findall(r'#define\s+([A-G][SB]?\d)\s+(\d+)', leer(NOTAS_H)):
        m[nombre] = int(val)
    return m


def _dur(txt):
    env = dict(DUR)
    env['DOTTED'] = lambda d: d * 3 // 2
    env['TRIPLET'] = lambda d: d * 2 // 3
    return int(eval(txt, {'__builtins__': {}}, env))


def leer_cancion(inc, nombre, d):
    """Devuelve dict con bpm, pistas [(inst, vol, [(nota, dur, vel, flags)])], p1, p2."""
    U = d['U']
    txt = leer(os.path.join(inc, d['archivo']))
    for fn in sorted(glob.glob(os.path.join(inc, f'song_{nombre}_canal*.h'))):
        txt += "\n" + leer(fn)
    notas_h = _notas_h()
    arreglos = {}
    for arr, cuerpo in re.findall(r'static\s+const\s+Note\s+(\w+)\s*\[\]\s*=\s*\{(.*?)\};', txt, re.S):
        lista = []
        for el in re.findall(r'\{([^{}]*)\}', cuerpo):
            p = [x.strip() for x in el.split(',') if x.strip()]
            nota = int(p[0]) if p[0].isdigit() else notas_h[p[0]]
            dur = _dur(p[1])
            vel = int(p[2]) if len(p) > 2 and p[2] != 'NOTE_CHORD' else 0
            flags = 1 if 'NOTE_CHORD' in p else 0
            lista.append((nota, dur, vel, flags))
        arreglos[arr] = lista
    m = re.search(rf'static\s+const\s+TrackDef\s+\w+\s*\[\]\s*=\s*\{{(.*?)\}};', txt, re.S)
    pistas = []
    for arr, inst, vol in re.findall(r'TRACK\(\s*(\w+)\s*,\s*INST_(\w+)\s*,\s*VOL\(\s*([0-9.]+)\s*\)\s*\)', m.group(1)):
        pistas.append((INSTR[inst], int(float(vol) * 255.0 + 0.5), arreglos[arr]))
    def canal(c):
        mm = re.search(rf'\b{nombre}_canal{c}\s*\[\]\s*[^=]*=\s*\{{([^}}]*)\}}', txt)
        return [int(x) for x in mm.group(1).split(',') if x.strip()] if mm else []
    bpm = int(re.search(rf'#define\s+SONG_{U}_BPM\s+(\d+)', txt).group(1))
    return dict(bpm=bpm, pistas=pistas, p1=canal(1), p2=canal(2))


def escribir_ghs(path, titulo, bpm, pistas, p1, p2, master=0):
    import struct
    if len(pistas) > 16:
        raise ValueError("más de 16 pistas")
    h = bytearray(b'GHS1')
    h += struct.pack('<HB', bpm, len(pistas))
    h += bytes([min(len(p1), 8)]) + bytes((p1 + [0] * 8)[:8])
    h += bytes([min(len(p2), 8)]) + bytes((p2 + [0] * 8)[:8])
    h += b'\x01' + struct.pack('<H', master)        # bandera 1 = al final va la suma de verificación
    t = titulo.encode('ascii', 'replace')[:31]
    h += t + b'\0' * (32 - len(t))
    assert len(h) == 60
    for inst, vol, notas in pistas:
        h += struct.pack('<BBH', inst, vol, len(notas))
    for inst, vol, notas in pistas:
        for n, d, v, f in notas:
            h += struct.pack('<BHBB', n, d, v, f)
    h += struct.pack('<I', sum(h) & 0xFFFFFFFF)     # suma de verificación (el STM vuelve a leer si no cuadra)
    with open(path, 'wb') as fh:
        fh.write(h)
    return len(h), sum(len(p[2]) for p in pistas)


def exportar_sd(bib, destino, solo=None):
    """Escribe las canciones de la biblioteca (o solo la llamada 'solo') en DESTINO/CANCIONES/<nombre>.ghs."""
    carpeta = os.path.join(destino, 'CANCIONES')
    os.makedirs(carpeta, exist_ok=True)
    hechas, avisos = [], []
    for nombre, d in canciones(bib):
        if solo and nombre != solo:
            continue
        c = leer_cancion(bib, nombre, d)
        path = os.path.join(carpeta, f'{nombre}.ghs')
        bytes_, notas = escribir_ghs(path, d['titulo'], c['bpm'], c['pistas'], c['p1'], c['p2'], d['master'])
        hechas.append((nombre, d['titulo'], notas, bytes_))
        aviso = ""
        if notas > MAX_NOTAS:
            aviso = f"  ! {notas} notas: no cabe en la RAM (máx. {MAX_NOTAS}), el STM la va a rechazar"
            avisos.append(f"{d['titulo']}: {notas} notas (máx. {MAX_NOTAS})")
        print(f"  {path}  {notas} notas, {bytes_} bytes{aviso}")
    print(f"{len(hechas)} canciones en {carpeta}")
    return hechas, avisos


def main():
    ap = argparse.ArgumentParser(description="Biblioteca de canciones de MusicaGame -> microSD (.ghs)")
    ap.add_argument('--biblioteca', default=BIBLIOTECA, help='carpeta con los song_*.h (MusicaGame/canciones)')
    ap.add_argument('--renombrar', nargs=2, metavar=('VIEJO', 'NUEVO'))
    ap.add_argument('--quitar', metavar='NOMBRE')
    ap.add_argument('--sd', metavar='DESTINO', help='escribe las canciones en DESTINO/CANCIONES/*.ghs (la microSD)')
    a = ap.parse_args()
    if a.renombrar:
        renombrar(a.biblioteca, *a.renombrar)
    if a.quitar:
        quitar(a.biblioteca, a.quitar)
    listar(a.biblioteca)
    if a.sd:
        exportar_sd(a.biblioteca, a.sd)


if __name__ == '__main__':
    main()
