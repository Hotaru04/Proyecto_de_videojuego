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

//enum { E_MENU_CANCIONES = 0, E_CUENTA, E_ENTRADA, E_JUGANDO, E_TERMINANDO, E_RESULTADOS, E_MENU_FIN };
//static const char *const NOMBRE_ESTADO[] = { "menu de canciones", "cuenta atras", "entrada", "jugando", "terminando",
//                                             "resultados", "menu final" };

/////////////////////////////

enum {
    E_INICIO = 0,
    E_MENU_CANCIONES,
    E_CUENTA,
    E_ENTRADA,
    E_JUGANDO,
    E_TERMINANDO,
    E_RESULTADOS,
    E_MENU_FIN
};

static const char *const NOMBRE_ESTADO[] = {
    "pantalla de inicio",
    "menu de canciones",
    "cuenta atras",
    "entrada",
    "jugando",
    "terminando",
    "resultados",
    "menu final"
};

////////////////////////////

//static uint8_t  estado = E_MENU_CANCIONES;
//////////////
static uint8_t estado = E_INICIO;
//////////////////
static uint8_t  cancion = 0;          /* la que se está jugando / la marcada en el menú */
static uint8_t  cursor = 0;
static uint32_t t_estado = 0;         /* cuándo se entró al estado actual */
static uint32_t t_reenvio = 0;        /* último reenvío del menú */
static uint8_t  cuenta = 0;           /* número que se está mostrando */
static const SongEntry *actual = 0;   /* la canción que va a sonar */

/* ---------- Catálogo: SOLO la microSD (en la flash no hay canciones) ---------- */
static int16_t sd_cargada = -1;       /* cuál está ya en RAM (para no volver a leerla en la revancha) */
static const SongEntry *sd_entrada = 0;
static uint8_t primera = 0;           /* primera canción visible en el menú (la lista se recorre) */
static uint32_t t_buscar_sd = 0;      /* último intento de leer la tarjeta (si no había canciones) */
static const char *aviso_sd = "";     /* lo que se muestra si no hay canciones */

