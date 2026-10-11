#include "gh_notas.h"
#include "ili9341.h"

/* Restaura el fondo PERO se salta matemáticamente la zona de los botones [262, 310] */
void GH_RestaurarFondo(int16_t x, int16_t y, int16_t w, int16_t h) {
    /* Zona Arriba del botón */
    int16_t y_above = y;
    int16_t h_above = h;
    if (y_above < 0) { h_above += y_above; y_above = 0; }

    if (y_above < GH_BOTON_Y && h_above > 0) {
        int16_t h_draw = h_above;
        if (y_above + h_draw > GH_BOTON_Y) h_draw = GH_BOTON_Y - y_above;
        FillRectFast(x, y_above, w, h_draw, GH_COLOR_FONDO);
    }

    /* Zona Debajo del botón */
    int16_t y_below = y;
    int16_t h_below = h;
    if (y_below < GH_BOTON_Y + GH_SPR_H) {
        int16_t diff = (GH_BOTON_Y + GH_SPR_H) - y_below;
        y_below += diff;
        h_below -= diff;
    }
    if (y_below < GH_PANTALLA_H && h_below > 0) {
        int16_t h_draw = h_below;
        if (y_below + h_draw > GH_PANTALLA_H) h_draw = GH_PANTALLA_H - y_below;
        FillRectFast(x, y_below, w, h_draw, GH_COLOR_FONDO);
    }
}

/* Pinta la estela saltándose la zona del botón */
static void GH_PintarCola(uint8_t carril, int16_t y, int16_t h, uint8_t encendida) {
    const uint8_t *tile = encendida ? gh_hold_tail_on[carril] : gh_hold_tail[carril];
    int16_t x = GH_CARRIL_X(carril);

    int16_t y_above = y, h_above = h;
    if (y_above < 0) { h_above += y_above; y_above = 0; }

    if (y_above < GH_BOTON_Y && h_above > 0) {
        int16_t h_draw = h_above;
        if (y_above + h_draw > GH_BOTON_Y) h_draw = GH_BOTON_Y - y_above;
        int16_t curr_y = y_above, curr_h = h_draw;
        while (curr_h > 0) {
            int16_t fila = curr_y % GH_COLA_TILE_H;
            int16_t n = GH_COLA_TILE_H - fila;
            if (n > curr_h) n = curr_h;
            LCD_BitmapFast(x, curr_y, GH_SPR_W, n, tile + fila * GH_SPR_W * 2);
            curr_y += n; curr_h -= n;
        }
    }

    int16_t y_below = y, h_below = h;
    if (y_below < GH_BOTON_Y + GH_SPR_H) {
        int16_t diff = (GH_BOTON_Y + GH_SPR_H) - y_below;
        y_below += diff;
        h_below -= diff;
    }
    if (y_below < GH_PANTALLA_H && h_below > 0) {
        int16_t h_draw = h_below;
        if (y_below + h_draw > GH_PANTALLA_H) h_draw = GH_PANTALLA_H - y_below;
        int16_t curr_y = y_below, curr_h = h_draw;
        while (curr_h > 0) {
            int16_t fila = curr_y % GH_COLA_TILE_H;
            int16_t n = GH_COLA_TILE_H - fila;
            if (n > curr_h) n = curr_h;
            LCD_BitmapFast(x, curr_y, GH_SPR_W, n, tile + fila * GH_SPR_W * 2);
            curr_y += n; curr_h -= n;
        }
    }
}

