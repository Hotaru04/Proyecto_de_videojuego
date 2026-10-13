#!/usr/bin/env python3
"""
midi_to_c_gui.py - Interfaz gráfica del conversor MIDI -> C para el motor de audio del STM32.

    pip install mido
    python midi_to_c_gui.py            (o doble clic)
    python midi_to_c_gui.py cancion.mid

Necesita midi_to_c.py en la misma carpeta.

  - Abrir MIDI: lista sus pistas; en cada una eliges pista de salida, instrumento y volumen.
  - Piano roll, código C y memoria de lo que se va a exportar.
  - Exportar .h: escribe los .h en la carpeta que elijas.
  - Guardar en microSD: guarda la canción en MusicaGame/canciones/ y escribe todas las canciones
    como .ghs en <tarjeta>/CANCIONES/. Canal 1 = jugador 1, canal 2 = jugador 2.
"""
import os
import sys
import tkinter as tk
from tkinter import ttk, filedialog, messagebox

AQUI = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, AQUI)


def _error(msg):
    r = tk.Tk(); r.withdraw()
    messagebox.showerror("Falta algo", msg)
    sys.exit(1)


# 1) mido: se instala en ESTE mismo Python (en Windows suele haber varios y 'pip' instala en otro)
try:
    import mido
except ImportError:
    import subprocess
    r = tk.Tk(); r.withdraw()
    if not messagebox.askyesno("Falta mido",
                               f"Este Python no tiene la librería 'mido':\n{sys.executable}\n\n¿Instalarla ahora?"):
        sys.exit(1)
    res = subprocess.run([sys.executable, "-m", "pip", "install", "--user", "mido"], capture_output=True, text=True)
    r.destroy()
    try:
        import site, importlib
        importlib.invalidate_caches()
        if site.getusersitepackages() not in sys.path:
            sys.path.append(site.getusersitepackages())
        import mido
    except ImportError:
        _error("No se pudo instalar mido.\n\nAbre una terminal y ejecuta:\n"
               f'  "{sys.executable}" -m pip install mido\n\n' + (res.stderr or res.stdout)[-600:])

# 2) midi_to_c.py tiene que estar en la misma carpeta
try:
    import midi_to_c as mc
except ImportError:
    _error(f"No encuentro midi_to_c.py en:\n{AQUI}\n\nPon midi_to_c.py en la misma carpeta que este archivo "
           "(los dos están en MusicaGame/tools).")

COLORS = ['#ff4d6d', '#4dabf7', '#ffd43b', '#51cf66', '#cc5de8', '#ff922b', '#22b8cf', '#e599f7']
GRIDS = [('Sin cuantizar', 1), ('1/32 (fusa)', 6), ('1/16 (semicorchea)', 12), ('1/8 (corchea)', 24)]
DRUM_NAMES = {35: 'bombo', 36: 'bombo', 37: 'aro', 38: 'caja', 39: 'palmas', 40: 'caja', 41: 'tom piso',
              42: 'hi-hat cerrado', 43: 'tom piso', 44: 'hi-hat pedal', 45: 'tom', 46: 'hi-hat abierto',
              47: 'tom', 48: 'tom alto', 49: 'crash', 50: 'tom alto', 51: 'ride', 57: 'crash 2'}
BG, PANEL, GRIDC, BARC, TXT = '#16161d', '#1f1f29', '#2a2a36', '#454559', '#d0d0dc'
BAND_H, LABEL_W, RULER_H = 120, 170, 24
MAX_VOICES = 12
FLASH = 512 * 1024


def hex_mix(c, k):
    """Oscurece el color c (k 0..1 = 0 negro .. 1 original)."""
    r, g, b = int(c[1:3], 16), int(c[3:5], 16), int(c[5:7], 16)
    return f"#{int(r * k):02x}{int(g * k):02x}{int(b * k):02x}"


class Row:
    """Una pista del MIDI y cómo se convierte."""
    def __init__(self, idx, name, notas, inst):
        self.idx, self.name, self.notas = idx, name, notas
        self.use = tk.BooleanVar(value=True)
        self.out = tk.StringVar(value=mc.c_ident(name) or f"t{idx}")
        self.inst = tk.StringVar(value=inst)
        self.vol = tk.IntVar(value=40 if inst == 'LEAD' else 30)
        self.canal = tk.IntVar(value=1)


