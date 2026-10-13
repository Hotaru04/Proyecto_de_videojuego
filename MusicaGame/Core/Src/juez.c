/*
 * juez.c
 * Calificación, puntos, mute y pausa con el reloj de MusicaGame. Ver juez.h.
 *
 * Todas las horas son HAL_GetTick() de este STM: la nota llega al botón en  t = (hora de envío) + faltan_ms,
 * la misma cuenta que hace la pantalla al recibirla. La hora de cada botón es la hora en que llegó su
 * comando por UART (la guarda la interrupción), así que no importa si el while(1) lo procesa un poco después.
 */
#include "juez.h"
#include "controles.h"
#include "enlace_spi.h"
#include "gh_protocolo.h"
#include "music.h"
#include "main.h"
#include "registro.h"
#include "audio.h"

#define JUEZ_REGISTRAR_NOTAS 1   /* 1 = escribir en la bitacora cada nota que se manda (hasta ~5 por segundo) */

static const char *const COLOR[4] = { "VERDE", "ROJO", "AMARILLO", "AZUL" };
static const char *const CALIF[4] = { "FALLO", "JUSTO", "MEDIO", "PERFECTO" };
static const uint16_t    PUNTOS[4] = { 0, JUEZ_PUNTOS_JUSTO, JUEZ_PUNTOS_MEDIO, JUEZ_PUNTOS_PERFECTO };

enum { N_ESPERA = 0, N_SOSTENIDA, N_TERMINADA };

typedef struct {
	uint32_t t;          /* ms en que la nota llega al botón */
	uint32_t t_fin;      /* ms en que termina la cola (= t si es nota normal) */
	uint32_t t_pago;     /* nota larga: hasta qué hora ya se pagaron los puntos por sostenerla */
	uint8_t  id;
	uint8_t  estado;
} JNota;

typedef struct {
	JNota   n[JUEZ_NOTAS_POR_CARRIL];
	uint8_t ini, cant;
} JCarril;

typedef struct {
	JCarril        car[CONTROLES_CARRILES];
	uint8_t        id_sig;
	uint8_t        botones_env, sost_env;   /* lo último que se le mandó a la pantalla */
	const uint8_t *pistas;                  /* pistas de este jugador en la canción (canal 1 o 2) */
	uint8_t        npistas;
} JJugador;

static JJugador jug[2];
static const SongEntry *entrada = 0;

static int8_t   pausa_de = -1;        /* jugador que puso la pausa (0 o 1), -1 = sin pausa */
static uint32_t pausa_t0;             /* cuándo empezó */
static uint8_t  pausa_opc = 0;        /* opción marcada en el menú de pausa (GH_PAUSA_*) */
static uint8_t  pedido = JUEZ_PEDIDO_NADA;
static uint32_t crudo[2], maximo[2];  /* puntos sin escalar y lo máximo que puede sacar cada jugador */

volatile uint32_t juez_aciertos[2] = { 0, 0 }, juez_fallos[2] = { 0, 0 }, juez_puntos[2] = { 0, 0 };
volatile uint8_t  juez_muteado[2] = { 0, 0 };

static JNota *Lugar(JCarril *k, uint8_t i) { return &k->n[(k->ini + i) % JUEZ_NOTAS_POR_CARRIL]; }

static void Evento(uint8_t j, uint8_t c, const JNota *n, uint8_t evt, uint8_t cal) {
	Enlace_Enviar(ENLACE_JUGADOR(j), GH_PKT_EVENTO, GH_ID_CARRIL(n->id, c), evt, cal, 0, 0);
}

/* Suma puntos y le manda el total a la pantalla del jugador (cal = lo que muestra junto a los puntos).
 * El total se escala: crudo / máximo del jugador * JUEZ_PUNTOS_MAXIMOS (ver juez.h). */
static void Sumar(uint8_t j, uint32_t puntos, uint8_t cal) {
	crudo[j] += puntos;
	juez_puntos[j] = maximo[j] ? (uint32_t)(((uint64_t)crudo[j] * JUEZ_PUNTOS_MAXIMOS) / maximo[j]) : crudo[j];
	uint32_t p = juez_puntos[j];
	Enlace_Enviar(ENLACE_JUGADOR(j), GH_PKT_PUNTOS, (uint8_t)p, (uint8_t)(p >> 8), (uint8_t)(p >> 16), (uint8_t)(p >> 24), cal);
}

static void Calificar(uint8_t j, uint8_t cal) { Sumar(j, PUNTOS[cal], cal); }

