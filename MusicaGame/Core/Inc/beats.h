/*
 * beats.h
 * Convierte las pistas de cada jugador (canal 1 = jugador 1, canal 2 = jugador 2) en notas del juego
 * (carril verde/rojo/amarillo/azul, notas largas, acordes) y las manda por SPI a la pantalla
 * GH_ANTICIPACION_MS antes de que suenen. Ver gh_protocolo.h.
 *
 * Uso (main.c):
 *   Enlace_Init();                    una vez
 *   Beats_Iniciar(cur, indice);       cada vez que arranca una canción
 *   Beats_Previa(ms);                 mientras el audio todavía no arranca (entrada de la canción)
 *   Beats_Actualizar();               en el while(1), lo más seguido posible
 */
#ifndef INC_BEATS_H_
#define INC_BEATS_H_

#include <stdint.h>
#include "songs.h"

/* ---------- Ajustes de cómo se "tabla" la canción ---------- */
#define BEATS_SEPARACION_MS   200   /* mínimo entre dos notas del mismo jugador (una nota mide 48 px = 192 ms) */
#define BEATS_LARGA_MIN_MS    400   /* una nota que dura esto o más se vuelve nota larga */
#define BEATS_ACORDE_NOTAS    255   /* sin acordes: una nota por botón (con 3, p. ej., 3 notas a la vez piden 2 botones) */
#define BEATS_LATENCIA_MS     12    /* el audio sale ~1.5 mitades de buffer después de calcularse */

#define BEATS_MAX_PISTAS      8     /* pistas máximas por jugador */

void Beats_Iniciar(const SongEntry *e, uint8_t indice);   /* e = 0 -> nada que mandar */
void Beats_Detener(void);                                 /* manda GH_PKT_ALTO */
void Beats_Actualizar(void);
void Beats_Previa(uint32_t faltan_ms);                   /* antes del audio: la canción empieza dentro de faltan_ms */

/* Diagnóstico (Live Expressions) */
extern volatile uint32_t beats_enviadas[2];   /* notas mandadas por jugador */
extern volatile uint32_t beats_tarde;         /* notas que ya no alcanzaron a mandarse a tiempo */

#endif /* INC_BEATS_H_ */
