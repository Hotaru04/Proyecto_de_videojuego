/*
 * juego.c
 * Menús, canción, resultados. Ver juego.h.
 */
#include "juego.h"
#include "songs.h"
#include "audio.h"
#include "beats.h"
#include "juez.h"
#include "controles.h"
#include "enlace_spi.h"
#include "gh_protocolo.h"
#include "registro.h"
#include "music.h"
#include "main.h"
#include "canciones_sd.h"

enum { E_MENU_CANCIONES = 0, E_JUGANDO, E_TERMINANDO, E_RESULTADOS, E_MENU_FIN };
static const char *const NOMBRE_ESTADO[] = { "menu de canciones", "jugando", "terminando", "resultados", "menu final" };

static uint8_t  estado = E_MENU_CANCIONES;
static uint8_t  cancion = 0;          /* la que se está jugando / la marcada en el menú */
static uint8_t  cursor = 0;
static uint32_t t_estado = 0;         /* cuándo se entró al estado actual */

/* ---------- Catálogo: canciones de la microSD, o las de la flash si no hay tarjeta ---------- */
static uint8_t desde_sd = 0;
static int16_t sd_cargada = -1;       /* cuál está ya en RAM (para no volver a leerla en la revancha) */
static const SongEntry *sd_entrada = 0;

static uint8_t Total(void) { return desde_sd ? CancionesSD_Total() : SONG_COUNT; }
static const char *Titulo(uint8_t i) { return desde_sd ? CancionesSD_Titulo(i) : song_table[i].titulo; }
static const SongEntry *Entrada(uint8_t i) {
	if (!desde_sd) return &song_table[i];
	if (sd_cargada != i || !sd_entrada) {
		sd_entrada = CancionesSD_Cargar(i);
		sd_cargada = sd_entrada ? i : -1;
	}
	return sd_entrada;
}

const char *Juego_Estado(void) { return NOMBRE_ESTADO[estado]; }

static void Pantalla(uint8_t modo, uint8_t marcada, uint8_t cuantas, uint8_t ganador) {
	/* a cada pantalla le dice qué jugador es (para "GANASTE" / "PERDISTE") */
	Enlace_Enviar(ENLACE_J1, GH_PKT_PANTALLA, modo, marcada, cuantas, ganador, 1);
	Enlace_Enviar(ENLACE_J2, GH_PKT_PANTALLA, modo, marcada, cuantas, ganador, 2);
}

/* Títulos de las canciones, de 3 letras por paquete */
static void Mandar_Titulos(void) {
	for (uint8_t i = 0; i < Total() && i < GH_TEXTO_LINEAS; i++) {
		const char *t = Titulo(i);
		uint8_t pos = 0, fin = 0;
		while (!fin && pos < GH_TEXTO_LARGO) {
			uint8_t c[3] = { 0, 0, 0 };
			for (uint8_t k = 0; k < 3; k++) {
				if (!fin && t[pos + k]) c[k] = (uint8_t)t[pos + k];
				else fin = 1;
			}
			Enlace_Enviar(ENLACE_AMBOS, GH_PKT_TEXTO, i, pos, c[0], c[1], c[2]);
			pos += 3;
		}
	}
}

static void Ir_A(uint8_t e) {
	estado = e;
	t_estado = HAL_GetTick();
	Registro("Juego: %s", NOMBRE_ESTADO[e]);
}

static void Menu_Canciones(void) {
	cursor = cancion;
	Mandar_Titulos();
	Pantalla(GH_PANT_CANCIONES, cursor, Total(), 0);
	Ir_A(E_MENU_CANCIONES);
}

static void Jugar(uint8_t idx) {
	if (idx >= Total()) idx = 0;
	const SongEntry *e = Entrada(idx);
	if (!e) { Registro("No se pudo cargar la cancion %u", idx); Menu_Canciones(); return; }
	cancion = idx;
	Registro("Cancion %u: %s  (%u bpm, %u pistas, jugador 1: %u pistas, jugador 2: %u pistas)",
	         idx, e->titulo, e->song->bpm, e->song->num_tracks, e->p1_n, e->p2_n);
	Audio_Pausa(0);
	Audio_SetMasterVolume(e->master ? e->master : AUDIO_MASTER_VOLUME);
	Audio_PlaySong(e->song, 0);                 /* 0 = suena una sola vez */
	Beats_Iniciar(e, idx);                      /* pantallas a la pista, juez y puntos en 0 */
	Ir_A(E_JUGANDO);
}