/* Pinta la cabeza de la nota saltándose el botón */
static void GH_SpriteRecortado(int16_t x, int16_t y, const uint8_t *bmp) {
    int16_t y_above = y, h_above = GH_SPR_H, offset_above = 0;

    if (y_above < 0) { offset_above = -y_above; h_above += y_above; y_above = 0; }

    if (y_above < GH_BOTON_Y && h_above > 0) {
        int16_t h_draw = h_above;
        if (y_above + h_draw > GH_BOTON_Y) h_draw = GH_BOTON_Y - y_above;
        LCD_BitmapFast(x, y_above, GH_SPR_W, h_draw, bmp + offset_above * GH_SPR_W * 2);
    }

    int16_t y_below = y, h_below = GH_SPR_H, offset_below = 0;
    if (y_below < GH_BOTON_Y + GH_SPR_H) {
        int16_t diff = (GH_BOTON_Y + GH_SPR_H) - y_below;
        y_below += diff;
        h_below -= diff;
        offset_below = diff;
    }
    if (y_below < GH_PANTALLA_H && h_below > 0) {
        int16_t h_draw = h_below;
        if (y_below + h_draw > GH_PANTALLA_H) h_draw = GH_PANTALLA_H - y_below;
        LCD_BitmapFast(x, y_below, GH_SPR_W, h_draw, bmp + offset_below * GH_SPR_W * 2);
    }
}

void GH_DibujarBoton(uint8_t carril, uint8_t estado) {
    LCD_SpriteFast(GH_CARRIL_X(carril), GH_BOTON_Y, GH_SPR_W, GH_SPR_H,
            gh_btn_sheet[carril], GH_BTN_CUADROS, estado, 0, 0, 0);
}

void GH_DibujarBotones(void) {
    for (uint8_t c = 0; c < GH_NUM_CARRILES; c++) GH_DibujarBoton(c, GH_BTN_REPOSO);
}

void GH_NotaIniciar(GH_Nota *n, uint8_t carril) { GH_NotaLargaIniciar(n, carril, 0); }

void GH_NotaLargaIniciar(GH_Nota *n, uint8_t carril, int16_t largo) {
    n->carril = carril;
    n->y = GH_NOTA_Y_INICIO;
    n->largo = largo;
    n->activa = 1;
    n->cabezaLlego = 0;
    n->atrapada = 0;     /* [GH] si no, una nota nueva hereda el 'atrapada' de la anterior */
}

void GH_ColaRedibujar(const GH_Nota *n, uint8_t encendida) {
    if (!n->activa || n->largo == 0) return;
    int16_t arriba = n->y - n->largo;
    int16_t abajo = n->y + GH_SPR_H / 2;
    GH_PintarCola(n->carril, arriba, abajo - arriba, encendida);
}

uint8_t GH_NotaMover(GH_Nota *n, int16_t dy) {
    if (!n->activa) return GH_EV_NADA;
    int16_t x = GH_CARRIL_X(n->carril);
    int16_t y_ant = n->y;
    int16_t y_nueva = y_ant + dy;
    uint8_t evento = GH_EV_NADA;
    n->y = y_nueva;

    /* ---- Cabeza ---- */
    if (!n->cabezaLlego && y_nueva >= GH_BOTON_Y) {
        n->cabezaLlego = 1;
        evento = GH_EV_LLEGO;
    }

    /* --- Dibujar la nota bajando --- */
    if (y_nueva < GH_PANTALLA_H) {
        if (n->largo == 0) GH_RestaurarFondo(x, y_ant, GH_SPR_W, dy);
        else               GH_PintarCola(n->carril, y_ant, dy, 0);
        GH_SpriteRecortado(x, y_nueva, n->largo ? gh_hold_head[n->carril] : gh_note[n->carril]);
    }
    /* Si la nota corta sale definitivamente de la pantalla (Fallo) */
    else if (n->largo == 0) {
        GH_RestaurarFondo(x, y_ant, GH_SPR_W, GH_PANTALLA_H - y_ant);
        n->activa = 0;
        return GH_EV_NADA;
    }

    /* ---- Fin de cola ---- */
    if (n->largo > 0) {
        int16_t fin_ant = y_ant - n->largo;
        int16_t fin_nuevo = y_nueva - n->largo;
        if (fin_nuevo >= GH_PANTALLA_H) {
            if (fin_ant < GH_PANTALLA_H) {
                GH_RestaurarFondo(x, fin_ant, GH_SPR_W, GH_PANTALLA_H - fin_ant);
            }
            n->activa = 0;
            return GH_EV_FIN_COLA;
        } else if (fin_nuevo >= 0) {
            GH_RestaurarFondo(x, fin_ant, GH_SPR_W, dy);
        }
    }
    return evento;
}
