/*
 * beats.c
 * Notas del juego a partir de las pistas de la canción. Ver beats.h.
 *
 * Cómo se decide cada nota del juego:
 *   1. Se juntan todas las notas MIDI de las pistas del jugador que empiezan en el mismo tick (= un "evento").
 *   2. Carril: la nota más aguda del evento. Graves -> verde ... agudas -> azul, con los cortes puestos
 *      para que cada carril reciba ~1/4 de las notas de ese jugador.
 *      Si es batería, el carril lo da el tambor: bombo verde, caja rojo, hi-hat amarillo, toms/platillos azul.
 *   3. Acorde: si el evento trae BEATS_ACORDE_NOTAS o más notas, se agrega el carril de la más grave
 *      (si es distinto) -> hay que presionar 2 botones.
 *   4. Los eventos a menos de BEATS_SEPARACION_MS del anterior se descartan (si no, es imposible de tocar).
 *   5. Si dura BEATS_LARGA_MIN_MS o más, es nota larga; su cola se corta antes de la siguiente nota.
 * Todo se calcula leyendo las pistas en flash: no hay que regenerar las canciones.
 */
#include "beats.h"
#include "gh_protocolo.h"
#include "enlace_spi.h"
#include "juez.h"
#include "music.h"
#include "synth.h"

/* Batería (números General MIDI): cada tipo de tambor tiene su carril */
static uint8_t Carril_Tambor(uint8_t n) {
	switch (n) {
		case 35: case 36:                               return 0;   /* bombo        -> verde    */
		case 37: case 38: case 39: case 40:             return 1;   /* caja / clap  -> rojo     */
		case 42: case 44: case 46:                      return 2;   /* hi-hat       -> amarillo */
		default:                                        return 3;   /* toms, platillos -> azul  */
	}
}

typedef struct {
	const Note *notas;
	uint16_t    largo;
	uint16_t    i;        /* siguiente nota a leer */
	uint32_t    tick;     /* tick en que empieza la nota i */
	uint8_t     bateria;  /* 1 = pista de batería (las notas son tambores, no tonos) */
} Cursor;

typedef struct {          /* notas MIDI que empiezan juntas */
	uint32_t tick;
	uint32_t dur;         /* la más larga */
	uint8_t  alta, baja, cuantas;   /* notas con tono (melodía, acordes, bajo) */
	uint8_t  tambores;              /* carriles de batería que suenan (bits) */
} Evento;

typedef struct {          /* nota del juego */
	uint32_t tick;
	uint32_t largo;       /* ticks; 0 = normal */
	uint8_t  carriles;
} Gema;

typedef struct {
	Cursor  cur[BEATS_MAX_PISTAS];
	uint8_t n;
	uint8_t corte[3];     /* notas MIDI donde empiezan rojo, amarillo y azul */
	Evento  prox;  uint8_t hay_prox;   /* siguiente evento ya leído (para saber dónde cortar las colas) */
	Gema    gema;  uint8_t hay_gema;   /* siguiente nota lista para mandar */
} Jugador;

static Jugador  jug[2];
static const SongEntry *entrada = 0;
static uint8_t  cancion = 0;
static uint8_t  activo = 0;
static uint32_t tick_previo = 0;
static uint32_t bpm = 120;
static uint32_t sep_ticks, larga_min_ticks, anticipa_ticks;

volatile uint32_t beats_enviadas[2] = { 0, 0 };
volatile uint32_t beats_tarde = 0;

/* ---------- Conversión de tiempo ---------- */
static uint32_t Ms_A_Ticks(uint32_t ms)  { return (ms * bpm * MUSIC_PPQ + 59999u) / 60000u; }
static uint32_t Ticks_A_Ms(uint32_t tk)  { return (uint32_t)(((uint64_t)tk * 60000u) / (bpm * MUSIC_PPQ)); }

