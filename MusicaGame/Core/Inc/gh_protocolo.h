/*
 * gh_protocolo.h  (versión 4: puntos, pausa, resultados y menús)
 * Protocolo SPI MusicaGame (maestro) -> pantallas (esclavos, un STM + ILI9341 por jugador).
 * ESTE MISMO ARCHIVO va en MusicaGame y en el proyecto ILI9341 (copiarlo igual en los dos).
 *
 * Quién hace qué:
 *   ESP32 --UART--> MusicaGame: recibe los botones, lleva el reloj, juzga acierto/fallo, mutea la pista.
 *   MusicaGame --SPI--> pantalla de cada jugador: le dice qué dibujar. La pantalla no juzga nada.
 *
 * Conexión (SPI2 en las dos Nucleo):
 *   MusicaGame PB13 (SCK)  ---> PB13 de cada pantalla
 *   MusicaGame PB15 (MOSI) ---> PB15 de cada pantalla
 *   MusicaGame PB12 (CS 1) ---> PB12 de la pantalla del jugador 1
 *   MusicaGame PB14 (CS 2) ---> PB12 de la pantalla del jugador 2
 *   GND común. SPI modo 0, 8 bits, MSB primero, 1.25 MHz. MISO no se usa.
 *
 * Paquete: 8 bytes con CS en bajo
 *   [0] GH_PKT_SYNC  [1] tipo  [2..6] datos  [7] checksum = XOR de [1]..[6]
 */
#ifndef INC_GH_PROTOCOLO_H_
#define INC_GH_PROTOCOLO_H_

#include <stdint.h>

#define GH_PKT_LEN   8
#define GH_PKT_SYNC  0xA5

/* ---------- Tipos de paquete ---------- */

/* Empieza (o vuelve a empezar) la canción: borrar todas las notas.
 *   [2] número de canción   [3..4] bpm */
#define GH_PKT_CANCION  0x01

/* Nota nueva.
 *   [2] (id << 2) | carril     id 0..63 (se repite), carril 0 verde, 1 rojo, 2 amarillo, 3 azul
 *   [3..4] faltan_ms           ms que faltan para que la nota llegue al botón (queda encima del botón, y = 262)
 *   [5..6] largo_ms            0 = normal, >0 = nota larga */
#define GH_PKT_NOTA     0x02

/* La música se detuvo: borrar las notas. */
#define GH_PKT_ALTO     0x03

/* Lo que decidió el juez sobre una nota.
 *   [2] (id << 2) | carril     [3] GH_EVT_*     [4] GH_CAL_* (qué tan a tiempo, en aciertos) */
#define GH_PKT_EVENTO   0x04

/* Estado de los botones del jugador (para dibujarlos).
 *   [2] presionados (bit 0 verde ... bit 3 azul)
 *   [3] sosteniendo una nota larga (mismos bits) */
#define GH_PKT_BOTONES  0x05

/* Puntos del jugador (después de cada acierto o fallo).
 *   [2..5] puntos (uint32, byte bajo primero)   [6] GH_CAL_* del último golpe */
#define GH_PKT_PUNTOS   0x06

/* Pausa.  [2] 1 = pausado, 0 = sigue   [3] jugador que la puso (1 o 2)
 * En pausa la pista se congela; al quitarla sigue exactamente donde estaba. */
#define GH_PKT_PAUSA    0x07

/* Cambia lo que muestra la pantalla (la pista se detiene mientras hay un menú).
 *   [2] GH_PANT_*   [3] opción marcada   [4] cuántas opciones
 *   [5] (canciones) primera canción visible: se ven GH_MENU_FILAS a partir de ella (la lista se recorre)
 *   [5] (resultados) ganador: 0 empate, 1 o 2      [6] (resultados) jugador de esta pantalla: 1 o 2
 * La pista vuelve sola con el siguiente GH_PKT_CANCION. */
#define GH_PKT_PANTALLA 0x08
#define GH_PANT_CANCIONES   1     /* lista de canciones (textos con GH_PKT_TEXTO) */
#define GH_PANT_RESULTADOS  2     /* ganador y puntos (GH_PKT_RESULTADO antes) */
#define GH_PANT_FIN         3     /* "SELECCIONAR CANCION" / "REVANCHA" */
#define GH_MENU_FILAS       6     /* canciones visibles a la vez */

/* Un pedazo de texto (títulos de canciones).  [2] fila visible 0..GH_MENU_FILAS-1   [3] posición   [4..6] 3 letras (0 = fin)
 * Solo se mandan los títulos de las filas que se ven; al recorrer la lista se vuelven a mandar. */
#define GH_PKT_TEXTO    0x09
#define GH_TEXTO_LINEAS 8
#define GH_TEXTO_LARGO  30

/* Puntos para la pantalla de resultados.  [2] 1 = los de esta pantalla, 2 = los del rival   [3..6] puntos */
#define GH_PKT_RESULTADO 0x0A

/* Cuenta atrás antes de la canción.  [2] número a mostrar (3, 2, 1); 0 = se acabó, la pista vuelve a correr.
 * Mientras dura, la pista está quieta (sin notas). Llega después del GH_PKT_CANCION. */
#define GH_PKT_CUENTA   0x0B

/* Eventos del juez */
#define GH_EVT_ACIERTO    1   /* nota normal atinada: desaparece */
#define GH_EVT_SOSTENIDA  2   /* nota larga atinada: cabeza desaparece, cola encendida */
#define GH_EVT_SOLTADA    3   /* soltó la nota larga antes de tiempo: cola apagada */
#define GH_EVT_COMPLETA   4   /* aguantó toda la nota larga */
#define GH_EVT_FALLO      5   /* no la tocó */

/* Calificación de un golpe y sus puntos (MusicaGame decide los márgenes, ver juez.h) */
#define GH_CAL_FALLO      0   /*   0 puntos */
#define GH_CAL_JUSTO      1   /*  10 puntos */
#define GH_CAL_MEDIO      2   /*  50 puntos */
#define GH_CAL_PERFECTO   3   /* 100 puntos */

/* Leer / armar el byte (id, carril) */
#define GH_ID_CARRIL(id, c)   ((uint8_t)((((id) & 0x3F) << 2) | ((c) & 0x03)))
#define GH_CARRIL_DE(b)       ((uint8_t)((b) & 0x03))
#define GH_ID_DE(b)           ((uint8_t)((b) >> 2))

/* ---------- Tiempos compartidos ---------- */
#define GH_VELOCIDAD_PX_S   250   /* px por segundo que bajan las notas */
#define GH_HUD_ALTO         20    /* franja fija de arriba con los puntos (no se desplaza) */
#define GH_CAIDA_PX         (262 - GH_HUD_ALTO + 48)   /* de nacer escondida bajo los puntos (y = 20 - 48) al botón (y = 262) */
#define GH_ANTICIPACION_MS  ((GH_CAIDA_PX * 1000) / GH_VELOCIDAD_PX_S)   /* 1160 ms */

static inline uint8_t GH_Checksum(const uint8_t *p) {
	return (uint8_t)(p[1] ^ p[2] ^ p[3] ^ p[4] ^ p[5] ^ p[6]);
}

#endif /* INC_GH_PROTOCOLO_H_ */