class App(tk.Tk):
    def __init__(self, path=None):
        super().__init__()
        self.title("MIDI → C  ·  STM32 Guitar Hero")
        self.geometry("1280x800")
        self.minsize(980, 600)
        self.configure(bg=BG)
        self.rows, self.files, self.stats = [], [], []
        self.midi_path, self.tpb, self.tempos = None, 480, []
        self.zoom = tk.DoubleVar(value=0.5)          # px por tick
        self.bpm = tk.IntVar(value=120)
        self.grid_name = tk.StringVar(value=GRIDS[0][0])
        self.song = tk.StringVar(value="cancion")
        self.split = tk.BooleanVar(value=True)
        self.status = tk.StringVar(value="Abre un archivo MIDI para empezar.")
        self._pending = None
        self.note_info = {}
        self._style()
        self._build()
        if path:
            self.open_midi(path)

    # ------------------------------------------------------------------ UI
    def _style(self):
        st = ttk.Style(self)
        st.theme_use('clam')
        st.configure('.', background=PANEL, foreground=TXT, fieldbackground='#2b2b38', bordercolor=BARC)
        st.configure('TFrame', background=PANEL)
        st.configure('Top.TFrame', background=BG)
        st.configure('TLabel', background=PANEL, foreground=TXT)
        st.configure('Top.TLabel', background=BG, foreground=TXT)
        st.configure('H.TLabel', background=PANEL, foreground='#8a8aa0', font=('Segoe UI', 9, 'bold'))
        st.configure('TCheckbutton', background=PANEL, foreground=TXT)
        st.configure('Top.TCheckbutton', background=BG, foreground=TXT)
        st.configure('TButton', background='#34344a', foreground='white', padding=6)
        st.map('TButton', background=[('active', '#4a4a66')])
        st.configure('Accent.TButton', background='#ff4d6d', foreground='white', padding=6)
        st.map('Accent.TButton', background=[('active', '#ff6b85')])
        st.configure('TNotebook', background=BG, borderwidth=0)
        st.configure('TNotebook.Tab', background='#2a2a36', foreground=TXT, padding=(14, 6))
        st.map('TNotebook.Tab', background=[('selected', PANEL)])
        st.configure('Treeview', background='#1b1b24', fieldbackground='#1b1b24', foreground=TXT, rowheight=24)
        st.configure('Treeview.Heading', background='#2a2a36', foreground=TXT)
        st.configure('TCombobox', fieldbackground='#2b2b38', foreground=TXT, arrowcolor=TXT)
        st.configure('TSpinbox', fieldbackground='#2b2b38', foreground=TXT, arrowcolor=TXT)
        st.configure('TEntry', fieldbackground='#2b2b38', foreground=TXT)
        st.map('TCombobox', fieldbackground=[('readonly', '#2b2b38')], foreground=[('readonly', TXT)],
               selectbackground=[('readonly', '#2b2b38')], selectforeground=[('readonly', TXT)],
               background=[('readonly', '#34344a')])
        st.configure('Horizontal.TScale', background='#6a6a88', troughcolor='#2b2b38', bordercolor=BARC)
        st.configure('Vertical.TScrollbar', background='#34344a', troughcolor=BG, arrowcolor=TXT)
        st.configure('Horizontal.TScrollbar', background='#34344a', troughcolor=BG, arrowcolor=TXT)
        self.option_add('*TCombobox*Listbox.background', '#2b2b38')
        self.option_add('*TCombobox*Listbox.foreground', TXT)

    def _build(self):
        top = ttk.Frame(self, style='Top.TFrame', padding=(10, 8))
        top.pack(fill='x')
        ttk.Button(top, text="📂  Abrir MIDI…", command=self.ask_open).pack(side='left')
        self.lbl_file = ttk.Label(top, text="(ningún archivo)", style='Top.TLabel', width=22)
        self.lbl_file.pack(side='left', padx=(8, 16))
        ttk.Label(top, text="Nombre C:", style='Top.TLabel').pack(side='left')
        e = ttk.Entry(top, textvariable=self.song, width=14)
        e.pack(side='left', padx=(4, 12))
        ttk.Label(top, text="BPM:", style='Top.TLabel').pack(side='left')
        ttk.Spinbox(top, from_=20, to=300, textvariable=self.bpm, width=5, command=self.schedule).pack(side='left', padx=(4, 12))
        ttk.Label(top, text="Cuantizar:", style='Top.TLabel').pack(side='left')
        cb = ttk.Combobox(top, textvariable=self.grid_name, values=[g[0] for g in GRIDS], width=18, state='readonly')
        cb.pack(side='left', padx=(4, 12))
        cb.bind('<<ComboboxSelected>>', lambda e: self.schedule())
        ttk.Checkbutton(top, text="Un .h por canal", variable=self.split, style='Top.TCheckbutton',
                        command=self.schedule).pack(side='left', padx=(0, 12))
        ttk.Button(top, text="💳  Guardar en microSD", style='Accent.TButton', command=self.save_sd).pack(side='right')
        ttk.Button(top, text="💾  Exportar .h…", command=self.export).pack(side='right', padx=(0, 8))
        for v in (self.song, self.bpm):
            v.trace_add('write', lambda *a: self.schedule())

        ttk.Label(self, textvariable=self.status, style='Top.TLabel', padding=(10, 4)).pack(side='bottom', fill='x')
        pw = ttk.PanedWindow(self, orient='vertical')
        pw.pack(fill='both', expand=True, padx=10, pady=(0, 6))

        # ---- tabla de pistas MIDI ----
        tf = ttk.Frame(pw, padding=8)
        pw.add(tf, weight=1)
        ttk.Label(tf, text="PISTAS DEL MIDI  ·  Canal = grupo de pistas (cada una conserva su instrumento)  ·  mismo nombre de salida = se fusionan en una sola pista",
                  style='H.TLabel').pack(anchor='w', pady=(0, 6))
        holder = ttk.Frame(tf)
        holder.pack(fill='both', expand=True)
        self.rows_canvas = tk.Canvas(holder, bg=PANEL, highlightthickness=0, height=140)
        sb = ttk.Scrollbar(holder, orient='vertical', command=self.rows_canvas.yview)
        self.rows_frame = ttk.Frame(self.rows_canvas)
        self.rows_frame.bind('<Configure>', lambda e: self.rows_canvas.configure(scrollregion=self.rows_canvas.bbox('all')))
        self.rows_canvas.create_window((0, 0), window=self.rows_frame, anchor='nw')
        self.rows_canvas.configure(yscrollcommand=sb.set)
        self.rows_canvas.pack(side='left', fill='both', expand=True)
        sb.pack(side='right', fill='y')

        # ---- pestañas ----
        nb = ttk.Notebook(pw)
        pw.add(nb, weight=3)
        self.nb = nb

        # piano roll
        pr = ttk.Frame(nb)
        nb.add(pr, text="🎹  Piano roll (lo que se guarda)")
        bar = ttk.Frame(pr, padding=(6, 4))
        bar.pack(fill='x')
        ttk.Label(bar, text="Zoom").pack(side='left')
        ttk.Scale(bar, from_=0.05, to=3.0, variable=self.zoom, command=lambda v: self.draw_roll(),
                  length=220).pack(side='left', padx=6)
        self.lbl_roll = ttk.Label(bar, text="")
        self.lbl_roll.pack(side='left', padx=12)
        body = ttk.Frame(pr)
        body.pack(fill='both', expand=True)
        self.lab_canvas = tk.Canvas(body, bg=BG, width=LABEL_W, highlightthickness=0)
        self.roll = tk.Canvas(body, bg=BG, highlightthickness=0)
        xs = ttk.Scrollbar(pr, orient='horizontal', command=self.roll.xview)
        ys = ttk.Scrollbar(body, orient='vertical', command=self._yview)
        self.roll.configure(xscrollcommand=xs.set, yscrollcommand=ys.set)
        self.lab_canvas.configure(yscrollcommand=ys.set)
        self.lab_canvas.pack(side='left', fill='y')
        self.roll.pack(side='left', fill='both', expand=True)
        ys.pack(side='right', fill='y')
        xs.pack(fill='x')
        self.roll.bind('<Motion>', self.on_hover)
        self.roll.bind('<Shift-MouseWheel>', lambda e: self.roll.xview_scroll(-1 * (e.delta // 120), 'units'))
        self.roll.bind('<MouseWheel>', lambda e: self._yview('scroll', -1 * (e.delta // 120), 'units'))
        self.roll.bind('<Control-MouseWheel>', self._wheel_zoom)
        self.roll.bind('<Configure>', lambda e: self.draw_roll())

        # código C
        cf = ttk.Frame(nb, padding=6)
        nb.add(cf, text="📄  Código C")
        cbar = ttk.Frame(cf)
        cbar.pack(fill='x', pady=(0, 4))
        ttk.Label(cbar, text="Archivo:").pack(side='left')
        self.code_file = tk.StringVar()
        self.code_cb = ttk.Combobox(cbar, textvariable=self.code_file, state='readonly', width=40)
        self.code_cb.pack(side='left', padx=6)
        self.code_cb.bind('<<ComboboxSelected>>', lambda e: self.show_code())
        ttk.Button(cbar, text="Copiar", command=self.copy_code).pack(side='left')
        self.code = tk.Text(cf, bg='#12121a', fg='#cfe3ff', insertbackground='white', font=('Consolas', 10),
                            wrap='none', relief='flat')
        cys = ttk.Scrollbar(cf, orient='vertical', command=self.code.yview)
        cxs = ttk.Scrollbar(cf, orient='horizontal', command=self.code.xview)
        self.code.configure(yscrollcommand=cys.set, xscrollcommand=cxs.set)
        cxs.pack(side='bottom', fill='x')
        cys.pack(side='right', fill='y')
        self.code.pack(fill='both', expand=True)

        # memoria
        mf = ttk.Frame(nb, padding=8)
        nb.add(mf, text="📊  Memoria")
        cols = ('pista', 'inst', 'eventos', 'bytes', 'poli', 'dur')
        self.mem = ttk.Treeview(mf, columns=cols, show='tree headings', height=12)
        self.mem.heading('#0', text='Canal')
        self.mem.column('#0', width=110)
        for c, t, w in zip(cols, ('Pista de salida', 'Instrumento', 'Eventos (notas+silencios)', 'Bytes en flash',
                                  'Polifonía máx.', 'Duración'), (180, 110, 180, 120, 110, 100)):
            self.mem.heading(c, text=t)
            self.mem.column(c, width=w, anchor='center')
        self.mem.pack(fill='x')
        self.mem_lbl = ttk.Label(mf, text="", justify='left', font=('Segoe UI', 10))
        self.mem_lbl.pack(anchor='w', pady=10)
        ttk.Label(mf, text="Cada evento ocupa 6 bytes: {nota, duración (2 B), velocidad, flags}.  "
                           "Polifonía = cuántas voces suenan a la vez en esa pista (el sintetizador tiene 12 en total).",
                  foreground='#8a8aa0').pack(anchor='w')


    def _yview(self, *args):
        self.roll.yview(*args)
        self.lab_canvas.yview(*args)

    def _wheel_zoom(self, e):
        z = self.zoom.get() * (1.15 if e.delta > 0 else 1 / 1.15)
        self.zoom.set(min(3.0, max(0.05, z)))
        self.draw_roll()

    # --------------------------------------------------------------- MIDI
    def ask_open(self):
        p = filedialog.askopenfilename(title="Abrir MIDI", filetypes=[("MIDI", "*.mid *.midi"), ("Todos", "*.*")])
        if p:
            self.open_midi(p)

    def open_midi(self, path):
        try:
            mid, tracks, names, programs, tempos = mc.load_midi(path)
        except Exception as e:
            messagebox.showerror("No se pudo abrir", str(e))
            return
        self.midi_path, self.tpb, self.tempos = path, mid.ticks_per_beat, tempos
        self.lbl_file.configure(text=os.path.basename(path))
        self.song.set(mc.c_ident(os.path.splitext(os.path.basename(path))[0]))
        self.bpm.set(int(round(mido.tempo2bpm(tempos[0][1]))) if tempos else 120)
        for w in self.rows_frame.winfo_children():
            w.destroy()
        self.rows = []
        heads = ("Usar", "Pista", "Nombre en el MIDI", "Canal MIDI", "Notas", "Rango", "Canal",
                 "→ Salida (nombre C)", "Instrumento", "Volumen", "")
        for c, h in enumerate(heads):
            ttk.Label(self.rows_frame, text=h, style='H.TLabel').grid(row=0, column=c, sticky='w', padx=6, pady=(0, 4))
        r = 1
        for i, (tr, nm) in enumerate(zip(tracks, names)):
            if not tr:
                continue
            row = Row(i, nm, tr, mc.guess_instrument(tr, programs[i]))
            self.rows.append(row)
            chs = ",".join(str(c) for c in sorted({n[4] + 1 for n in tr}))
            lo, hi = min(n[2] for n in tr), max(n[2] for n in tr)
            ttk.Checkbutton(self.rows_frame, variable=row.use, command=self.schedule).grid(row=r, column=0, padx=6)
            ttk.Label(self.rows_frame, text=f"t{i}").grid(row=r, column=1, sticky='w', padx=6)
            ttk.Label(self.rows_frame, text=nm[:28]).grid(row=r, column=2, sticky='w', padx=6)
            ttk.Label(self.rows_frame, text=chs).grid(row=r, column=3, sticky='w', padx=6)
            ttk.Label(self.rows_frame, text=str(len(tr))).grid(row=r, column=4, sticky='w', padx=6)
            ttk.Label(self.rows_frame, text=f"{mc.note_name(lo, 0)} – {mc.note_name(hi, 0)}").grid(row=r, column=5, sticky='w', padx=6)
            ttk.Spinbox(self.rows_frame, from_=1, to=8, textvariable=row.canal, width=3, state='readonly',
                        command=self.schedule).grid(row=r, column=6, sticky='w', padx=6)
            ttk.Entry(self.rows_frame, textvariable=row.out, width=16).grid(row=r, column=7, sticky='w', padx=6, pady=2)
            ttk.Combobox(self.rows_frame, textvariable=row.inst, values=mc.INSTRUMENTS, width=9,
                         state='readonly').grid(row=r, column=8, sticky='w', padx=6)
            sp = ttk.Frame(self.rows_frame)
            sp.grid(row=r, column=9, sticky='w', padx=6)
            ttk.Scale(sp, from_=0, to=100, variable=row.vol, length=110,
                      command=lambda v, rw=row: (rw.vol.set(int(float(v))), self.schedule())).pack(side='left')
            ttk.Label(sp, textvariable=row.vol, width=4).pack(side='left', padx=4)
            for v in (row.out, row.inst, row.canal):
                v.trace_add('write', lambda *a: self.schedule())
            r += 1
        if len({t for _, t in tempos}) > 1:
            self.status.set("Aviso: el MIDI cambia de tempo; se usa el primero. Puedes ajustar el BPM arriba.")
        self.schedule(10)

    # ----------------------------------------------------------- convertir
    def schedule(self, ms=250):
        if self._pending:
            self.after_cancel(self._pending)
        self._pending = self.after(ms, self.regenerate)

    def build_salidas(self):
        groups, order = {}, []
        for row in self.rows:
            if not row.use.get():
                continue
            name = mc.c_ident(row.out.get()) or f"t{row.idx}"
            if name not in groups:
                groups[name] = [[], row.inst.get(), row.vol.get() / 100.0, [], int(row.canal.get())]
                order.append(name)
            groups[name][0] += row.notas
            groups[name][3].append(f"t{row.idx}")
        return ([(n, groups[n][0], groups[n][1], groups[n][2], groups[n][4]) for n in order],
                {n: groups[n][3] for n in order})

    def regenerate(self):
        self._pending = None
        if not self.midi_path:
            return
        salidas, self.sources = self.build_salidas()
        if not salidas:
            self.files, self.stats = [], []
            self.status.set("No hay pistas seleccionadas.")
            self.draw_roll(); self.fill_mem(); self.fill_code()
            return
        song = mc.c_ident(self.song.get()) or "cancion"
        try:
            bpm = max(1, int(self.bpm.get()))
        except (tk.TclError, ValueError):
            return
        grid = dict(GRIDS)[self.grid_name.get()]
        self.files, self.stats = mc.generate_files(os.path.basename(self.midi_path), self.tpb, salidas, song,
                                                   bpm, grid, self.split.get(), f"song_{song}.h")
        self.cur_bpm = bpm
        self.draw_roll(); self.fill_mem(); self.fill_code()
        total = sum(s['bytes'] for s in self.stats)
        self.status.set(f"{len(self.stats)} pistas de salida · {sum(s['eventos'] for s in self.stats)} eventos · "
                        f"{total:,} bytes de flash · {bpm} BPM")

    # ---------------------------------------------------------- piano roll
    def draw_roll(self):
        c, lc = self.roll, self.lab_canvas
        c.delete('all'); lc.delete('all')
        self.note_info = {}
        if not self.stats:
            c.create_text(20, 20, anchor='nw', fill='#6a6a80', text="Sin pistas para mostrar.", font=('Segoe UI', 12))
            return
        z = self.zoom.get()
        # decodificar cada arreglo como lo hace el STM32: t avanza salvo en NOTE_CHORD
        tracks, end = [], 0
        for s in self.stats:
            t, notes = 0, []
            for n, d, v, ch in s['seq']:
                if n != 'REST':
                    notes.append((t, d, n, v, ch))
                    end = max(end, t + d)
                t += 0 if ch else d
            end = max(end, t)
            tracks.append(notes)
        width = int(end * z) + 80
        height = RULER_H + BAND_H * len(self.stats)
        c.configure(scrollregion=(0, 0, width, height))
        lc.configure(scrollregion=(0, 0, LABEL_W, height))
        # regla y compases (4/4)
        bar = mc.PPQ * 4
        c.create_rectangle(0, 0, width, RULER_H, fill='#20202b', outline='')
        step = 1 if bar * z >= 40 else (4 if bar * z >= 10 else 16)
        for b in range(0, end // bar + 2):
            x = b * bar * z
            c.create_line(x, RULER_H, x, height, fill=BARC if b % step == 0 else GRIDC)
            if b % step == 0:
                c.create_text(x + 3, RULER_H / 2, anchor='w', text=str(b + 1), fill='#8a8aa0', font=('Segoe UI', 8))
            if bar * z >= 60:
                for q in range(1, 4):
                    xq = x + q * mc.PPQ * z
                    c.create_line(xq, RULER_H, xq, height, fill='#202029')
        lc.create_rectangle(0, 0, LABEL_W, RULER_H, fill='#20202b', outline='')
        lc.create_text(8, RULER_H / 2, anchor='w', text="compás →", fill='#8a8aa0', font=('Segoe UI', 8))
        # bandas
        for k, (s, notes) in enumerate(zip(self.stats, tracks)):
            y0 = RULER_H + k * BAND_H
            col = COLORS[(s['canal'] - 1) % len(COLORS)]
            c.create_rectangle(0, y0, width, y0 + BAND_H, fill='#1a1a22' if k % 2 else BG, outline='')
            c.create_line(0, y0, width, y0, fill=BARC)
            lc.create_rectangle(0, y0, LABEL_W, y0 + BAND_H, fill='#1d1d27', outline=BARC)
            lc.create_rectangle(0, y0, 5, y0 + BAND_H, fill=col, outline='')
            lc.create_text(14, y0 + 16, anchor='w', text=s['nombre'][:17], fill='white', font=('Segoe UI', 11, 'bold'))
            lc.create_text(14, y0 + 35, anchor='w', text=f"CANAL {s['canal']}", fill=col, font=('Segoe UI', 9, 'bold'))
            lc.create_text(14, y0 + 53, anchor='w', text=f"{s['inst']} · vol {int(s['vol'] * 100)}", fill=TXT, font=('Segoe UI', 9))
            lc.create_text(14, y0 + 70, anchor='w', text=f"de {', '.join(self.sources.get(s['nombre'], []))}", fill='#8a8aa0', font=('Segoe UI', 9))
            lc.create_text(14, y0 + 87, anchor='w', text=f"{s['eventos']} eventos · {s['bytes']} B", fill='#8a8aa0', font=('Segoe UI', 9))
            lc.create_text(14, y0 + 104, anchor='w', text=f"polifonía {s['poli']}", fill='#8a8aa0', font=('Segoe UI', 9))
            if not notes:
                continue
            lo, hi = min(n[2] for n in notes), max(n[2] for n in notes)
            span = max(hi - lo + 1, 12)
            pad = (span - (hi - lo + 1)) / 2
            nh = max(2.0, (BAND_H - 12) / span)
            for (t, d, n, v, ch) in notes:
                x0, x1 = t * z, max(t * z + 2, (t + d) * z - 1)
                yy = y0 + 6 + (span - 1 - (n - lo + pad)) * (BAND_H - 12) / span
                fill = hex_mix(col, 0.45 + 0.55 * min(v, 127) / 127)
                it = c.create_rectangle(x0, yy, x1, yy + nh, fill=fill, outline=col if d * z > 6 else '')
                self.note_info[it] = (s, t, d, n, v, ch)
        self.lbl_roll.configure(text=f"{end / bar:.1f} compases · {end / mc.PPQ * 60 / self.cur_bpm:.1f} s"
                                     "   ·   Ctrl+rueda = zoom, Shift+rueda = mover")

    def on_hover(self, e):
        x, y = self.roll.canvasx(e.x), self.roll.canvasy(e.y)
        for it in reversed(self.roll.find_overlapping(x, y, x + 1, y + 1)):
            if it in self.note_info:
                s, t, d, n, v, ch = self.note_info[it]
                bar, beat = divmod(t, mc.PPQ * 4)
                if s['drums']:
                    nombre = f"{n} ({DRUM_NAMES.get(n, 'percusión')})"
                else:
                    nombre = mc.note_name(n, False)
                c_txt = f"{{{mc.note_name(n, s['drums'])}, {mc.dur_text(d)}, {v}{', NOTE_CHORD' if ch else ''}}}"
                self.status.set(f"{s['nombre']}: {nombre} · compás {bar + 1}, tiempo {beat / mc.PPQ + 1:.2f} · "
                                f"dura {mc.dur_text(d)} ({d * 60000 // (mc.PPQ * self.cur_bpm)} ms) · vel {v}   →   {c_txt}")
                return

    # ------------------------------------------------------- código / memoria
    def fill_code(self):
        names = [f for f, _ in self.files]
        self.code_cb.configure(values=names)
        if self.code_file.get() not in names:
            self.code_file.set(names[-1] if names else "")
        self.show_code()

    def show_code(self):
        txt = dict(self.files).get(self.code_file.get(), "")
        self.code.configure(state='normal')
        self.code.delete('1.0', 'end')
        self.code.insert('1.0', txt)
        self.code.configure(state='disabled')

    def copy_code(self):
        self.clipboard_clear()
        self.clipboard_append(dict(self.files).get(self.code_file.get(), ""))
        self.status.set(f"{self.code_file.get()} copiado al portapapeles.")

    def fill_mem(self):
        self.mem.delete(*self.mem.get_children())
        total, poli = 0, 0
        padres = {}
        for k, s in enumerate(self.stats):
            c = s['canal']
            if c not in padres:
                grupo = [x for x in self.stats if x['canal'] == c]
                padres[c] = self.mem.insert('', 'end', text=f"Canal {c}", open=True, values=(
                    f"{len(grupo)} pista(s)", "", sum(x['eventos'] for x in grupo),
                    f"{sum(x['bytes'] for x in grupo):,}", sum(x['poli'] for x in grupo), ""))
            ticks = 0
            for n, d, v, ch in s['seq']:
                ticks += 0 if ch else d
            secs = ticks / mc.PPQ * 60 / max(1, getattr(self, 'cur_bpm', 120))
            self.mem.insert(padres[c], 'end', text=f"  pista {k}", values=(s['nombre'], s['inst'], s['eventos'],
                                               f"{s['bytes']:,}", s['poli'], f"{int(secs // 60)}:{int(secs % 60):02d}"))
            total += s['bytes']
            poli += s['poli']
        aviso = ""
        if poli > MAX_VOICES:
            aviso = f"\n⚠ Suma de polifonías = {poli} > {MAX_VOICES} voces: en los pasajes densos se cortarán notas (se roban las más viejas)."
        if len(self.stats) > 8:
            aviso += "\n⚠ Más de 8 pistas de salida (MUSIC_MAX_TRACKS = 8): combina algunas."
        self.mem_lbl.configure(text=f"Total: {total:,} bytes = {total / 1024:.1f} KB  "
                                    f"({100 * total / FLASH:.1f} % de los 512 KB de flash){aviso}")

    # -------------------------------------------------------------- exportar
    def export(self):
        if not self.files:
            messagebox.showinfo("Nada que exportar", "Abre un MIDI y selecciona al menos una pista.")
            return
        d = filedialog.askdirectory(title="Carpeta destino (p. ej. Core/Inc de tu proyecto)")
        if not d:
            return
        existentes = [f for f, _ in self.files if os.path.exists(os.path.join(d, f))]
        if existentes and not messagebox.askyesno("Sobrescribir", "Ya existen:\n  " + "\n  ".join(existentes) +
                                                   "\n\n¿Reemplazarlos?"):
            return
        for f, txt in self.files:
            with open(os.path.join(d, f), 'w', encoding='utf-8') as fh:
                fh.write(txt)
        messagebox.showinfo("Listo", f"Escribí {len(self.files)} archivo(s) en:\n{d}")
        self.status.set(f"Exportado a {d}")

    def save_sd(self):
        """Guarda la canción abierta en la biblioteca y escribe todas las canciones a la microSD (.ghs)."""
        import registrar_canciones as rc
        bib = rc.BIBLIOTECA
        os.makedirs(bib, exist_ok=True)
        song = None
        if self.files:                               # hay un MIDI abierto: va a la biblioteca
            song = mc.c_ident(self.song.get()) or "cancion"
            if len(song) > 28 and not messagebox.askyesno(
                    "Nombre largo", f"El nombre C es '{song}'.\n\nMejor uno corto (p. ej. 'animals'): escríbelo en "
                                    "\"Nombre C\" arriba.\n\n¿Guardarla con este nombre de todos modos?"):
                return
            existentes = [f for f, _ in self.files if os.path.exists(os.path.join(bib, f))]
            if existentes and not messagebox.askyesno("Ya existe", f"'{song}' ya está en la biblioteca.\n\n¿Reemplazarla?"):
                return
            for f, txt in self.files:
                with open(os.path.join(bib, f), 'w', encoding='utf-8', newline='') as fh:
                    fh.write(txt.replace('\r\n', '\n').replace('\n', '\r\n'))
        d = filedialog.askdirectory(title="Unidad de la microSD (p. ej. E:\\)", initialdir=getattr(self, 'sd_dir', None))
        if not d:
            if song:
                self.status.set(f"'{song}' guardada en la biblioteca (falta escribirla a la microSD)")
            return
        self.sd_dir = d
        try:
            hechas, avisos = rc.exportar_sd(bib, d)
        except Exception as e:                       # noqa: BLE001 - mostrar cualquier error al usuario
            messagebox.showerror("Error", str(e))
            return
        texto = "\n".join(f"  {i + 1:2}. {t}  ({n} notas)" for i, (_, t, n, _) in enumerate(hechas))
        extra = ("\n\n⚠ No caben en la RAM del STM (se van a rechazar):\n  " + "\n  ".join(avisos)) if avisos else ""
        if len(hechas) > rc.MAX_CANCIONES:
            extra += f"\n\n⚠ El menú muestra solo {rc.MAX_CANCIONES} canciones."
        messagebox.showinfo("Listo", (f"'{song}' agregada.\n\n" if song else "") +
                            f"Escribí {len(hechas)} canciones en:\n{os.path.join(d, 'CANCIONES')}\n\n{texto}{extra}\n\n"
                            "Expulsa la tarjeta con seguridad antes de sacarla.")
        self.status.set(f"microSD: {len(hechas)} canciones en {d}")

if __name__ == '__main__':
    App(sys.argv[1] if len(sys.argv) > 1 else None).mainloop()
