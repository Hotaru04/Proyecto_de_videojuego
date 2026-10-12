/*
 * gh_pista.c
 * Pista con scroll por hardware. Ver gh_pista.h.
 *
 * Pantalla:  filas 0..19 fijas (puntos)  |  20..261 se desplazan (pista)  |  262..319 fijas (botones)
 *
 * Coordenadas:
 *   S       = px que ha bajado la pista desde GH_Pista_Iniciar (crece con el tiempo: S = t * velocidad).
 *   "mundo" = posición fija de cada nota en la pista. Su punto p está en la fila  y = 262 + S - p.
 *             Una nota que debe golpearse a la hora T tiene p = T * velocidad: a esa hora queda encima de su
 *             botón (y = 262).
 *   Fila de MEMORIA donde se dibuja algo del mundo w:  20 + (242 - w) mod 242  -> no depende de S,
 *   así que lo que se dibuja una vez se queda quieto en memoria y el scroll lo mueve.
 */
#include "gh_pista.h"
#include "gh_protocolo.h"
#include "gh_notas.h"
#include "gh_sprites.h"
#include "gh_pausa.h"
#include "ili9341.h"
#include "main.h"
#include <stdio.h>

#define HUD         GH_HUD_ALTO                   /* 20 filas fijas arriba (puntos) */
#define VSA         (GH_BOTON_Y - HUD)            /* 242 filas que se desplazan */
#define BFA         (GH_PANTALLA_H - GH_BOTON_Y)  /* 58 filas fijas abajo (botones) */
#define ALTO        GH_SPR_H                      /* 48 */
#define MITAD       (GH_SPR_H / 2)
#define TILE        GH_COLA_TILE_H                /* 8 */
#define FILA_BYTES  (GH_SPR_W * 2)

enum { N_ESPERA = 0, N_CORTA_OK, N_SOSTENIDA, N_SOLTADA, N_LARGA_OK, N_FALLADA };

typedef struct {
	uint32_t t;          /* ms (HAL_GetTick) en que llega al botón */
	int32_t  p;          /* posición en el mundo (px) */
	int16_t  largo;      /* cola en px (0 = nota normal) */
	uint16_t largo_ms;
	uint8_t  id;         /* número que le puso MusicaGame (para los eventos) */
	uint8_t  estado;
} Nota;

typedef struct {
	Nota    n[GH_NOTAS_POR_CARRIL];
	uint8_t ini, cant;   /* cola circular: n[ini] es la más vieja (la más cerca del botón) */
	uint8_t boton;       /* estado dibujado del botón */
} Carril;

static uint8_t btn_presionados = 0, btn_sosteniendo = 0;   /* lo último que mandó MusicaGame */

static Carril   car[GH_NUM_CARRILES];
static uint32_t t0;
static int32_t  S;

static uint8_t  activa = 1;           /* 0 = hay un menú en pantalla (la pista no se dibuja) */
static uint8_t  pausa = 0;            /* 1 = la pista está congelada */
static uint32_t pausa_t;              /* cuándo empezó la pausa */
static uint8_t  pausa_jugador = 0;
static uint8_t  cuenta = 0;           /* número de la cuenta atrás en pantalla (0 = no hay) */
static uint32_t puntos = 0;
static uint8_t  ultima_cal = 0xFF;    /* 0xFF = todavía nada */

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
		int32_t mem = HUD + Mod(VSA - w, VSA);
		if (run > HUD + VSA - mem) run = HUD + VSA - mem;   /* no pasar del final de la memoria del scroll */

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

/* ---------- Sprite de pausa (al centro de la pista) ----------
 * La pista está en memoria "enrollada" por el scroll: la fila de pantalla y está en la fila de memoria
 * HUD + (VSA - w) mod VSA, con w = S + 262 - y. Se dibuja por tramos de filas seguidas en memoria. */
#define PAUSA_X  ((GH_PANTALLA_W - GH_PAUSA_W) / 2)
#define PAUSA_Y  (HUD + (VSA - GH_PAUSA_H) / 2)

static int32_t Mem_De_Fila(int32_t y) { return HUD + Mod(VSA - (S + GH_BOTON_Y - y), VSA); }

