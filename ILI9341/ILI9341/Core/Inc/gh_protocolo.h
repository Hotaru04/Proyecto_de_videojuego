/*
 * gh_protocolo.h
 * Protocolo SPI MusicaGame (maestro) -> ILI9341 (esclavo).
 * ESTE MISMO ARCHIVO se copia tal cual al proyecto ILI9341 para que los dos lados usen los mismos números.
 *
 * Conexión (las dos son NUCLEO-F446RE, SPI2 en ambas):
 *   MusicaGame PB13 (SPI2_SCK)  --->  ILI9341 PB13 (SPI2_SCK)
 *   MusicaGame PB15 (SPI2_MOSI) --->  ILI9341 PB15 (SPI2_MOSI)
 *   MusicaGame PB12 (CS)        --->  ILI9341 PB12 (SPI2_NSS, hardware, entrada)
 *   GND                         ---   GND
 *   SPI modo 0 (CPOL 0, CPHA 0), 8 bits, MSB primero, 1.25 MHz. MISO no se usa.
 *
 * Cada paquete mide GH_PKT_LEN (8) bytes, con CS en bajo durante el paquete:
 *   [0] GH_PKT_SYNC (0xA5)
 *   [1] tipo (GH_PKT_*)
 *   [2..6] datos (según el tipo)
 *   [7] checksum = XOR de los bytes [1]..[6]
 *
 * IDEA: MusicaGame avisa cada nota GH_ANTICIPACION_MS ANTES de que suene, y dice cuánto falta (faltan_ms).
 * La pantalla la pone donde le toca:  y = GH_BOTON_Y - faltan_ms * GH_VELOCIDAD_PX_S / 1000
 * y la mueve según el TIEMPO (HAL_GetTick), no por cuadro. Así llega al botón justo cuando suena.
 *
 * Recepción en el ILI9341 (CubeMX: SPI2 "Receive Only Slave", NSS "Hardware NSS Input",
 * Data Size 8 bits, CPOL Low, CPHA 1 Edge, interrupción global de SPI2 activada):
 *
 *   uint8_t rx[GH_PKT_LEN];
 *   HAL_SPI_Receive_IT(&hspi2, rx, GH_PKT_LEN);            // una vez, en USER CODE 2
 *
 *   void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *h) {
 *     if (h->Instance == SPI2) {
 *       if (rx[0] == GH_PKT_SYNC && rx[7] == GH_Checksum(rx)) {
 *         // copiar rx a una cola; el while(1) la procesa (no dibujar aquí)
 *       }
 *       HAL_SPI_Receive_IT(&hspi2, rx, GH_PKT_LEN);         // volver a escuchar
 *     }
 *   }
 *   Si llegan paquetes inválidos seguidos (empezó a escuchar a medio paquete):
 *   HAL_SPI_Abort(&hspi2) y volver a llamar HAL_SPI_Receive_IT.
 */
#ifndef INC_GH_PROTOCOLO_H_
#define INC_GH_PROTOCOLO_H_

#include <stdint.h>

#define GH_PKT_LEN   8
#define GH_PKT_SYNC  0xA5

/* ---------- Tipos de paquete ---------- */

/* Empieza (o vuelve a empezar) una canción: la pantalla borra las notas que tenga.
 *   [2] número de canción (índice de song_table)
 *   [3] bpm, byte bajo   [4] bpm, byte alto
 *   [5] [6] 0 */
#define GH_PKT_CANCION  0x01

/* Una nota (o acorde) que hay que tocar.
 *   [2] (jugador << 4) | carriles      jugador 1 o 2; carriles: bit0 verde, bit1 rojo, bit2 amarillo, bit3 azul
 *                                      (más de un bit = acorde: presionar varios botones a la vez)
 *   [3] faltan_ms, byte bajo    [4] faltan_ms, byte alto    -> ms que faltan para que llegue al botón
 *   [5] largo_ms,  byte bajo    [6] largo_ms,  byte alto    -> 0 = nota normal; >0 = nota larga (sostener)
 *   Largo de la cola en px = largo_ms * GH_VELOCIDAD_PX_S / 1000 */
#define GH_PKT_NOTA     0x02

/* La música se detuvo: la pantalla borra las notas. Datos en 0. */
#define GH_PKT_ALTO     0x03

#define GH_CARRIL_VERDE     0x01
#define GH_CARRIL_ROJO      0x02
#define GH_CARRIL_AMARILLO  0x04
#define GH_CARRIL_AZUL      0x08

/* ---------- Tiempos compartidos (cambiar AQUÍ y copiar a los dos proyectos) ---------- */
#define GH_VELOCIDAD_PX_S   250   /* px por segundo que bajan las notas (la demo actual: 4 px cada 16 ms) */
#define GH_CAIDA_PX         310   /* de y = -48 (nace arriba) a y = 262 (fila de botones) */
#define GH_ANTICIPACION_MS  ((GH_CAIDA_PX * 1000) / GH_VELOCIDAD_PX_S)   /* 1240 ms */

/* Checksum de un paquete (bytes 1..6) */
static inline uint8_t GH_Checksum(const uint8_t *p) {
	return (uint8_t)(p[1] ^ p[2] ^ p[3] ^ p[4] ^ p[5] ^ p[6]);
}

#endif /* INC_GH_PROTOCOLO_H_ */
