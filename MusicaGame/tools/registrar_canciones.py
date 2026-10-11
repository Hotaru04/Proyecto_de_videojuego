#!/usr/bin/env python3
"""
registrar_canciones.py - Indexa las canciones del proyecto MusicaGame.

Busca en Core/Inc todos los song_<nombre>.h generados por midi_to_c.py y escribe Core/Inc/canciones_tabla.h:
    - los #include de cada canción,
    - CANCION_<NOMBRE> = su número (para song_index),
    - la fila de cada una para song_table[] (pistas del jugador 1 = canal 1, jugador 2 = canal 2).
songs.c ya incluye ese archivo: no hay que tocar código a mano.

Uso:
    python registrar_canciones.py                      (busca ../Core/Inc desde esta carpeta)
    python registrar_canciones.py --inc RUTA/Core/Inc
    python registrar_canciones.py --renombrar viejo nuevo     (cambia el nombre C de una canción ya exportada)
    python registrar_canciones.py --quitar nombre             (borra los .h de una canción)
    python registrar_canciones.py --sd E:\                   (escribe las canciones a la microSD: E:\CANCIONES\*.ghs)
    python registrar_canciones.py --sd carpeta                (o a una carpeta, para copiarla después a la tarjeta)

Opcional, dentro del song_<nombre>.h (el script los respeta):
    #define SONG_<NOMBRE>_TITULO "Texto del menu"     (si no, se usa el nombre en mayúsculas)
    #define SONG_<NOMBRE>_MASTER 160                  (volumen maestro si la canción recorta; 0 = normal)
    #define SONG_<NOMBRE>_ORDEN  1                    (posición en el menú; si no, orden alfabético)
"""
import argparse
import glob
import os
import re
import sys

AQUI = os.path.dirname(os.path.abspath(__file__))
INC_POR_DEFECTO = os.path.normpath(os.path.join(AQUI, '..', 'Core', 'Inc'))
TABLA = 'canciones_tabla.h'


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


def registrar(inc):
    lista = canciones(inc)
    if not lista:
        sys.exit(f"No encontré canciones (song_*.h de midi_to_c.py) en {inc}")
    L = ["/*",
         f" * {TABLA}",
         " * GENERADO por tools/registrar_canciones.py  ->  NO editar a mano (se sobrescribe).",
         " * Para agregar una canción: expórtala con midi_to_c_gui.py (botón \"Agregar al juego\")",
         " * o copia sus .h a Core/Inc y corre:  python tools/registrar_canciones.py",
         " */",
         "#ifndef INC_CANCIONES_TABLA_H_",
         "#define INC_CANCIONES_TABLA_H_",
         ""]
    for nombre, d in lista:
        L.append(f'#include "{d["archivo"]}"')
    L += ["", "/* Número de cada canción (para song_index en main.c) */"]
    for i, (nombre, d) in enumerate(lista):
        L.append(f"#define CANCION_{d['U']:<40} {i}")
    L += [f"#define CANCIONES_TOTAL {len(lista)}", "",
          "/* FILA(cancion, titulo, pistas_j1, n_j1, pistas_j2, n_j2, volumen_maestro) -> la define songs.c */",
          "#define CANCIONES_FILAS \\"]
    for nombre, d in lista:
        U = d['U']
        c1 = f"{d['canal1']}, SONG_{U}_CANAL1_N" if d['canal1'] else "0, 0"
        c2 = f"{d['canal2']}, SONG_{U}_CANAL2_N" if d['canal2'] else "0, 0"
        L.append(f'\tFILA(song_{nombre}, "{d["titulo"]}", {c1}, {c2}, {d["master"]}), \\')
    L += ["", "#endif /* INC_CANCIONES_TABLA_H_ */", ""]
    escribir(os.path.join(inc, TABLA), "\n".join(L))

    print(f"{TABLA}: {len(lista)} canciones")
    for i, (nombre, d) in enumerate(lista):
        avisos = []
        if not d['canal1']:
            avisos.append("sin canal 1: el jugador 1 no tiene notas")
        if not d['canal2']:
            avisos.append("sin canal 2: el jugador 2 no tiene notas")
        print(f"  {i}  CANCION_{d['U']:<32} \"{d['titulo']}\"" + (f"   ({'; '.join(avisos)})" if avisos else ""))
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


def _notas_h(inc):
    m = {'REST': 0}
    for nombre, val in re.findall(r'#define\s+([A-G][SB]?\d)\s+(\d+)', leer(os.path.join(inc, 'notes.h'))):
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
    notas_h = _notas_h(inc)
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
    h += b'\0' + struct.pack('<H', master)
    t = titulo.encode('ascii', 'replace')[:31]
    h += t + b'\0' * (32 - len(t))
    assert len(h) == 60
    for inst, vol, notas in pistas:
        h += struct.pack('<BBH', inst, vol, len(notas))
    for inst, vol, notas in pistas:
        for n, d, v, f in notas:
            h += struct.pack('<BHBB', n, d, v, f)
    with open(path, 'wb') as fh:
        fh.write(h)
    return len(h), sum(len(p[2]) for p in pistas)


def exportar_sd(inc, destino):
    carpeta = os.path.join(destino, 'CANCIONES')
    os.makedirs(carpeta, exist_ok=True)
    total = 0
    for nombre, d in canciones(inc):
        c = leer_cancion(inc, nombre, d)
        path = os.path.join(carpeta, f'{nombre}.ghs')
        bytes_, notas = escribir_ghs(path, d['titulo'], c['bpm'], c['pistas'], c['p1'], c['p2'], d['master'])
        total += 1
        aviso = "  (más de 10000 notas: no cabe en la RAM, ver CANCIONES_SD_MAX_NOTAS)" if notas > 10000 else ""
        print(f"  {path}  {notas} notas, {bytes_} bytes{aviso}")
    print(f"{total} canciones en {carpeta}")


def main():
    ap = argparse.ArgumentParser(description="Indexa las canciones de MusicaGame (genera canciones_tabla.h)")
    ap.add_argument('--inc', default=INC_POR_DEFECTO, help='carpeta Core/Inc del proyecto MusicaGame')
    ap.add_argument('--renombrar', nargs=2, metavar=('VIEJO', 'NUEVO'))
    ap.add_argument('--quitar', metavar='NOMBRE')
    ap.add_argument('--sd', metavar='DESTINO', help='escribe las canciones en DESTINO/CANCIONES/*.ghs (la microSD)')
    a = ap.parse_args()
    if a.renombrar:
        renombrar(a.inc, *a.renombrar)
    if a.quitar:
        quitar(a.inc, a.quitar)
    registrar(a.inc)
    if a.sd:
        exportar_sd(a.inc, a.sd)


if __name__ == '__main__':
    main()
