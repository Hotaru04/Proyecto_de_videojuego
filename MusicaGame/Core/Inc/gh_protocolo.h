/*
 * gh_protocolo.h
 * Protocolo SPI MusicaGame -> pantallas. El mismo archivo va en los dos proyectos.
 * Paquete: 8 bytes con CS en bajo
 *   [0] GH_PKT_SYNC  [1] tipo  [2..6] datos  [7] checksum = XOR de [1]..[6]
 */
#ifndef INC_GH_PROTOCOLO_H_
#define INC_GH_PROTOCOLO_H_

#include <stdint.h>

#define GH_PKT_LEN   8
#define GH_PKT_SYNC  0xA5

/* Tipos de paquete */

/* Empieza la canción: borrar las notas.  [2] canción  [3..4] bpm */
#define GH_PKT_CANCION  0x01

/* Nota nueva.  [2] (id << 2) | carril  [3..4] ms para llegar al botón  [5..6] largo en ms (0 = normal) */
#define GH_PKT_NOTA     0x02

/* La música se detuvo: borrar las notas */
#define GH_PKT_ALTO     0x03

/* Lo que decidió el juez.  [2] (id << 2) | carril  [3] GH_EVT_*  [4] GH_CAL_* */
#define GH_PKT_EVENTO   0x04

/* Botones del jugador.  [2] presionados  [3] sosteniendo nota larga */
#define GH_PKT_BOTONES  0x05

/* Puntos del jugador.  [2..5] puntos  [6] GH_CAL_* del último golpe */
#define GH_PKT_PUNTOS   0x06

/* Pausa.  [2] 1 = pausado  [3] jugador que la puso  [4] opción marcada */
#define GH_PKT_PAUSA    0x07
#define GH_PAUSA_CONTINUAR  0
#define GH_PAUSA_SALIR      1     /* termina la partida: resultados y luego REVANCHA / SALIR */
#define GH_PAUSA_OPCIONES   2

/* Cambia la pantalla.  [2] GH_PANT_*  [3] opción marcada  [4] opciones
 *   canciones: [5] primera visible     resultados: [5] ganador  [6] jugador de esta pantalla */
#define GH_PKT_PANTALLA 0x08
#define GH_PANT_CANCIONES   1
#define GH_PANT_RESULTADOS  2
#define GH_PANT_FIN         3
#define GH_PANT_INICIO      4
#define GH_MENU_FILAS       6

/* Título de canción, 3 letras por paquete.  [2] fila  [3] posición  [4..6] letras */
#define GH_PKT_TEXTO    0x09
#define GH_TEXTO_LINEAS 8
#define GH_TEXTO_LARGO  30

/* Puntos para resultados.  [2] 1 = propios, 2 = rival  [3..6] puntos */
#define GH_PKT_RESULTADO 0x0A

/* Cuenta atrás.  [2] número (0 = termina) */
#define GH_PKT_CUENTA   0x0B

/* Eventos del juez */
#define GH_EVT_ACIERTO    1
#define GH_EVT_SOSTENIDA  2
#define GH_EVT_SOLTADA    3
#define GH_EVT_COMPLETA   4
#define GH_EVT_FALLO      5

/* Calificación de un golpe */
#define GH_CAL_FALLO      0
#define GH_CAL_JUSTO      1
#define GH_CAL_MEDIO      2
#define GH_CAL_PERFECTO   3
#define GH_CAL_SOSTENIDA  4

/* Byte (id, carril) */
#define GH_ID_CARRIL(id, c)   ((uint8_t)((((id) & 0x3F) << 2) | ((c) & 0x03)))
#define GH_CARRIL_DE(b)       ((uint8_t)((b) & 0x03))
#define GH_ID_DE(b)           ((uint8_t)((b) >> 2))

/* Tiempos compartidos */
#define GH_VELOCIDAD_PX_S   250
#define GH_HUD_ALTO         20
#define GH_CAIDA_PX         (262 - GH_HUD_ALTO + 48)
#define GH_ANTICIPACION_MS  ((GH_CAIDA_PX * 1000) / GH_VELOCIDAD_PX_S)

static inline uint8_t GH_Checksum(const uint8_t *p) {
	return (uint8_t)(p[1] ^ p[2] ^ p[3] ^ p[4] ^ p[5] ^ p[6]);
}

#endif /* INC_GH_PROTOCOLO_H_ */
