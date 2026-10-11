/*
 * juego.h
 * Flujo del juego (MusicaGame es quien manda):
 *
 *   MENÚ DE CANCIONES --A / Start--> JUGANDO --termina la canción--> RESULTADOS --A / 4 s--> MENÚ FINAL
 *          ^                                                                                   |
 *          +---------------------------- "SELECCIONAR CANCION" ---------------------------------+
 *                                         "REVANCHA" -> vuelve a JUGANDO la misma canción
 *
 * En los menús cualquiera de los dos jugadores mueve el cursor (flechas arriba / abajo del control)
 * y elige con A (verde) o Start. Las dos pantallas muestran lo mismo.
 * La canción suena UNA vez; el ganador es el que tiene más puntos.
 */
#ifndef INC_JUEGO_H_
#define INC_JUEGO_H_

#include <stdint.h>

#define JUEGO_INICIAR_EN_MENU   1      /* 0 = al encender arranca directo song_index (para probar rápido) */
#define JUEGO_USAR_SD           1      /* 1 = canciones de la microSD (si no hay tarjeta, usa las de la flash) */
#define JUEGO_ESPERA_FIN_MS     1500   /* después de la última nota, antes de mostrar resultados */
#define JUEGO_RESULTADOS_MS     4000   /* resultados en pantalla antes del menú (o antes, con A) */

void Juego_Iniciar(uint8_t cancion_inicial);
void Juego_Actualizar(void);           /* en el while(1) */
const char *Juego_Estado(void);        /* para la bitácora */

#endif /* INC_JUEGO_H_ */