/* Nota larga: paga lo sostenido hasta la hora 'hasta' (nunca más allá del final de la cola) */
static void Pagar_Sostenida(uint8_t j, JNota *n, uint32_t hasta) {
	if ((int32_t)(hasta - n->t_fin) > 0) hasta = n->t_fin;
	uint32_t pts = 0;
	while ((int32_t)(hasta - n->t_pago) >= JUEZ_SOSTENIDA_CADA_MS) { n->t_pago += JUEZ_SOSTENIDA_CADA_MS; pts += JUEZ_PUNTOS_SOSTENIDA; }
	if (pts) Sumar(j, pts, GH_CAL_SOSTENIDA);
}

/* Mutea / devuelve el volumen original de las pistas del jugador */
static void Mutear(uint8_t j, uint8_t si) {
	JJugador *J = &jug[j];
	if (juez_muteado[j] != si)
		Registro("J%u pista %s", j + 1, si ? "MUTEADA (fallo)" : "suena otra vez (acierto)");
	juez_muteado[j] = si;
	if (!entrada || !J->pistas) return;
	for (uint8_t i = 0; i < J->npistas; i++) {
		uint8_t t = J->pistas[i];
		if (t >= entrada->song->num_tracks) continue;
		Music_SetTrackVolume(t, si ? 0 : entrada->song->tracks[t].volume);
	}
}

void Juez_Iniciar(const SongEntry *e, uint8_t nueva_cancion) {
	entrada = e;
	if (pausa_de >= 0) { pausa_de = -1; Audio_Pausa(0); }
	pedido = JUEZ_PEDIDO_NADA;
	for (uint8_t j = 0; j < 2; j++) {
		JJugador *J = &jug[j];
		for (uint8_t c = 0; c < CONTROLES_CARRILES; c++) { J->car[c].ini = 0; J->car[c].cant = 0; }
		J->id_sig = 0;
		J->botones_env = 0xFF;                  /* fuerza a mandar el estado de botones */
		J->sost_env = 0xFF;
		J->pistas  = e ? (j == 0 ? e->p1_tracks : e->p2_tracks) : 0;
		J->npistas = e ? (j == 0 ? e->p1_n      : e->p2_n)      : 0;
		Mutear(j, 0);
		if (nueva_cancion) {                    /* al repetir la misma canción los puntos se conservan */
			juez_puntos[j] = 0; juez_aciertos[j] = 0; juez_fallos[j] = 0; crudo[j] = 0; maximo[j] = 0;
		}
		if (e) Enlace_Enviar(ENLACE_JUGADOR(j), GH_PKT_PUNTOS, (uint8_t)juez_puntos[j], (uint8_t)(juez_puntos[j] >> 8),
		                     (uint8_t)(juez_puntos[j] >> 16), (uint8_t)(juez_puntos[j] >> 24), 0xFF);
	}
}

uint8_t Juez_EnPausa(void) { return pausa_de >= 0; }

void Juez_Maximo(uint8_t j, uint32_t puntos_base) {
	if (j > 1) return;
	maximo[j] = puntos_base;
	Registro("J%u: maximo %lu puntos base -> se escala a %lu", j + 1, (unsigned long)puntos_base,
	         (unsigned long)JUEZ_PUNTOS_MAXIMOS);
}

uint8_t Juez_Pedido(void) { uint8_t p = pedido; pedido = JUEZ_PEDIDO_NADA; return p; }

void Juez_NotaNueva(uint8_t j, uint8_t c, uint16_t faltan_ms, uint16_t largo_ms) {
	if (j > 1 || c >= CONTROLES_CARRILES) return;
	JJugador *J = &jug[j];
	JCarril *k = &J->car[c];
	if (k->cant >= JUEZ_NOTAS_POR_CARRIL) return;          /* no debería pasar: en pantalla caben ~6 */

	JNota *n = Lugar(k, k->cant++);
	n->id     = (uint8_t)(J->id_sig++ & 0x3F);
	n->t      = HAL_GetTick() + faltan_ms;
	n->t_fin  = n->t + largo_ms;
	n->t_pago = n->t;
	n->estado = N_ESPERA;
#if JUEZ_REGISTRAR_NOTAS
	if (largo_ms) Registro("J%u nota %-8s id %2u  llega en %u ms  larga %u ms", j + 1, COLOR[c], n->id, faltan_ms, largo_ms);
	else          Registro("J%u nota %-8s id %2u  llega en %u ms", j + 1, COLOR[c], n->id, faltan_ms);
#endif
	Enlace_Enviar(ENLACE_JUGADOR(j), GH_PKT_NOTA, GH_ID_CARRIL(n->id, c),
	              (uint8_t)faltan_ms, (uint8_t)(faltan_ms >> 8),
	              (uint8_t)largo_ms,  (uint8_t)(largo_ms >> 8));
}