static void Resultados(void) {
	uint32_t p1 = juez_puntos[0], p2 = juez_puntos[1];
	uint8_t ganador = (p1 > p2) ? 1 : (p2 > p1) ? 2 : 0;
	/* a cada pantalla: sus puntos y los del rival */
	Enlace_Enviar(ENLACE_J1, GH_PKT_RESULTADO, 1, (uint8_t)p1, (uint8_t)(p1 >> 8), (uint8_t)(p1 >> 16), (uint8_t)(p1 >> 24));
	Enlace_Enviar(ENLACE_J1, GH_PKT_RESULTADO, 2, (uint8_t)p2, (uint8_t)(p2 >> 8), (uint8_t)(p2 >> 16), (uint8_t)(p2 >> 24));
	Enlace_Enviar(ENLACE_J2, GH_PKT_RESULTADO, 1, (uint8_t)p2, (uint8_t)(p2 >> 8), (uint8_t)(p2 >> 16), (uint8_t)(p2 >> 24));
	Enlace_Enviar(ENLACE_J2, GH_PKT_RESULTADO, 2, (uint8_t)p1, (uint8_t)(p1 >> 8), (uint8_t)(p1 >> 16), (uint8_t)(p1 >> 24));
	Pantalla(GH_PANT_RESULTADOS, 0, 0, ganador);
	if (ganador) Registro("GANADOR: jugador %u  (J1 %lu - J2 %lu)", ganador, (unsigned long)p1, (unsigned long)p2);
	else         Registro("EMPATE  (J1 %lu - J2 %lu)", (unsigned long)p1, (unsigned long)p2);
	Ir_A(E_RESULTADOS);
}

static void Menu_Fin(void) {
	cursor = 0;
	Pantalla(GH_PANT_FIN, cursor, 2, 0);
	Ir_A(E_MENU_FIN);
}

void Juego_Iniciar(uint8_t cancion_inicial) {
#if JUEGO_USAR_SD
	desde_sd = CancionesSD_Iniciar() > 0;
	Registro(desde_sd ? "Canciones: de la microSD" : "Canciones: de la flash (sin microSD o sin .ghs)");
#endif
	cancion = (cancion_inicial < Total()) ? cancion_inicial : 0;
#if JUEGO_INICIAR_EN_MENU
	Menu_Canciones();
#else
	Jugar(cancion);
#endif
}

/* Botones en los menús: flechas mueven, A (verde) o Start eligen. Devuelve 1 si se eligió. */
static uint8_t Navegar(uint8_t cuantas, uint8_t modo) {
	Control_Evento ev;
	uint8_t elegido = 0;
	while (Controles_Siguiente(&ev)) {
		if (!ev.presionado || elegido) continue;
		if (ev.carril == CONTROL_ARRIBA && cuantas) {
			cursor = (uint8_t)((cursor + cuantas - 1) % cuantas);
			Pantalla(modo, cursor, cuantas, 0);
		} else if (ev.carril == CONTROL_ABAJO && cuantas) {
			cursor = (uint8_t)((cursor + 1) % cuantas);
			Pantalla(modo, cursor, cuantas, 0);
		} else if (ev.carril == 0 || ev.carril == CONTROL_START) {
			elegido = 1;
		}
	}
	return elegido;
}

void Juego_Actualizar(void) {
	switch (estado) {
		case E_MENU_CANCIONES:
			if (Navegar(Total(), GH_PANT_CANCIONES)) Jugar(cursor);
			break;

		case E_JUGANDO:
			Beats_Actualizar();                 /* notas hacia las pantallas */
			Juez_Actualizar();                  /* botones, puntos, mute, pausa */
			if (!Music_IsPlaying() && !Juez_EnPausa()) Ir_A(E_TERMINANDO);
			break;

		case E_TERMINANDO:                      /* deja que el juez califique las últimas notas */
			Juez_Actualizar();
			if (HAL_GetTick() - t_estado >= JUEGO_ESPERA_FIN_MS) Resultados();
			break;

		case E_RESULTADOS:
			if (Navegar(0, GH_PANT_RESULTADOS) || HAL_GetTick() - t_estado >= JUEGO_RESULTADOS_MS) Menu_Fin();
			break;

		case E_MENU_FIN:
			if (Navegar(2, GH_PANT_FIN)) {
				if (cursor == 0) Menu_Canciones();   /* SELECCIONAR CANCION */
				else             Jugar(cancion);     /* REVANCHA */
			}
			break;
	}
}
