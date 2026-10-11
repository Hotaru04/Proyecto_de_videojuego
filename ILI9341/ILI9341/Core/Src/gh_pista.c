/*
 * gh_pista.c
 * Pista con scroll por hardware. Ver gh_pista.h.
 *
 * Coordenadas:
 *   S       = px que ha bajado la pista desde GH_Pista_Iniciar (crece con el tiempo: S = t * velocidad).
 *   "mundo" = posición fija de cada nota en la pista. Una nota que llega al botón a la hora T está en
 *             p = T * velocidad. La parte de arriba de su cabeza se ve en la fila  y = 262 + S - p.
 *   Fila de MEMORIA donde se dibuja algo del mundo w:  (262 - w) mod 262  -> no depende de S,
 *   así que lo que se dibuja una vez se queda quieto en memoria y el scroll lo mueve.
 */
#include "gh_pista.h"
#include "gh_protocolo.h"
#include "gh_notas.h"
#include "gh_sprites.h"
#include "ili9341.h"
#include "main.h"

#define VSA         GH_BOTON_Y                    /* 262 filas que se desplazan */
#define BFA         (GH_PANTALLA_H - VSA)         /* 58 filas fijas abajo (botones) */
#define ALTO        GH_SPR_H                      /* 48 */
#define MITAD       (GH_SPR_H / 2)
#define TILE        GH_COLA_TILE_H                /* 8 */
#define FILA_BYTES  (GH_SPR_W * 2)

enum { N_ESPERA = 0, N_CORTA_OK, N_SOSTENIDA, N_SOLTADA, N_LARGA_OK, N_FALLADA };

typedef struct {
	uint32_t t;          /* ms (HAL_GetTick) en que la cabeza llega al botón */
	int32_t  p;          /* posición en el mundo (px) */
	int16_t  largo;      /* cola en px (0 = nota normal) */
	uint16_t largo_ms;
	uint8_t  estado;
} Nota;

typedef struct {
	Nota    n[GH_NOTAS_POR_CARRIL];
	uint8_t ini, cant;   /* cola circular: n[ini] es la más vieja (la más cerca del botón) */
	uint8_t boton;       /* estado dibujado del botón */
} Carril;

static Carril   car[GH_NUM_CARRILES];
static uint32_t t0;
static int32_t  S;

volatile uint32_t pista_aciertos = 0, pista_fallos = 0, pista_descartadas = 0;

static int32_t Mod(int32_t a, int32_t m) { int32_t r = a % m; return r < 0 ? r + m : r; }
static int32_t Px(uint32_t t_ms) {
	return (int32_t)(((int64_t)(int32_t)(t_ms - t0) * GH_VELOCIDAD_PX_S) / 1000);
}
static Nota *Lugar(Carril *k, uint8_t i) { return &k->n[(k->ini + i) % GH_NOTAS_POR_CARRIL]; }

static const uint8_t *Cola(uint8_t c, uint8_t estado) {
	return (estado == N_SOSTENIDA || estado == N_LARGA_OK) ? gh_hold_tail_on[c] : gh_hold_tail[c];
}

