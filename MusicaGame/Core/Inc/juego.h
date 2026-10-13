/*
 * juego.h
 * Flujo del juego (MusicaGame es quien manda):
 *
 *   INICIO --Start--> MENÚ DE CANCIONES --A--> CUENTA 3-2-1 --> ENTRADA --> JUGANDO --termina--> RESULTADOS
 *     ^                                                              |                                 |
 *     |                                                     pausa: SALIR AL MENU ----------------------+
 *     |                                                                                                v
 *     +--------------------------------- "SALIR" ------------------------------------------- MENÚ FINAL
 *                                        "REVANCHA" -> vuelve a la CUENTA con la misma canción
 *
 * CUENTA:  3, 2, 1 grandes en las pantallas, la pista quieta.
 * ENTRADA: las primeras notas ya caen pero el audio todavía no suena (así la primera nota no sale de golpe).
 *
 * En los menús cualquiera de los dos jugadores mueve el cursor (flechas arriba / abajo del control)
 * y elige con A (verde) o Start. Las dos pantallas muestran lo mismo.
 * La canción suena UNA vez; el ganador es el que tiene más puntos.
 */
#ifndef INC_JUEGO_H_
#define INC_JUEGO_H_

#include <stdint.h>
#include "gh_protocolo.h"

#define JUEGO_INICIAR_EN_MENU   1      /* 0 = al encender arranca directo song_index (para probar rápido) */
#define JUEGO_BUSCAR_SD_MS      3000   /* sin canciones: cada cuánto se vuelve a leer la microSD */
#define JUEGO_CUENTA_DESDE      3      /* 3, 2, 1 */
#define JUEGO_CUENTA_PASO_MS    1000   /* cuánto dura cada número */
#define JUEGO_ENTRADA_MS        GH_ANTICIPACION_MS   /* notas cayendo antes del audio (lo que tarda una nota en bajar) */
#define JUEGO_REENVIO_MS        1000   /* en los menús se vuelve a mandar la pantalla (por si una pantalla arrancó tarde) */
#define JUEGO_ESPERA_FIN_MS     1500   /* después de la última nota, antes de mostrar resultados */
#define JUEGO_RESULTADOS_MS     4000   /* resultados en pantalla antes del menú (o antes, con A) */

void Juego_Iniciar(uint8_t cancion_inicial);
void Juego_Actualizar(void);           /* en el while(1) */
const char *Juego_Estado(void);        /* para la bitácora */

#endif /* INC_JUEGO_H_ */