/* ---------- Lectura de las pistas ---------- */
static uint8_t Leer_Evento(Jugador *J, Evento *ev) {
	for (;;) {
		uint32_t t = 0xFFFFFFFFu;
		for (uint8_t k = 0; k < J->n; k++)
			if (J->cur[k].i < J->cur[k].largo && J->cur[k].tick < t) t = J->cur[k].tick;
		if (t == 0xFFFFFFFFu) return 0;                    /* se acabaron las pistas */

		ev->tick = t; ev->dur = 0; ev->alta = 0; ev->baja = 255; ev->cuantas = 0; ev->tambores = 0;
		for (uint8_t k = 0; k < J->n; k++) {
			Cursor *c = &J->cur[k];
			while (c->i < c->largo && c->tick == t) {
				const Note *nt = &c->notas[c->i++];
				if (nt->note != REST && c->bateria) {
					ev->tambores |= (uint8_t)(1u << Carril_Tambor(nt->note));
				} else if (nt->note != REST) {
					if (nt->note > ev->alta) ev->alta = nt->note;
					if (nt->note < ev->baja) ev->baja = nt->note;
					if (nt->dur > ev->dur)   ev->dur  = nt->dur;
					ev->cuantas++;
				}
				if (!(nt->flags & NOTE_CHORD)) c->tick += nt->dur;
			}
		}
		if (ev->cuantas || ev->tambores) return 1;         /* si solo eran silencios, sigue buscando */
	}
}

static uint8_t Carril(const Jugador *J, uint8_t nota) {
	uint8_t c = 0;
	while (c < 3 && nota >= J->corte[c]) c++;
	return c;                                             /* 0..3 */
}

/* Arma la siguiente nota del juego del jugador. Devuelve 0 si ya no hay. */
static uint8_t Siguiente_Gema(Jugador *J, Gema *g) {
	if (!J->hay_prox) return 0;
	Evento ev = J->prox;

	g->tick = ev.tick;
	g->largo = ev.dur;
	if (ev.cuantas) {                                     /* hay notas con tono: mandan ellas */
		g->carriles = (uint8_t)(1u << Carril(J, ev.alta));
		if (ev.cuantas >= BEATS_ACORDE_NOTAS) g->carriles |= (uint8_t)(1u << Carril(J, ev.baja));
	} else {                                              /* solo batería: un carril, por importancia */
		static const uint8_t prioridad[4] = { 1, 0, 3, 2 };   /* caja, bombo, toms/platillos, hi-hat */
		for (uint8_t k = 0; k < 4; k++)
			if (ev.tambores & (1u << prioridad[k])) { g->carriles = (uint8_t)(1u << prioridad[k]); break; }
		g->largo = 0;                                     /* un golpe de batería nunca es nota larga */
	}

	/* descarta lo que cae demasiado cerca; lo que queda en prox es la siguiente nota real */
	do { J->hay_prox = Leer_Evento(J, &J->prox); }
	while (J->hay_prox && J->prox.tick < ev.tick + sep_ticks);

	/* la cola no puede tapar la siguiente nota */
	if (J->hay_prox) {
		uint32_t max = J->prox.tick - ev.tick - sep_ticks;
		if (g->largo > max) g->largo = max;
	}
	if (g->largo < larga_min_ticks) g->largo = 0;
	return 1;
}

static void Cursores_Al_Inicio(Jugador *J) {
	for (uint8_t k = 0; k < J->n; k++) { J->cur[k].i = 0; J->cur[k].tick = 0; }
}

/* Cortes entre carriles: cada carril recibe más o menos 1/4 de las notas del jugador
 * (por eso se cuentan las notas: si se repartiera el rango a partes iguales, casi todo caería en 1 o 2 carriles) */
static void Calcular_Cortes(Jugador *J) {
	uint16_t hist[128] = { 0 };
	uint32_t total = 0;
	Evento ev;
	while (Leer_Evento(J, &ev)) {
		if (!ev.cuantas) continue;                        /* la batería no usa cortes */
		if (hist[ev.alta & 0x7F] < 0xFFFF) hist[ev.alta & 0x7F]++;
		total++;
	}
	Cursores_Al_Inicio(J);

	uint32_t acum = 0;
	uint8_t c = 0, nota = 0;
	for (uint32_t n = 0; n < 128 && c < 3; n++) {
		acum += hist[n];
		/* el corte c va después de la nota donde se pasa la fracción (c+1)/4 */
		while (c < 3 && acum * 4u >= total * (c + 1u)) J->corte[c++] = (uint8_t)(n + 1);
		if (hist[n]) nota = (uint8_t)n;
	}
	while (c < 3) J->corte[c++] = (uint8_t)(nota + 1);
	/* cortes repetidos (una nota domina): se separan para que no queden carriles vacíos seguidos */
	for (c = 1; c < 3; c++) if (J->corte[c] <= J->corte[c - 1]) J->corte[c] = (uint8_t)(J->corte[c - 1] + 1);
}