static void Pausa_Dibujar(uint8_t mostrar) {
	int32_t y = PAUSA_Y, fin = PAUSA_Y + GH_PAUSA_H;
	while (y < fin) {
		int32_t mem = Mem_De_Fila(y);
		int32_t n = fin - y;
		if (n > HUD + VSA - mem) n = HUD + VSA - mem;           /* hasta donde la memoria da la vuelta */
		if (mostrar) LCD_BitmapFast(PAUSA_X, mem, GH_PAUSA_W, n, gh_pausa + (y - PAUSA_Y) * GH_PAUSA_W * 2);
		else         FillRectFast(PAUSA_X, mem, GH_PAUSA_W, n, GH_COLOR_FONDO);
		y += n;
	}
	if (!mostrar) {                                              /* volver a pintar las notas que tapaba */
		int32_t wa = S + GH_BOTON_Y - (PAUSA_Y + GH_PAUSA_H - 1), wb = S + GH_BOTON_Y - PAUSA_Y;
		for (uint8_t c = 0; c < GH_NUM_CARRILES; c++) Pintar(c, wa, wb);
	}
}

/* ---------- Cuenta atrás (3, 2, 1 grandes al centro de la pista) ----------
 * Al empezar la cuenta la pista se reinicia (scroll en 0) y se queda quieta, así que la memoria es igual a la
 * pantalla y se dibuja directo. Las letras son las de 16x16 (bigFont) agrandadas CUENTA_ESCALA veces. */
#define CUENTA_ESCALA  6
#define CUENTA_LADO    (16 * CUENTA_ESCALA)                    /* 96 px */
#define CUENTA_X       ((GH_PANTALLA_W - CUENTA_LADO) / 2)
#define CUENTA_Y       (HUD + (VSA - CUENTA_LADO) / 2)
#define C_AMARILLO     0xFFE0
extern const uint16_t bigFont[];

static void Cuenta_Dibujar(uint8_t n) {
	FillRectFast(CUENTA_X, CUENTA_Y, CUENTA_LADO, CUENTA_LADO, GH_COLOR_FONDO);
	if (!n || n > 9) return;
	const uint16_t *g = bigFont + ((('0' + n) - 32) * 16);
	for (int f = 0; f < 16; f++) {
		uint16_t bits = g[f];
		for (int col = 0; col < 16; ) {                          /* tramos seguidos de pixeles encendidos */
			if (!(bits & (0x8000u >> col))) { col++; continue; }
			int ini = col;
			while (col < 16 && (bits & (0x8000u >> col))) col++;
			FillRectFast(CUENTA_X + ini * CUENTA_ESCALA, CUENTA_Y + f * CUENTA_ESCALA,
			             (col - ini) * CUENTA_ESCALA, CUENTA_ESCALA, C_AMARILLO);
		}
	}
}

static void Hud_Dibujar(void);

static void Cuenta_Paquete(uint8_t n, uint32_t t_rx_ms) {
	if (n) {
		if (!cuenta) GH_Pista_Iniciar();              /* pista limpia y quieta, scroll en 0 */
		cuenta = n;
		Cuenta_Dibujar(n);
		Hud_Dibujar();
	} else if (cuenta) {
		Cuenta_Dibujar(0);                            /* borra el número */
		cuenta = 0;
		t0 = t_rx_ms;                                 /* la pista arranca desde aquí (S sigue en 0) */
		Hud_Dibujar();
	}
}

/* ---------- Scroll ---------- */
static void Avanzar(uint32_t ahora) {
	int32_t nuevo = Px(ahora);
	if (nuevo <= S) return;
	int32_t viejo = S;
	S = nuevo;
	LCD_ScrollStart((uint16_t)(HUD + (GH_SCROLL_INVERTIDO ? Mod(S, VSA) : Mod(-S, VSA))));

	int32_t desde = viejo + VSA + 1;              /* franja que acaba de entrar por arriba */
	if (desde < S + 1) desde = S + 1;             /* si se atrasó más de una pantalla: todo */
	for (uint8_t c = 0; c < GH_NUM_CARRILES; c++) Pintar(c, desde, S + VSA);
}