/* ---------- Dibujo: pinta el carril c entre los puntos del mundo wa..wb (solo lo que se ve) ---------- */
static void Pintar(uint8_t c, int32_t wa, int32_t wb) {
	if (wa < S + 1)   wa = S + 1;                 /* lo que ya se metió detrás de los botones */
	if (wb > S + VSA) wb = S + VSA;               /* lo que todavía no entra por arriba */
	const int16_t x = GH_CARRIL_X(c);
	Carril *k = &car[c];

	for (int32_t w = wb; w >= wa; ) {
		int32_t run = w - wa + 1;                 /* filas seguidas a dibujar (hacia abajo) */
		int32_t mem = Mod(VSA - w, VSA);
		if (run > VSA - mem) run = VSA - mem;     /* no pasar del final de la memoria del scroll */

		Nota *nota = 0; uint8_t zona = 0;         /* 1 = cola, 2 = cabeza */
		int32_t techo = INT32_MIN;                /* punto más alto de la nota más cercana por debajo de w */
		for (uint8_t i = 0; i < k->cant; i++) {
			Nota *n = Lugar(k, i);
			if (w > n->p && w <= n->p + n->largo)   { nota = n; zona = 1; break; }
			if (w <= n->p && w > n->p - ALTO)       { nota = n; zona = 2; break; }
			int32_t arriba = n->p + n->largo;
			if (arriba < w && arriba > techo) techo = arriba;
		}

		if (!nota) {                              /* vacío: negro hasta la siguiente nota */
			if (techo != INT32_MIN && run > w - techo) run = w - techo;
			FillRectFast(x, mem, GH_SPR_W, run, GH_COLOR_FONDO);
		} else if (zona == 1) {                   /* cola: mosaico de 8 filas */
			if (run > w - nota->p) run = w - nota->p;
			int32_t r = Mod(-w, TILE);
			if (run > TILE - r) run = TILE - r;
			LCD_BitmapFast(x, mem, GH_SPR_W, run, Cola(c, nota->estado) + r * FILA_BYTES);
		} else {                                  /* cabeza */
			int32_t f = nota->p - w;              /* fila del sprite 0..47 */
			if (run > ALTO - f) run = ALTO - f;
			if (nota->estado == N_ESPERA || nota->estado == N_FALLADA) {
				const uint8_t *spr = nota->largo ? gh_hold_head[c] : gh_note[c];
				LCD_BitmapFast(x, mem, GH_SPR_W, run, spr + f * FILA_BYTES);
			} else if (nota->largo && f < MITAD) {  /* nota larga ya tocada: arriba sigue la cola */
				if (run > MITAD - f) run = MITAD - f;
				int32_t r = Mod(-w, TILE);
				if (run > TILE - r) run = TILE - r;
				LCD_BitmapFast(x, mem, GH_SPR_W, run, Cola(c, nota->estado) + r * FILA_BYTES);
			} else {                              /* nota tocada: la cabeza desaparece */
				FillRectFast(x, mem, GH_SPR_W, run, GH_COLOR_FONDO);
			}
		}
		w -= run;
	}
}

static void Pintar_Nota(uint8_t c, const Nota *n) { Pintar(c, n->p - ALTO + 1, n->p + n->largo); }

/* ---------- Scroll ---------- */
static void Avanzar(uint32_t ahora) {
	int32_t nuevo = Px(ahora);
	if (nuevo <= S) return;
	int32_t viejo = S;
	S = nuevo;
	LCD_ScrollStart((uint16_t)(GH_SCROLL_INVERTIDO ? Mod(S, VSA) : Mod(-S, VSA)));

	int32_t desde = viejo + VSA + 1;              /* franja que acaba de entrar por arriba */
	if (desde < S + 1) desde = S + 1;             /* si se atrasó más de una pantalla: todo */
	for (uint8_t c = 0; c < GH_NUM_CARRILES; c++) Pintar(c, desde, S + VSA);
}

/* ---------- API ---------- */
void GH_Pista_Iniciar(void) {
	t0 = HAL_GetTick();
	S = 0;
	for (uint8_t c = 0; c < GH_NUM_CARRILES; c++) { car[c].ini = 0; car[c].cant = 0; car[c].boton = GH_BTN_REPOSO; }
	LCD_ScrollArea(0, VSA, BFA);
	LCD_ScrollStart(0);
	FillRectFast(0, 0, GH_PANTALLA_W, VSA, GH_COLOR_FONDO);
	GH_DibujarBotones();
}

void GH_Pista_Limpiar(void) {
	for (uint8_t c = 0; c < GH_NUM_CARRILES; c++) {
		car[c].ini = 0; car[c].cant = 0;
		Pintar(c, S + 1, S + VSA);                /* sin notas = todo negro */
	}
}