static void Jugador_Preparar(Jugador *J, const Song *s, const uint8_t *pistas, uint8_t n) {
	J->n = 0; J->hay_prox = 0; J->hay_gema = 0;
	if (!pistas) return;
	for (uint8_t k = 0; k < n && J->n < BEATS_MAX_PISTAS; k++) {
		if (pistas[k] >= s->num_tracks) continue;
		const TrackDef *td = &s->tracks[pistas[k]];
		Cursor *c = &J->cur[J->n++];
		c->notas = td->notes; c->largo = td->length;
		c->bateria = (td->instrument == INST_DRUMS);
	}
	Cursores_Al_Inicio(J);
	Calcular_Cortes(J);
	J->hay_prox = Leer_Evento(J, &J->prox);
	J->hay_gema = Siguiente_Gema(J, &J->gema);
}

static void Reiniciar(uint8_t nueva) {
	const Song *s = entrada->song;
	Jugador_Preparar(&jug[0], s, entrada->p1_tracks, entrada->p1_n);
	Jugador_Preparar(&jug[1], s, entrada->p2_tracks, entrada->p2_n);
	tick_previo = 0;
	Juez_Iniciar(entrada, nueva);                         /* borra las notas que el juez llevaba */
	Enlace_Enviar(ENLACE_AMBOS, GH_PKT_CANCION, cancion, (uint8_t)bpm, (uint8_t)(bpm >> 8), 0, 0);
}

void Beats_Iniciar(const SongEntry *e, uint8_t indice) {
	activo = 0;
	entrada = e;
	cancion = indice;
	if (!e) { Beats_Detener(); return; }
	bpm             = e->song->bpm ? e->song->bpm : 120;
	sep_ticks       = Ms_A_Ticks(BEATS_SEPARACION_MS);
	larga_min_ticks = Ms_A_Ticks(BEATS_LARGA_MIN_MS);
	anticipa_ticks  = Ms_A_Ticks(GH_ANTICIPACION_MS);
	Reiniciar(1);
	activo = 1;
}

void Beats_Detener(void) {
	activo = 0;
	Juez_Iniciar(0, 1);
	Enlace_Enviar(ENLACE_AMBOS, GH_PKT_ALTO, 0, 0, 0, 0, 0);
}

void Beats_Actualizar(void) {
	if (!activo || !Music_IsPlaying()) return;

	uint32_t ahora = Music_GetTick();
	if (ahora < tick_previo) Reiniciar(0);                 /* la canción dio la vuelta (repetir) */
	tick_previo = ahora;

	for (uint8_t j = 0; j < 2; j++) {
		Jugador *J = &jug[j];
		while (J->hay_gema && J->gema.tick <= ahora + anticipa_ticks) {
			if (J->gema.tick >= ahora) {
				uint32_t faltan = Ticks_A_Ms(J->gema.tick - ahora) + BEATS_LATENCIA_MS;
				uint32_t largo  = Ticks_A_Ms(J->gema.largo);
				if (faltan > 0xFFFF) faltan = 0xFFFF;
				if (largo  > 0xFFFF) largo  = 0xFFFF;
				uint8_t c = 0;                                /* una nota por botón: el carril más bajo */
				while (c < 3 && !(J->gema.carriles & (1u << c))) c++;
				Juez_NotaNueva(j, c, (uint16_t)faltan, (uint16_t)largo);   /* el juez la registra y la manda */
				beats_enviadas[j]++;
			} else {
				beats_tarde++;                                /* ya pasó: no tiene sentido mandarla */
			}
			J->hay_gema = Siguiente_Gema(J, &J->gema);
		}
	}
}