static uint8_t Total(void) { return CancionesSD_Total(); }
static const char *Titulo(uint8_t i) { return CancionesSD_Titulo(i); }
static const SongEntry *Entrada(uint8_t i) {
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

static void Mandar_Linea(uint8_t fila, const char *t);

/* Títulos de las canciones que se ven (GH_MENU_FILAS a partir de 'primera') */
static void Mandar_Titulos(void) {
	if (!Total()) { Mandar_Linea(0, aviso_sd); return; }
	for (uint8_t f = 0; f < GH_MENU_FILAS && primera + f < Total(); f++) Mandar_Linea(f, Titulo(primera + f));
}

/* Lista de canciones: la ventana sigue al cursor; si cambia, se mandan los títulos nuevos */
static void Mostrar_Canciones(uint8_t titulos) {
	uint8_t p = primera;
	if (cursor < p) p = cursor;
	if (cursor >= p + GH_MENU_FILAS) p = (uint8_t)(cursor - GH_MENU_FILAS + 1);
	if (p != primera) { primera = p; titulos = 1; }
	if (titulos) Mandar_Titulos();
	uint8_t n = Total() ? Total() : 1;             /* sin canciones: una fila con el aviso */
	Pantalla(GH_PANT_CANCIONES, cursor, n, primera);
}

/* Un título, de 3 letras por paquete */
static void Mandar_Linea(uint8_t fila, const char *t) {
	uint8_t pos = 0, fin = 0;
	while (!fin && pos < GH_TEXTO_LARGO) {
		uint8_t c[3] = { 0, 0, 0 };
		for (uint8_t k = 0; k < 3; k++) {
			if (!fin && t[pos + k]) c[k] = (uint8_t)t[pos + k];
			else fin = 1;
		}
		Enlace_Enviar(ENLACE_AMBOS, GH_PKT_TEXTO, fila, pos, c[0], c[1], c[2]);
		pos += 3;
	}
}

static void Ir_A(uint8_t e) {
	estado = e;
	t_estado = HAL_GetTick();
	t_reenvio = t_estado;
	Registro("Juego: %s", NOMBRE_ESTADO[e]);
}

///////////////////////////
//Pantalla de inicio

static void Menu_Inicio(void)
{
    /* Mostrar pantalla inicial en ambos jugadores */
    Pantalla(GH_PANT_INICIO, 0, 0, 0);

    /* Cambiar al estado de inicio */
    Ir_A(E_INICIO);
}

///////////////////////////////


static void Menu_Canciones(void) {
	cursor = (cancion < Total()) ? cancion : 0;
	Mostrar_Canciones(1);
	Ir_A(E_MENU_CANCIONES);
}

/* Lee la tarjeta (al encender, y otra vez cada JUEGO_BUSCAR_SD_MS si no había canciones) */
static void Buscar_SD(void) {
	sd_cargada = -1; sd_entrada = 0;
	uint8_t n = CancionesSD_Iniciar();
	aviso_sd = CancionesSD_Montada() ? "MICROSD SIN CANCIONES" : "NO HAY MICROSD";
	if (n) Registro("Canciones: %u en la microSD", n);
	else   Registro("Canciones: ninguna (%s). Reintento en %u ms", aviso_sd, JUEGO_BUSCAR_SD_MS);
	t_buscar_sd = HAL_GetTick();
}

static void Jugar(uint8_t idx) {
	if (!Total()) return;                       /* no hay canciones que tocar */
	if (idx >= Total()) idx = 0;
	const SongEntry *e = Entrada(idx);
	if (!e) { Registro("No se pudo cargar la cancion %u", idx); Menu_Canciones(); return; }
	cancion = idx;
	actual = e;
	Registro("Cancion %u: %s  (%u bpm, %u pistas, jugador 1: %u pistas, jugador 2: %u pistas)",
	         idx, e->titulo, e->song->bpm, e->song->num_tracks, e->p1_n, e->p2_n);
	Beats_Iniciar(e, idx);                      /* pantallas a la pista, juez y puntos en 0 (todavía sin audio) */
	cuenta = JUEGO_CUENTA_DESDE;
	Enlace_Enviar(ENLACE_AMBOS, GH_PKT_CUENTA, cuenta, 0, 0, 0, 0);
	Ir_A(E_CUENTA);
}

static void Descartar_Botones(void) {          /* en la cuenta y la entrada los botones no hacen nada */
	Control_Evento ev;
	while (Controles_Siguiente(&ev)) { }
}

static void Arrancar_Audio(void) {
	Audio_Pausa(0);
	Audio_SetMasterVolume(actual->master ? actual->master : AUDIO_MASTER_VOLUME);
	Audio_PlaySong(actual->song, 0);            /* 0 = suena una sola vez */
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

/* SALIR AL MENU desde la pausa: la partida termina aquí y se muestra quién iba ganando */
static uint8_t Atender_Pausa(void) {
	if (Juez_Pedido() != JUEZ_PEDIDO_SALIR) return 0;
	Audio_Detener();
	Registro("Partida terminada desde la pausa");
	Resultados();
	return 1;
}

static void Menu_Fin(void) {
	cursor = 0;
	Pantalla(GH_PANT_FIN, cursor, 2, 0);
	Ir_A(E_MENU_FIN);
}

void Juego_Iniciar(uint8_t cancion_inicial) {
	Buscar_SD();
	cancion = (cancion_inicial < Total()) ? cancion_inicial : 0;
#if JUEGO_INICIAR_EN_MENU
	//Menu_Canciones();
	////////////////////////////
	Menu_Inicio();
	///////////////////////
#else
	Jugar(cancion);
#endif
}

/* Botones en los menús: flechas mueven, A (verde) o Start eligen. Devuelve 1 si se eligió. */
static uint8_t Navegar(uint8_t cuantas, uint8_t modo) {
	uint8_t antes = cursor;
	Control_Evento ev;
	uint8_t elegido = 0;
	while (Controles_Siguiente(&ev)) {
		if (!ev.presionado || elegido) continue;
		if (ev.carril == CONTROL_ARRIBA && cuantas) {
			cursor = (uint8_t)((cursor + cuantas - 1) % cuantas);
		} else if (ev.carril == CONTROL_ABAJO && cuantas) {
			cursor = (uint8_t)((cursor + 1) % cuantas);
		} else if (ev.carril == 0 || ev.carril == CONTROL_START) {
			elegido = 1;
		}
	}
	if (cursor != antes) {
		if (modo == GH_PANT_CANCIONES) Mostrar_Canciones(0);
		else                           Pantalla(modo, cursor, cuantas, 0);
	}
	return elegido;
}

void Juego_Actualizar(void) {
	uint32_t ahora = HAL_GetTick();
	//uint8_t reenviar = (estado == E_MENU_CANCIONES || estado == E_MENU_FIN) && (ahora - t_reenvio >= JUEGO_REENVIO_MS);
	/////////////////////////////
	uint8_t reenviar = (estado == E_INICIO || estado == E_MENU_CANCIONES || estado == E_MENU_FIN) && (ahora - t_reenvio >= JUEGO_REENVIO_MS);
	////////////////////////
	if (reenviar) t_reenvio = ahora;

	switch (estado) {
	/////////////////////////////
	case E_INICIO: {
	    /* Reenviar BEAT HERO si corresponde */
	    if (reenviar) {
	        Pantalla(GH_PANT_INICIO, 0, 0, 0);
	    }

	    /* Esperar Start de cualquiera de los jugadores */
	    Control_Evento ev;

	    while (Controles_Siguiente(&ev)) {

	        if (ev.presionado && ev.carril == CONTROL_START) {

	            /* Pasar al menu de canciones */
	            Menu_Canciones();
	            break;
	        }
	    }

	    break;
	}
	/////////////////////////////

		case E_MENU_CANCIONES:
			if (!Total() && ahora - t_buscar_sd >= JUEGO_BUSCAR_SD_MS) {   /* ¿ya metieron la tarjeta? */
				Buscar_SD();
				cursor = 0; primera = 0; reenviar = 1;
			}
			if (reenviar) Mostrar_Canciones(1);
			if (Navegar(Total(), GH_PANT_CANCIONES)) Jugar(cursor);
			break;

		case E_CUENTA: {
			Descartar_Botones();
			uint32_t paso = (ahora - t_estado) / JUEGO_CUENTA_PASO_MS;
			uint8_t n = (paso >= JUEGO_CUENTA_DESDE) ? 0 : (uint8_t)(JUEGO_CUENTA_DESDE - paso);
			if (n != cuenta) {
				cuenta = n;
				Enlace_Enviar(ENLACE_AMBOS, GH_PKT_CUENTA, n, 0, 0, 0, 0);
				if (!n) Ir_A(E_ENTRADA);
			}
			break;
		}

		case E_ENTRADA: {
			Descartar_Botones();
			uint32_t pasado = ahora - t_estado;
			if (pasado >= JUEGO_ENTRADA_MS) Arrancar_Audio();
			else Beats_Previa(JUEGO_ENTRADA_MS - pasado);   /* las notas del principio empiezan a caer */
			break;
		}

		case E_JUGANDO:
			Beats_Actualizar();                 /* notas hacia las pantallas */
			Juez_Actualizar();                  /* botones, puntos, mute, pausa */
			if (Atender_Pausa()) break;
			if (!Music_IsPlaying() && !Juez_EnPausa()) Ir_A(E_TERMINANDO);
			break;

		case E_TERMINANDO:                      /* deja que el juez califique las últimas notas */
			Juez_Actualizar();
			if (Atender_Pausa()) break;
			if (Juez_EnPausa()) t_estado = HAL_GetTick();   /* en pausa no corre la espera */
			else if (HAL_GetTick() - t_estado >= JUEGO_ESPERA_FIN_MS) Resultados();
			break;

		case E_RESULTADOS:
			if (Navegar(0, GH_PANT_RESULTADOS) || HAL_GetTick() - t_estado >= JUEGO_RESULTADOS_MS) Menu_Fin();
			break;

		case E_MENU_FIN:
			if (reenviar) Pantalla(GH_PANT_FIN, cursor, 2, 0);
			if (Navegar(2, GH_PANT_FIN)) {
				if (cursor == 0) Jugar(cancion);     /* REVANCHA: la misma canción otra vez */
				else             Menu_Inicio();      /* SALIR: al menú principal */
			}
			break;
	}
}
