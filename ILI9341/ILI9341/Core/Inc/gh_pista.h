/*
 * gh_pista.h
 * Pista del juego con scroll por hardware de la ILI9341.
 */
#ifndef INC_GH_PISTA_H_
#define INC_GH_PISTA_H_

#include <stdint.h>

#define GH_NOTAS_POR_CARRIL   16
#define GH_SCROLL_INVERTIDO   0

void GH_Pista_Iniciar(void);
void GH_Pista_Limpiar(void);
void GH_Pista_AgregarNota(uint8_t carril, uint8_t id, uint32_t t_golpe_ms, uint16_t largo_ms);
void GH_Pista_Paquete(const uint8_t *pkt, uint32_t t_rx_ms);
void GH_Pista_Actualizar(void);
void GH_Pista_Desactivar(void);
uint8_t GH_Pista_Activa(void);

extern volatile uint32_t pista_aciertos, pista_fallos, pista_descartadas;

#endif /* INC_GH_PISTA_H_ */
