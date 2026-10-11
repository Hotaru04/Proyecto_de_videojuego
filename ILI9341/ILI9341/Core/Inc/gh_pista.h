/*
 * gh_pista.h
 * Pista del Guitar Hero con SCROLL POR HARDWARE de la ILI9341.
 *
 * Cómo funciona:
 *   - La pantalla se parte en: filas 0..261 que se desplazan (la pista) y 262..319 fijas (los botones).
 *   - Toda la pista baja a GH_VELOCIDAD_PX_S moviendo el registro de scroll: no se redibuja ninguna nota
 *     para moverla. En cada paso solo se pinta la franja nueva que entra por arriba (unos px de alto).
 *     Por eso aguanta muchas notas sin ponerse lenta.
 *   - Cada nota sabe a qué hora (HAL_GetTick) debe llegar al botón. Las notas desaparecen detrás de los
 *     botones (ya no pasan por debajo).
 *
 * Uso (main.c):
 *   GH_Pista_Iniciar();                         después de LCD_Init()
 *   GH_Pista_Paquete(pk.b, pk.t_ms);            por cada paquete SPI de MusicaGame
 *   GH_Pista_Presionar(carril, t_ms);           cuando un botón se PRESIONA (flanco)
 *   GH_Pista_Actualizar(botones);               en el while(1), con el estado actual de los 4 botones
 */
#ifndef INC_GH_PISTA_H_
#define INC_GH_PISTA_H_

#include <stdint.h>

#define GH_JUGADOR_PANTALLA   1     /* qué jugador muestra esta pantalla (los paquetes del otro se ignoran) */
#define GH_VENTANA_ANTES_MS   70    /* se acepta presionar hasta esto antes de que llegue la nota */
#define GH_VENTANA_DESPUES_MS 90    /* ...y hasta esto después */
#define GH_NOTAS_POR_CARRIL   16    /* notas guardadas por carril (en pantalla caben ~6) */
#define GH_SCROLL_INVERTIDO   0     /* 1 si en tu pantalla las notas suben en vez de bajar */

void GH_Pista_Iniciar(void);
void GH_Pista_Limpiar(void);                                    /* borra todas las notas */
void GH_Pista_AgregarNota(uint8_t carril, uint32_t t_golpe_ms, uint16_t largo_ms);
void GH_Pista_Paquete(const uint8_t *pkt, uint32_t t_rx_ms);    /* paquete de gh_protocolo.h */
void GH_Pista_Presionar(uint8_t carril, uint32_t t_ms);
void GH_Pista_Actualizar(const uint8_t botones[4]);

/* Diagnóstico (Live Expressions) */
extern volatile uint32_t pista_aciertos, pista_fallos, pista_descartadas;

#endif /* INC_GH_PISTA_H_ */