/* ---------- Franja de puntos (arriba, fija) ---------- */
#define C_BLANCO   0xFFFF
#define C_VERDE    0x07E0
#define C_CIAN     0x07FF
#define C_NARANJA  0xFD20
#define C_ROJO     0xF800

static void Hud_Dibujar(void) {
	char txt[16];
	FillRectFast(0, 0, GH_PANTALLA_W, HUD, GH_COLOR_FONDO);
	snprintf(txt, sizeof(txt), "%lu", (unsigned long)puntos);
	LCD_Print(txt, 4, 2, 2, C_BLANCO, GH_COLOR_FONDO);                 /* puntos: letra grande 16x16 */

	const char *msg = 0; uint16_t col = C_BLANCO;
	if (pausa) {
		snprintf(txt, sizeof(txt), "PAUSA J%u", pausa_jugador);
		msg = txt; col = C_ROJO;
	} else if (cuenta) {
		msg = "PREPARATE"; col = C_AMARILLO;
	} else {
		switch (ultima_cal) {
			case GH_CAL_PERFECTO: msg = "PERFECTO +100"; col = C_VERDE;   break;
			case GH_CAL_MEDIO:    msg = "MEDIO +50";     col = C_CIAN;    break;
			case GH_CAL_JUSTO:    msg = "JUSTO +10";     col = C_NARANJA; break;
			case GH_CAL_FALLO:    msg = "FALLO";         col = C_ROJO;    break;
			default: break;
		}
	}
	if (msg) {                                                          /* calificación: letra chica 8x12, a la derecha */
		int len = 0; while (msg[len]) len++;
		LCD_Print((char *)msg, GH_PANTALLA_W - 4 - len * 8, 4, 1, col, GH_COLOR_FONDO);
	}
}

/* ---------- API ---------- */
void GH_Pista_Iniciar(void) {
	t0 = HAL_GetTick();
	S = 0;
	for (uint8_t c = 0; c < GH_NUM_CARRILES; c++) { car[c].ini = 0; car[c].cant = 0; car[c].boton = GH_BTN_REPOSO; }
	btn_presionados = 0; btn_sosteniendo = 0;
	pausa = 0; cuenta = 0; puntos = 0; ultima_cal = 0xFF; activa = 1;
	LCD_ScrollArea(HUD, VSA, BFA);
	LCD_ScrollStart(HUD);
	FillRectFast(0, HUD, GH_PANTALLA_W, GH_PANTALLA_H - HUD, GH_COLOR_FONDO);   /* pista y fila de botones */
	Hud_Dibujar();
	GH_DibujarBotones();
}

void GH_Pista_Limpiar(void) {
	for (uint8_t c = 0; c < GH_NUM_CARRILES; c++) {
		car[c].ini = 0; car[c].cant = 0;
		Pintar(c, S + 1, S + VSA);                /* sin notas = todo negro */
	}
}

void GH_Pista_AgregarNota(uint8_t c, uint8_t id, uint32_t t_golpe_ms, uint16_t largo_ms) {
	if (c >= GH_NUM_CARRILES) return;
	Carril *k = &car[c];
	if (k->cant >= GH_NOTAS_POR_CARRIL) { pista_descartadas++; return; }
	Nota *n = Lugar(k, k->cant);
	n->t        = t_golpe_ms;
	n->p        = Px(t_golpe_ms);                 /* a la hora del golpe queda encima de su botón */
	n->largo_ms = largo_ms;
	n->largo    = (int16_t)(((uint32_t)largo_ms * GH_VELOCIDAD_PX_S) / 1000u);
	n->id       = id;
	n->estado   = N_ESPERA;
	k->cant++;
	if (n->p - ALTO + 1 <= S + VSA) Pintar_Nota(c, n);   /* llegó tarde y ya debería verse: dibujarla ya */
}