static void Presionar(uint8_t j, uint8_t c, uint32_t t) {
	JCarril *k = &jug[j].car[c];
	JNota *mejor = 0;
	int32_t mejor_err = 0;
	int32_t prox = INT32_MAX;                           /* para la bitácora: cuánto faltaba para la siguiente */
	for (uint8_t i = 0; i < k->cant; i++) {             /* la nota en espera más cercana a este golpe */
		JNota *n = Lugar(k, i);
		if (n->estado != N_ESPERA) continue;
		int32_t err = (int32_t)(t - JUEZ_AJUSTE_MS - n->t);   /* <0 temprano, >0 tarde */
		int32_t a = err < 0 ? -err : err;
		if (err < 0 && -err < prox) prox = -err;
		if (a <= JUEZ_JUSTO_MS && (!mejor || a < (mejor_err < 0 ? -mejor_err : mejor_err))) { mejor = n; mejor_err = err; }
	}
	if (!mejor) {
		if (prox != INT32_MAX) Registro("J%u %s presionado: la proxima nota llega en %ld ms (muy temprano)", j + 1, COLOR[c], (long)prox);
		else                   Registro("J%u %s presionado: no hay nota en este carril", j + 1, COLOR[c]);
		return;
	}

	int32_t a = mejor_err < 0 ? -mejor_err : mejor_err;
	uint8_t cal = (a <= JUEZ_PERFECTO_MS) ? GH_CAL_PERFECTO : (a <= JUEZ_MEDIO_MS) ? GH_CAL_MEDIO : GH_CAL_JUSTO;
	if (mejor->t_fin != mejor->t) { mejor->estado = N_SOSTENIDA; mejor->t_pago = mejor->t; Evento(j, c, mejor, GH_EVT_SOSTENIDA, cal); }
	else                          { mejor->estado = N_TERMINADA; Evento(j, c, mejor, GH_EVT_ACIERTO,   cal); }
	juez_aciertos[j]++;
	Calificar(j, cal);
	Registro("J%u %s %s%s (%+ld ms)  +%u = %lu", j + 1, COLOR[c], CALIF[cal],
	         (mejor->estado == N_SOSTENIDA) ? " nota larga" : "", (long)mejor_err, PUNTOS[cal], (unsigned long)juez_puntos[j]);
	if (juez_muteado[j]) Mutear(j, 0);                  /* volvió a atinar: vuelve su pista */
}

static void Soltar(uint8_t j, uint8_t c, uint32_t t) {
	JCarril *k = &jug[j].car[c];
	for (uint8_t i = 0; i < k->cant; i++) {
		JNota *n = Lugar(k, i);
		if (n->estado != N_SOSTENIDA) continue;
		Pagar_Sostenida(j, n, t);                       /* se queda con lo que sostuvo */
		n->estado = N_TERMINADA;
		if ((int32_t)(n->t_fin - t) <= JUEZ_MEDIO_MS) {
			Evento(j, c, n, GH_EVT_COMPLETA, 0);            /* soltó justo al final: cuenta como completa */
			Registro("J%u %s nota larga COMPLETA", j + 1, COLOR[c]);
		} else {
			Evento(j, c, n, GH_EVT_SOLTADA, 0);
			Registro("J%u %s nota larga SOLTADA %ld ms antes", j + 1, COLOR[c], (long)(int32_t)(n->t_fin - t));
			Mutear(j, 1);
		}
		return;
	}
}

/* Start: pone la pausa, o la quita si la puso el mismo jugador */
static void Start(uint8_t j, uint32_t t) {
	if (pausa_de < 0) {
		pausa_de = (int8_t)j;
		pausa_t0 = t;
		pausa_opc = GH_PAUSA_CONTINUAR;
		Audio_Pausa(1);
		Enlace_Enviar(ENLACE_AMBOS, GH_PKT_PAUSA, 1, (uint8_t)(j + 1), pausa_opc, 0, 0);
		Registro("PAUSA (la puso J%u; solo J%u la puede quitar)", j + 1, j + 1);
	} else if (pausa_de == (int8_t)j) {
		uint32_t dur = t - pausa_t0;
		for (uint8_t jj = 0; jj < 2; jj++)                /* las notas pendientes se corren lo que duró la pausa */
			for (uint8_t c = 0; c < CONTROLES_CARRILES; c++) {
				JCarril *k = &jug[jj].car[c];
				for (uint8_t i = 0; i < k->cant; i++) { Lugar(k, i)->t += dur; Lugar(k, i)->t_fin += dur; Lugar(k, i)->t_pago += dur; }
			}
		pausa_de = -1;
		Audio_Pausa(0);
		Enlace_Enviar(ENLACE_AMBOS, GH_PKT_PAUSA, 0, (uint8_t)(j + 1), 0, 0, 0);
		Registro("Sigue el juego (pausa de %lu ms)", (unsigned long)dur);
		/* si alguien soltó una nota larga durante la pausa, cuenta como soltada ahora */
		for (uint8_t jj = 0; jj < 2; jj++)
			for (uint8_t c = 0; c < CONTROLES_CARRILES; c++)
				if (!(Controles_Estado(jj) & (1u << c))) Soltar(jj, c, t);
	} else {
		Registro("J%u presiono Start: solo J%u puede quitar la pausa", j + 1, pausa_de + 1);
	}
}

