/*
 * gh_menu.c
 * Dibujo de la lista de canciones, los resultados y el menú final. Ver gh_menu.h.
 * Mientras hay un menú la pista está desactivada y el scroll en su posición normal,
 * así que se dibuja directo en coordenadas de pantalla (240 x 320).
 */
#include "gh_menu.h"
#include "gh_pista.h"
#include "gh_protocolo.h"
#include "ili9341.h"
#include <stdio.h>
#include <string.h>

#define ANCHO      240
#define ALTO_PANT  320

#define C_FONDO    0x0000
#define C_BLANCO   0xFFFF
#define C_CIAN     0x07FF
#define C_MAGENTA  0xF81F
#define C_AMARILLO 0xFFE0
#define C_VERDE    0x07E0
#define C_ROJO     0xF800
#define C_GRIS     0x7BEF
#define C_MORADO   0x300C            /* fondo de las opciones */

static char     titulos[GH_TEXTO_LINEAS][GH_TEXTO_LARGO + 1];
static uint32_t pts_mios = 0, pts_rival = 0;
static uint8_t  modo = 0;            /* lo que está dibujado ahora (GH_PANT_*), 0 = nada */
static uint8_t  marcada_dib = 0xFF;
static uint8_t  cuantas_dib = 0;
static uint8_t  primera_dib = 0;     /* primera canción visible dibujada */
static uint8_t  titulos_nuevos = 0;  /* llegó texto distinto al dibujado */

/* ---------- ayudas de dibujo ---------- */
static void Texto(const char *s, int y, uint8_t grande, uint16_t color, uint16_t fondo) {
	int ancho_letra = grande ? 16 : 8;
	int len = (int)strlen(s);
	int x = (ANCHO - len * ancho_letra) / 2;
	if (x < 0) x = 0;
	LCD_Print((char *)s, x, y, grande ? 2 : 1, color, fondo);
}

static void Marco(void) {                                     /* borde neón como el del sprite de pausa */
	FillRectFast(0, 0, ANCHO, ALTO_PANT, C_FONDO);
	FillRectFast(0, 0, ANCHO, 3, C_MAGENTA);
	FillRectFast(0, ALTO_PANT - 3, ANCHO, 3, C_MAGENTA);
	FillRectFast(0, 0, 3, ALTO_PANT, C_MAGENTA);
	FillRectFast(ANCHO - 3, 0, 3, ALTO_PANT, C_MAGENTA);
	FillRectFast(6, 6, ANCHO - 12, 1, C_CIAN);
	FillRectFast(6, ALTO_PANT - 7, ANCHO - 12, 1, C_CIAN);
	FillRectFast(6, 6, 1, ALTO_PANT - 12, C_CIAN);
	FillRectFast(ANCHO - 7, 6, 1, ALTO_PANT - 12, C_CIAN);
}

#define PASO       36                /* separación entre opciones */
#define LISTA_Y    70                /* primera fila de la lista de canciones */
#define MAX_LETRAS 26                /* lo que cabe en una opción (los títulos más largos se cortan) */

/* Una opción: caja de 224 x 30; la marcada va rellena y con flecha */
static void Opcion(int i, int y0, const char *txt_largo, uint8_t marcada) {
	int y = y0 + i * PASO;
	char txt[MAX_LETRAS + 1];
	strncpy(txt, txt_largo, MAX_LETRAS); txt[MAX_LETRAS] = 0;
	uint16_t fondo = marcada ? C_CIAN : C_MORADO;
	uint16_t letra = marcada ? C_FONDO : C_BLANCO;
	FillRectFast(8, y, ANCHO - 16, 30, fondo);
	if (!marcada) {                                           /* contorno magenta en las no marcadas */
		FillRectFast(8, y, ANCHO - 16, 1, C_MAGENTA);
		FillRectFast(8, y + 29, ANCHO - 16, 1, C_MAGENTA);
	}
	Texto(txt, y + 9, 0, letra, fondo);
	if (marcada && strlen(txt) <= 24) LCD_Print(">", 14, y + 9, 1, C_FONDO, fondo);   /* flecha si cabe */
}

/* ---------- pantallas ---------- */
/* Triángulo chico (hay más canciones arriba / abajo) */
static void Flecha(int y, uint8_t arriba, uint8_t mostrar) {
	for (int i = 0; i < 6; i++) {
		int fila = arriba ? i : 5 - i;
		FillRectFast(ANCHO / 2 - i * 2, y + fila, i * 4 + 1, 1, mostrar ? C_AMARILLO : C_FONDO);
	}
}