/* Lo que decidió el juez sobre la nota 'id' del carril c */
static void Aplicar_Evento(uint8_t c, uint8_t id, uint8_t evt) {
	Carril *k = &car[c];
	if (evt == GH_EVT_ACIERTO || evt == GH_EVT_SOSTENIDA) pista_aciertos++;   /* se cuenta aunque la nota ya no se vea */
	if (evt == GH_EVT_FALLO) pista_fallos++;
	for (uint8_t i = 0; i < k->cant; i++) {
		Nota *n = Lugar(k, i);
		if (n->id != id) continue;
		switch (evt) {
			case GH_EVT_ACIERTO:   n->estado = N_CORTA_OK;  break;
			case GH_EVT_SOSTENIDA: n->estado = N_SOSTENIDA; break;
			case GH_EVT_SOLTADA:   n->estado = N_SOLTADA;   break;
			case GH_EVT_COMPLETA:  n->estado = N_LARGA_OK;  break;
			case GH_EVT_FALLO:     n->estado = N_FALLADA;   return;   /* se ve igual: no repintar */
			default: return;
		}
		Pintar_Nota(c, n);
		return;
	}
}

void GH_Pista_Paquete(const uint8_t *b, uint32_t t_rx_ms) {
	switch (b[1]) {
		case GH_PKT_CANCION:
			if (!activa) { GH_Pista_Iniciar(); break; }   /* venía de un menú: pista nueva completa */
			/* fall through */
		case GH_PKT_ALTO:
			if (pausa) { pausa = 0; t0 += t_rx_ms - pausa_t; }
			if (cuenta) Cuenta_Paquete(0, t_rx_ms);
			if (activa) GH_Pista_Limpiar();
			break;
		case GH_PKT_CUENTA:
			if (activa) Cuenta_Paquete(b[2], t_rx_ms);
			break;
		case GH_PKT_PUNTOS:
			if (!activa) break;
			puntos = (uint32_t)b[2] | ((uint32_t)b[3] << 8) | ((uint32_t)b[4] << 16) | ((uint32_t)b[5] << 24);
			ultima_cal = b[6];
			Hud_Dibujar();
			break;
		case GH_PKT_PAUSA:
			if (!activa) break;
			if (b[2] && !pausa) {
				pausa = 1; pausa_t = t_rx_ms; pausa_jugador = b[3];
				Pausa_Dibujar(1);
			} else if (!b[2] && pausa) {
				Pausa_Dibujar(0);                     /* antes de mover la pista: borra el sprite en su lugar */
				pausa = 0;
				t0 += t_rx_ms - pausa_t;              /* la pista sigue donde se quedó */
			}
			Hud_Dibujar();
			break;
		case GH_PKT_NOTA: {
			uint16_t faltan = (uint16_t)(b[3] | (b[4] << 8));
			uint16_t largo  = (uint16_t)(b[5] | (b[6] << 8));
			GH_Pista_AgregarNota(GH_CARRIL_DE(b[2]), GH_ID_DE(b[2]), t_rx_ms + faltan, largo);
			break;
		}
		case GH_PKT_EVENTO:
			Aplicar_Evento(GH_CARRIL_DE(b[2]), GH_ID_DE(b[2]), b[3]);
			break;
		case GH_PKT_BOTONES:
			btn_presionados = b[2] & 0x0F;
			btn_sosteniendo = b[3] & 0x0F;
			break;
		default: break;
	}
}

void GH_Pista_Desactivar(void) {
	activa = 0;
	pausa = 0;
	cuenta = 0;
	LCD_ScrollStart(HUD);                         /* memoria = pantalla: los menús dibujan directo */
}

uint8_t GH_Pista_Activa(void) { return activa; }

void GH_Pista_Actualizar(void) {
	if (!activa) return;
	if (!pausa && !cuenta) Avanzar(HAL_GetTick());

	for (uint8_t c = 0; c < GH_NUM_CARRILES; c++) {
		Carril *k = &car[c];

		/* sacar las notas que ya pasaron completas detrás de los botones (ya no se ven) */
		while (k->cant) {
			Nota *n = Lugar(k, 0);
			if (n->p + n->largo > S) break;
			k->ini = (uint8_t)((k->ini + 1) % GH_NOTAS_POR_CARRIL);
			k->cant--;
		}

		/* botón: como lo dice MusicaGame */
		uint8_t m = (uint8_t)(1u << c);
		uint8_t est = !(btn_presionados & m) ? GH_BTN_REPOSO
		            : (btn_sosteniendo & m)  ? GH_BTN_SOSTENIDO : GH_BTN_PRESIONADO;
		if (est != k->boton) { GH_DibujarBoton(c, est); k->boton = est; }
	}
}