/* Menú de pausa: solo el jugador que la puso. Flechas mueven, A (verde) elige. */
static void Menu_Pausa(uint8_t j, uint8_t carril, uint32_t t) {
	if (j != (uint8_t)pausa_de) return;
	if (carril == CONTROL_ARRIBA || carril == CONTROL_ABAJO) {
		pausa_opc = (uint8_t)((pausa_opc + (carril == CONTROL_ABAJO ? 1 : GH_PAUSA_OPCIONES - 1)) % GH_PAUSA_OPCIONES);
		Enlace_Enviar(ENLACE_AMBOS, GH_PKT_PAUSA, 1, (uint8_t)(j + 1), pausa_opc, 0, 0);
	} else if (carril == 0) {
		if (pausa_opc == GH_PAUSA_CONTINUAR) Start(j, t);
		else                                 { pedido = JUEZ_PEDIDO_SALIR; Registro("J%u: SALIR AL MENU", j + 1); }
	}
}

void Juez_Actualizar(void) {
	/* 1. Botones (primero, con la hora en que llegaron) */
	Control_Evento ev;
	while (Controles_Siguiente(&ev)) {
		if (ev.jugador > 1) continue;
		if (ev.carril == CONTROL_START) { if (ev.presionado) Start(ev.jugador, ev.t_ms); continue; }
		if (pausa_de >= 0) { if (ev.presionado) Menu_Pausa(ev.jugador, ev.carril, ev.t_ms); continue; }
		if (ev.carril >= CONTROLES_CARRILES) continue;  /* flechas: solo para los menús */
		if (ev.presionado) Presionar(ev.jugador, ev.carril, ev.t_ms);
		else               Soltar(ev.jugador, ev.carril, ev.t_ms);
	}

	uint32_t ahora = HAL_GetTick();
	uint8_t en_pausa = (pausa_de >= 0);
	for (uint8_t j = 0; j < 2; j++) {
		JJugador *J = &jug[j];
		uint8_t sost = 0;

		for (uint8_t c = 0; c < CONTROLES_CARRILES; c++) {
			JCarril *k = &J->car[c];
			for (uint8_t i = 0; i < k->cant; i++) {
				JNota *n = Lugar(k, i);
				if (en_pausa) { if (n->estado == N_SOSTENIDA) sost |= (uint8_t)(1u << c); continue; }   /* el tiempo no corre */
				if (n->estado == N_ESPERA && (int32_t)(ahora - JUEZ_AJUSTE_MS - n->t) > JUEZ_JUSTO_MS) {
					n->estado = N_TERMINADA;                /* no la tocó */
					Evento(j, c, n, GH_EVT_FALLO, GH_CAL_FALLO);
					juez_fallos[j]++;
					Calificar(j, GH_CAL_FALLO);
					Registro("J%u %s FALLO (id %u)", j + 1, COLOR[c], n->id);
					Mutear(j, 1);
				} else if (n->estado == N_SOSTENIDA) {
					Pagar_Sostenida(j, n, ahora);
					if ((int32_t)(ahora - n->t_fin) >= 0) {
						n->estado = N_TERMINADA;            /* aguantó toda la cola */
						Evento(j, c, n, GH_EVT_COMPLETA, 0);
						Registro("J%u %s nota larga COMPLETA", j + 1, COLOR[c]);
					} else {
						sost |= (uint8_t)(1u << c);
					}
				}
			}
			while (k->cant && Lugar(k, 0)->estado == N_TERMINADA) {   /* sacar las ya juzgadas */
				k->ini = (uint8_t)((k->ini + 1) % JUEZ_NOTAS_POR_CARRIL);
				k->cant--;
			}
		}

		/* 2. Dibujo de botones en la pantalla del jugador: solo cuando cambia */
		uint8_t b = Controles_Estado(j);
		if (b != J->botones_env || sost != J->sost_env) {
			Enlace_Enviar(ENLACE_JUGADOR(j), GH_PKT_BOTONES, b, sost, 0, 0, 0);
			J->botones_env = b;
			J->sost_env = sost;
		}

		/* 3. Al repetir la canción el motor devuelve los volúmenes: mantener el mute */
		if (juez_muteado[j] && entrada && J->pistas) {
			const uint8_t *vol = Music_TrackVolumes();
			for (uint8_t i = 0; i < J->npistas; i++)
				if (J->pistas[i] < MUSIC_MAX_TRACKS && vol[J->pistas[i]]) Music_SetTrackVolume(J->pistas[i], 0);
		}
	}
}
