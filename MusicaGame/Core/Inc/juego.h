/*
 * juego.h
 * Flujo del juego (lo controla MusicaGame):
 *
 *   INICIO -> MENÚ DE CANCIONES -> CUENTA 3-2-1 -> ENTRADA -> JUGANDO -> RESULTADOS -> MENÚ FINAL
 *   MENÚ FINAL: REVANCHA (otra vez la misma canción) o SALIR (a INICIO).
 *   Pausa -> SALIR AL MENU: termina la partida y pasa a RESULTADOS.
 *
 * ENTRADA: las primeras notas ya caen pero el audio todavía no suena.
 */
#ifndef INC_JUEGO_H_
#define INC_JUEGO_H_

#include <stdint.h>
#include "gh_protocolo.h"

#define JUEGO_INICIAR_EN_MENU   1      /* 0 = arranca directo la canción song_index */
#define JUEGO_BUSCAR_SD_MS      3000   /* reintento si no hay canciones */
#define JUEGO_CUENTA_DESDE      3
#define JUEGO_CUENTA_PASO_MS    1000
#define JUEGO_ENTRADA_MS        GH_ANTICIPACION_MS
#define JUEGO_REENVIO_MS        1000
#define JUEGO_ESPERA_FIN_MS     1500   /* después de la última nota */
#define JUEGO_RESULTADOS_MS     4000

void Juego_Iniciar(uint8_t cancion_inicial);
void Juego_Actualizar(void);
const char *Juego_Estado(void);

#endif /* INC_JUEGO_H_ */