void GH_Pista_AgregarNota(uint8_t c, uint32_t t_golpe_ms, uint16_t largo_ms) {
	if (c >= GH_NUM_CARRILES) return;
	Carril *k = &car[c];
	if (k->cant >= GH_NOTAS_POR_CARRIL) { pista_descartadas++; return; }
	Nota *n = Lugar(k, k->cant);
	n->t        = t_golpe_ms;
	n->p        = Px(t_golpe_ms);
	n->largo_ms = largo_ms;
	n->largo    = (int16_t)(((uint32_t)largo_ms * GH_VELOCIDAD_PX_S) / 1000u);
	n->estado   = N_ESPERA;
	k->cant++;
	if (n->p - ALTO + 1 <= S + VSA) Pintar_Nota(c, n);   /* llegó tarde y ya debería verse: dibujarla ya */
}

void GH_Pista_Paquete(const uint8_t *b, uint32_t t_rx_ms) {
	switch (b[1]) {
		case GH_PKT_CANCION:
		case GH_PKT_ALTO:
			GH_Pista_Limpiar();
			break;
		case GH_PKT_NOTA: {
			uint8_t jugador = b[2] >> 4, carriles = b[2] & 0x0F;
			if (jugador != GH_JUGADOR_PANTALLA || carriles == 0) return;
			uint16_t faltan = (uint16_t)(b[3] | (b[4] << 8));
			uint16_t largo  = (uint16_t)(b[5] | (b[6] << 8));
			uint8_t c = 0;
			while (!(carriles & (1u << c))) c++;  /* una nota por botón: si viniera acorde, solo el primero */
			GH_Pista_AgregarNota(c, t_rx_ms + faltan, largo);
			break;
		}
		default: break;
	}
}

void GH_Pista_Presionar(uint8_t c, uint32_t t_ms) {
	if (c >= GH_NUM_CARRILES) return;
	Carril *k = &car[c];
	for (uint8_t i = 0; i < k->cant; i++) {
		Nota *n = Lugar(k, i);
		if (n->estado != N_ESPERA) continue;
		int32_t err = (int32_t)(t_ms - n->t);     /* <0 temprano, >0 tarde */
		if (err < -GH_VENTANA_ANTES_MS) return;   /* la más cercana aún está lejos: presión al aire */
		if (err >  GH_VENTANA_DESPUES_MS) continue;
		n->estado = n->largo ? N_SOSTENIDA : N_CORTA_OK;
		pista_aciertos++;
		Pintar_Nota(c, n);
		return;
	}
}

void GH_Pista_Actualizar(const uint8_t botones[4]) {
	uint32_t ahora = HAL_GetTick();
	Avanzar(ahora);

	for (uint8_t c = 0; c < GH_NUM_CARRILES; c++) {
		Carril *k = &car[c];
		uint8_t sosteniendo = 0;

		for (uint8_t i = 0; i < k->cant; i++) {
			Nota *n = Lugar(k, i);
			if (n->estado == N_ESPERA) {
				if ((int32_t)(ahora - n->t) > GH_VENTANA_DESPUES_MS) { n->estado = N_FALLADA; pista_fallos++; }
			} else if (n->estado == N_SOSTENIDA) {
				int32_t falta = (int32_t)(n->t + n->largo_ms - ahora);
				if (falta <= 0) {
					n->estado = N_LARGA_OK;                       /* aguantó toda la cola */
				} else if (!botones[c]) {
					if (falta <= GH_VENTANA_DESPUES_MS) n->estado = N_LARGA_OK;   /* soltó justo al final */
					else { n->estado = N_SOLTADA; Pintar_Nota(c, n); }           /* soltó antes: cola apagada */
				} else {
					sosteniendo = 1;
				}
			}
		}

		/* sacar las notas que ya pasaron completas detrás de los botones */
		while (k->cant) {
			Nota *n = Lugar(k, 0);
			if (n->estado == N_ESPERA || n->estado == N_SOSTENIDA) break;
			if (n->p + n->largo > S) break;           /* todavía se ve un pedazo */
			k->ini = (uint8_t)((k->ini + 1) % GH_NOTAS_POR_CARRIL);
			k->cant--;
		}

		uint8_t est = !botones[c] ? GH_BTN_REPOSO : (sosteniendo ? GH_BTN_SOSTENIDO : GH_BTN_PRESIONADO);
		if (est != k->boton) { GH_DibujarBoton(c, est); k->boton = est; }
	}
}