/* Lista de canciones: se ven GH_MENU_FILAS a partir de 'primera'; titulos[f] es la fila visible f */
static void Dibujar_Canciones(uint8_t marcada, uint8_t cuantas, uint8_t primera, uint8_t completa, uint8_t lista) {
	char t[12];
	if (completa) {
		Marco();
		Texto("ELIGE", 22, 1, C_MAGENTA, C_FONDO);
		Texto("UNA CANCION", 42, 1, C_CIAN, C_FONDO);
		Texto("FLECHAS: MOVER   A: JUGAR", 292, 0, C_AMARILLO, C_FONDO);
	}
	uint8_t filas = (cuantas > primera) ? (uint8_t)(cuantas - primera) : 0;
	if (filas > GH_MENU_FILAS) filas = GH_MENU_FILAS;
	for (uint8_t f = 0; f < GH_MENU_FILAS; f++) {
		uint8_t i = (uint8_t)(primera + f);
		if (f >= filas) {                                         /* fila sin canción */
			if (lista) FillRectFast(8, LISTA_Y + f * PASO, ANCHO - 16, 30, C_FONDO);
			continue;
		}
		if (lista || i == marcada || i == marcada_dib)
			Opcion(f, LISTA_Y, titulos[f][0] ? titulos[f] : "?", i == marcada);
	}
	if (lista) {
		Flecha(LISTA_Y - 9, 1, primera > 0);                      /* hay más arriba */
		Flecha(LISTA_Y + GH_MENU_FILAS * PASO - 3, 0, primera + GH_MENU_FILAS < cuantas);   /* hay más abajo */
	}
	snprintf(t, sizeof(t), "%u/%u", (unsigned)(marcada + 1), (unsigned)cuantas);   /* número de canción, arriba a la izquierda */
	FillRectFast(12, 10, 56, 12, C_FONDO);
	if (cuantas > GH_MENU_FILAS) LCD_Print(t, 12, 10, 1, C_GRIS, C_FONDO);
}

static void Dibujar_Resultados(uint8_t ganador, uint8_t yo) {
	char t[32];
	Marco();
	Texto("RESULTADOS", 24, 1, C_CIAN, C_FONDO);
	if (ganador == 0) {
		Texto("EMPATE", 80, 1, C_AMARILLO, C_FONDO);
	} else if (ganador == yo) {
		Texto("GANASTE!", 80, 1, C_VERDE, C_FONDO);
	} else {
		Texto("PERDISTE", 80, 1, C_ROJO, C_FONDO);
	}
	if (ganador) { snprintf(t, sizeof(t), "GANADOR: JUGADOR %u", ganador); Texto(t, 112, 0, C_BLANCO, C_FONDO); }

	FillRectFast(20, 140, ANCHO - 40, 1, C_MAGENTA);
	snprintf(t, sizeof(t), "TUS PUNTOS");     Texto(t, 156, 0, C_GRIS, C_FONDO);
	snprintf(t, sizeof(t), "%lu", (unsigned long)pts_mios);  Texto(t, 172, 1, C_BLANCO, C_FONDO);
	snprintf(t, sizeof(t), "RIVAL");          Texto(t, 206, 0, C_GRIS, C_FONDO);
	snprintf(t, sizeof(t), "%lu", (unsigned long)pts_rival); Texto(t, 222, 1, C_GRIS, C_FONDO);
	FillRectFast(20, 252, ANCHO - 40, 1, C_MAGENTA);
	Texto("A: CONTINUAR", 292, 0, C_AMARILLO, C_FONDO);
}

static void Dibujar_Fin(uint8_t marcada, uint8_t completa) {
	if (completa) {
		Marco();
		Texto("Y AHORA?", 40, 1, C_MAGENTA, C_FONDO);
		Texto("FLECHAS: MOVER   A: ELEGIR", 292, 0, C_AMARILLO, C_FONDO);
	}
	Opcion(0, 120, "SELECCIONAR CANCION", marcada == 0);
	Opcion(1, 120, "REVANCHA", marcada == 1);
}

uint8_t GH_Menu_Paquete(const uint8_t *b) {
	switch (b[1]) {
		case GH_PKT_TEXTO: {
			uint8_t linea = b[2], pos = b[3];
			if (linea >= GH_TEXTO_LINEAS) return 1;
			for (uint8_t k = 0; k < 3; k++) {
				if (pos + k >= GH_TEXTO_LARGO) break;
				if (titulos[linea][pos + k] != (char)b[4 + k]) titulos_nuevos = 1;
				titulos[linea][pos + k] = (char)b[4 + k];
				if (!b[4 + k]) break;
			}
			titulos[linea][GH_TEXTO_LARGO] = 0;
			return 1;
		}
		case GH_PKT_RESULTADO: {
			uint32_t p = (uint32_t)b[3] | ((uint32_t)b[4] << 8) | ((uint32_t)b[5] << 16) | ((uint32_t)b[6] << 24);
			if (b[2] == 1) pts_mios = p; else pts_rival = p;
			return 1;
		}
		case GH_PKT_PANTALLA: {
			uint8_t nuevo = b[2], marcada = b[3], cuantas = b[4];
			uint8_t primera = (nuevo == GH_PANT_CANCIONES) ? b[5] : 0;
			uint8_t completa = (nuevo != modo) || GH_Pista_Activa();
			uint8_t lista = completa || cuantas != cuantas_dib || primera != primera_dib
			             || (nuevo == GH_PANT_CANCIONES && titulos_nuevos);
			/* MusicaGame la vuelve a mandar cada segundo en los menús: si no cambió nada, no se redibuja */
			if (!lista && marcada == marcada_dib) return 1;
			if (GH_Pista_Activa()) GH_Pista_Desactivar();
			switch (nuevo) {
				case GH_PANT_CANCIONES:  Dibujar_Canciones(marcada, cuantas, primera, completa, lista); break;
				case GH_PANT_RESULTADOS: Dibujar_Resultados(b[5], b[6]);               break;
				case GH_PANT_FIN:        Dibujar_Fin(marcada, completa);               break;
				default: break;
			}
			modo = nuevo;
			marcada_dib = marcada;
			cuantas_dib = cuantas;
			primera_dib = primera;
			if (nuevo == GH_PANT_CANCIONES) titulos_nuevos = 0;
			return 1;
		}
		case GH_PKT_CANCION:
			modo = 0;                                         /* la pista retoma la pantalla */
			return 0;
		default:
			return 0;
	}
}
