/*
 * gh_pista.h
 * Pista del Guitar Hero con SCROLL POR HARDWARE de la ILI9341.
 *
 * Cómo funciona:
 *   - La pantalla se parte en: 0..19 fija (puntos), 20..261 se desplaza (la pista), 262..319 fija (botones).
 *   - Toda la pista baja a GH_VELOCIDAD_PX_S moviendo el registro de scroll: no se redibuja ninguna nota
 *     para moverla. En cada paso solo se pinta la franja nueva que entra por arriba (unos px de alto).
 *     Por eso aguanta muchas notas sin ponerse lenta.
 *   - Cada nota sabe a qué hora (HAL_GetTick) debe golpearse: en ese momento queda encima de su botón
 *     (se mete detrás de los botones y desaparece).
 *
 * Esta pantalla NO juzga: MusicaGame (el maestro) decide acierto / fallo y se lo dice por SPI.
 *
 * Uso (main.c):
 *   GH_Pista_Iniciar();                         después de LCD_Init()
 *   GH_Pista_Paquete(pk.b, pk.t_ms);            por cada paquete SPI de MusicaGame
 *   GH_Pista_Actualizar();                      en el while(1), lo más seguido posible
 */
#ifndef INC_GH_PISTA_H_
#define INC_GH_PISTA_H_

#include <stdint.h>

#define GH_NOTAS_POR_CARRIL   16    /* notas guardadas por carril (en pantalla caben ~6) */
#define GH_SCROLL_INVERTIDO   0     /* 1 si en tu pantalla las notas suben en vez de bajar */

void GH_Pista_Iniciar(void);
void GH_Pista_Limpiar(void);                                    /* borra todas las notas */
void GH_Pista_AgregarNota(uint8_t carril, uint8_t id, uint32_t t_golpe_ms, uint16_t largo_ms);
void GH_Pista_Paquete(const uint8_t *pkt, uint32_t t_rx_ms);    /* paquete de gh_protocolo.h */
void GH_Pista_Actualizar(void);
void GH_Pista_Desactivar(void);     /* un menú toma la pantalla; la pista vuelve con el siguiente GH_PKT_CANCION */
uint8_t GH_Pista_Activa(void);

/* Diagnóstico (Live Expressions) */
extern volatile uint32_t pista_aciertos, pista_fallos, pista_descartadas;

#endif /* INC_GH_PISTA_H_ */
