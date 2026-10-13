/*
 * gh_menu.c
 * Dibujo de la lista de canciones, los resultados y el menú final. Ver gh_menu.h.
 * Mientras hay un menú la pista está desactivada y el scroll en su posición normal,
 * así que se dibuja directo en coordenadas de pantalla (240 x 320).
 */
#include "gh_menu.h"
#include "gh_pista.h"
#include "gh_protocolo.h"
#include "ili9341.h"
#include <stdio.h>
#include <string.h>

#include "graficos.h"


#define ANCHO      240
#define ALTO_PANT  320

#define C_FONDO    0x0000
#define C_BLANCO   0xFFFF
#define C_CIAN     0x07FF
#define C_MAGENTA  0xF81F
#define C_AMARILLO 0xFFE0
#define C_VERDE    0x07E0
#define C_ROJO     0xF800
#define C_GRIS     0x7BEF
#define C_MORADO   0x300C            /* fondo de las opciones */


////////////////////////

#define LISTA_X     8
#define LISTA_Y     72
#define LISTA_W   120
#define PASO       30
#define MAX_LETRAS 14


//////////////////////////







//////////////////////////////////////////////////////////////////////////

#define C_AZUL_OSCURO 0x0825         /* NUEVO: fondo para el encabezado */

// Declaramos el acceso al fondo que está en otro archivo
extern const uint16_t fondoT[];

static char     titulos[GH_TEXTO_LINEAS][GH_TEXTO_LARGO + 1];

///////////////////////////////////////////////////////////////

/* Fuentes existentes en font.c */
extern const uint8_t smallFont[1140];
extern const uint16_t bigFont[1520];
//////////////////////////

static char     titulos[GH_TEXTO_LINEAS][GH_TEXTO_LARGO + 1];
static uint32_t pts_mios = 0, pts_rival = 0;
static uint8_t  modo = 0;            /* lo que está dibujado ahora (GH_PANT_*), 0 = nada */
static uint8_t  marcada_dib = 0xFF;
static uint8_t  cuantas_dib = 0;
static uint8_t  primera_dib = 0;     /* primera canción visible dibujada */
static uint8_t  titulos_nuevos = 0;  /* llegó texto distinto al dibujado */

//////////////////////////
/* Prototipos de funciones de dibujo */
static void Dibujar_Preview_Cancion(uint8_t marcada, uint8_t primera);


////////////////////
/* Dimensiones de las zonas donde aparece el título */
#define TXT_LISTA_X   (LISTA_X + 16)
#define TXT_LISTA_W   (LISTA_W - 20)
//////////////////////////////////////////////
/* ============================================
 * CONFIGURACION DEL PREVIEW
 * ============================================ */

/* Posicion y dimensiones del recuadro */
#define PREV_X       136
#define PREV_Y        68
#define PREV_W       100
#define PREV_H       132

/* Dimensiones de la imagen */
#define PREVIEW_W     96
#define PREVIEW_H     96

/* Posicion de la imagen */
#define PREV_IMG_X   138
#define PREV_IMG_Y    86

/* ============================================
 * ZONAS DE TEXTO DESPLAZABLE
 * ============================================ */

#define TXT_PREV_X   (PREV_X + 2)
#define TXT_PREV_W   (PREV_W - 4)
#define TXT_PREV_Y   (PREV_Y + 115)

//////////////////////////////////////////////


#define SCROLL_MS         200u
#define SCROLL_ESPERA_MS 1000u

static uint16_t paso_scroll = 0;
static uint32_t t_scroll = 0;


/* Dibuja únicamente los caracteres que caben en el espacio.
 * Si el texto es largo, muestra una ventana desde 'offset'.
 * Si es corto, lo centra.
 */
static void TextoVentana(const char *s,
                         int x, int y, int ancho,
                         uint16_t color,
                         uint16_t fondo,
                         uint16_t offset)
{
    char buffer[GH_TEXTO_LARGO + 1];

    unsigned int capacidad = (unsigned int)ancho / 8u;
    unsigned int longitud = (unsigned int)strlen(s);

    if (capacidad > GH_TEXTO_LARGO)
        capacidad = GH_TEXTO_LARGO;

    if (capacidad == 0)
        return;

    /* Texto corto: centrado, sin desplazamiento */
    if (longitud <= capacidad) {

        memcpy(buffer, s, longitud);
        buffer[longitud] = '\0';

        int x_centrado = x +
            (ancho - (int)longitud * 8) / 2;

        LCD_Print(buffer, x_centrado, y,
                  1, color, fondo);

        return;
    }

    /* Texto largo: mostrar únicamente la parte visible */
    for (unsigned int i = 0; i < capacidad; i++) {

        unsigned int indice = (unsigned int)offset + i;

        if (indice < longitud)
            buffer[i] = s[indice];
        else
            buffer[i] = ' ';
    }

    buffer[capacidad] = '\0';

    LCD_Print(buffer, x, y, 1, color, fondo);
}


//////////////////////////////////////
static void Opcion_Cancion(int i, int y0,
                           const char *nombre,
                           uint8_t seleccionada)
{
    int y = y0 + i * PASO;

    uint16_t fondo = seleccionada ? C_CIAN : C_MORADO;
    uint16_t letra = seleccionada ? C_FONDO : C_BLANCO;

    FillRectFast(LISTA_X, y, LISTA_W, 24, fondo);

    if (!seleccionada) {
        FillRectFast(LISTA_X, y, LISTA_W, 1, C_MAGENTA);
        FillRectFast(LISTA_X, y + 23, LISTA_W, 1, C_MAGENTA);
    } else {
        LCD_Print(">", LISTA_X + 4, y + 7,
                  1, C_FONDO, fondo);
    }

    /* Nunca dibujar fuera de la zona de la lista */
    TextoVentana(nombre,
                 TXT_LISTA_X, y + 7,
                 TXT_LISTA_W,
                 letra, fondo, 0);
}

////////////////////////////////////


/* ---------- ayudas de dibujo ---------- */
static void Texto(const char *s, int y, uint8_t grande, uint16_t color, uint16_t fondo) {
	int ancho_letra = grande ? 16 : 8;
	int len = (int)strlen(s);
	int x = (ANCHO - len * ancho_letra) / 2;
	if (x < 0) x = 0;
	LCD_Print((char *)s, x, y, grande ? 2 : 1, color, fondo);
}


////////////////////////////////////////////////////

static void TextoCaja(const char *s, int x0, int y, int ancho,
                      uint8_t grande, uint16_t color, uint16_t fondo) {
    int ancho_letra = grande ? 16 : 8;
    int len = (int)strlen(s);
    int x = x0 + (ancho - len * ancho_letra) / 2;
    if (x < x0) x = x0;
    LCD_Print((char *)s, x, y, grande ? 2 : 1, color, fondo);
}



/////////////////////////////////////////////////////





static void Marco(void) {                                     /* borde neón como el del sprite de pausa */
	FillRectFast(0, 0, ANCHO, ALTO_PANT, C_FONDO);
	FillRectFast(0, 0, ANCHO, 3, C_MAGENTA);
	FillRectFast(0, ALTO_PANT - 3, ANCHO, 3, C_MAGENTA);
	FillRectFast(0, 0, 3, ALTO_PANT, C_MAGENTA);
	FillRectFast(ANCHO - 3, 0, 3, ALTO_PANT, C_MAGENTA);
	FillRectFast(6, 6, ANCHO - 12, 1, C_CIAN);
	FillRectFast(6, ALTO_PANT - 7, ANCHO - 12, 1, C_CIAN);
	FillRectFast(6, 6, 1, ALTO_PANT - 12, C_CIAN);
	FillRectFast(ANCHO - 7, 6, 1, ALTO_PANT - 12, C_CIAN);
}

//#define PASO       36                /* separación entre opciones */
//#define LISTA_Y    70                /* primera fila de la lista de canciones */
//#define MAX_LETRAS 26                /* lo que cabe en una opción (los títulos más largos se cortan) */




///* Una opción: caja de 224 x 30; la marcada va rellena y con flecha */
//

////////////////////////////////////////////////////////////

/* ---------- pantallas ---------- */
/* Triángulo chico (hay más canciones arriba / abajo) */
static void Flecha(int y, uint8_t arriba, uint8_t mostrar) {
	for (int i = 0; i < 6; i++) {
		int fila = arriba ? i : 5 - i;
		FillRectFast(ANCHO / 2 - i * 2, y + fila, i * 4 + 1, 1, mostrar ? C_AMARILLO : C_FONDO);
	}
}

////////////////////////////////////////////////////////
/* ============================================================
 * BEAT HERO - MOTOR DE TEXTO NEON CON FONDO TRANSPARENTE
 * ============================================================ */

/* Una fila RGB565: 240 x 2 = 480 bytes de RAM */
static uint16_t titulo_fila[ANCHO];

/* Mezclar dos colores RGB565 sin utilizar float */
static uint16_t MezclarRGB565(uint16_t base,
                              uint16_t tinta,
                              uint8_t opacidad)
{
    uint32_t resto = 255u - opacidad;

    uint32_t r =
        ((((uint32_t)(base >> 11) & 31u) * resto) +
         (((uint32_t)(tinta >> 11) & 31u) * opacidad)) / 255u;

    uint32_t g =
        ((((uint32_t)(base >> 5) & 63u) * resto) +
         (((uint32_t)(tinta >> 5) & 63u) * opacidad)) / 255u;

    uint32_t b =
        ((((uint32_t)base & 31u) * resto) +
         (((uint32_t)tinta & 31u) * opacidad)) / 255u;

    return (uint16_t)((r << 11) | (g << 5) | b);
}

/* Dibuja una capa de texto sobre la fila RGB565 */
static void CapaTextoFila(const char *s,
                          int x, int y,
                          uint8_t grande,
                          uint8_t escala,
                          int yy,
                          int dx, int dy,
                          uint16_t color,
                          uint8_t opacidad)
{
    const int fw = grande ? 16 : 8;
    const int fh = grande ? 16 : 12;

    const int localY = yy - (y + dy);

    if (localY < 0 || localY >= fh * escala)
        return;

    const int f = localY / escala;

    for (int i = 0; s[i] != '\0'; i++) {

        unsigned char ch = (unsigned char)s[i];

        if (ch < 32 || ch > 126)
            ch = '?';

        uint16_t bits = grande
            ? bigFont[(ch - 32) * 16 + f]
            : smallFont[(ch - 32) * 12 + f];

        for (int col = 0; col < fw; col++) {

            if (!(bits & (1u << (fw - 1 - col))))
                continue;

            int px0 = x + i * fw * escala
                        + col * escala + dx;

            for (int sub = 0; sub < escala; sub++) {

                int px = px0 + sub;

                if (px < 0 || px >= ANCHO)
                    continue;

                titulo_fila[px] = (opacidad == 255)
                    ? color
                    : MezclarRGB565(
                          titulo_fila[px],
                          color,
                          opacidad
                      );
            }
        }
    }
}

/* Texto centrado con sombra, halo y contorno */
static void TextoNeon(const char *s,
                      int y,
                      uint8_t grande,
                      uint8_t escala,
                      uint16_t color,
                      uint16_t brillo)
{
    const int fw = grande ? 16 : 8;
    const int fh = grande ? 16 : 12;

    const int ancho = (int)strlen(s) * fw * escala;
    const int x = (ANCHO - ancho) / 2;

    int inicio = y - 4;
    int fin = y + fh * escala + 6;

    if (inicio < 0)
        inicio = 0;

    if (fin > ALTO_PANT)
        fin = ALTO_PANT;

    for (int yy = inicio; yy < fin; yy++) {

        /* Recuperar los pixeles originales del cielo */
        memcpy(titulo_fila,
               &fondoT[(uint32_t)yy * ANCHO],
               sizeof(titulo_fila));

        /* Sombra oscura */
        CapaTextoFila(s, x, y, grande, escala,
                      yy, 3, 4, 0x0004, 205);

        /* Resplandor exterior */
        CapaTextoFila(s, x, y, grande, escala,
                      yy, -3, 0, brillo, 90);

        CapaTextoFila(s, x, y, grande, escala,
                      yy, 3, 0, brillo, 90);

        CapaTextoFila(s, x, y, grande, escala,
                      yy, 0, -3, brillo, 90);

        CapaTextoFila(s, x, y, grande, escala,
                      yy, 0, 3, brillo, 90);

        CapaTextoFila(s, x, y, grande, escala,
                      yy, -2, -2, brillo, 70);

        CapaTextoFila(s, x, y, grande, escala,
                      yy, 2, -2, brillo, 70);

        CapaTextoFila(s, x, y, grande, escala,
                      yy, -2, 2, brillo, 70);

        CapaTextoFila(s, x, y, grande, escala,
                      yy, 2, 2, brillo, 70);

        /* Contorno oscuro para mejorar el contraste */
        CapaTextoFila(s, x, y, grande, escala,
                      yy, -1, 0, 0x1808, 255);

        CapaTextoFila(s, x, y, grande, escala,
                      yy, 1, 0, 0x1808, 255);

        CapaTextoFila(s, x, y, grande, escala,
                      yy, 0, -1, 0x1808, 255);

        CapaTextoFila(s, x, y, grande, escala,
                      yy, 0, 1, 0x1808, 255);

        /* Letras principales */
        CapaTextoFila(s, x, y, grande, escala,
                      yy, 0, 0, color, 255);

        /* Enviar la fila terminada a la pantalla */
        LCD_Bitmap(0, (unsigned int)yy,
                   ANCHO, 1, titulo_fila);
    }
}

//////////////////////////////////////////////////////

/* ============================================================
 * PANTALLA DE INICIO - BEAT HERO
 * ============================================================ */

static void Dibujar_Inicio(void)
{
    /* Fondo: cielo estrellado RGB565 */
    LCD_Bitmap(0, 0, ANCHO, ALTO_PANT, fondoT);

    /* BEAT: blanco con resplandor magenta */
    TextoNeon(
        "BEAT",
        94,
        1,
        3,
        C_BLANCO,
        C_MAGENTA
    );

    /* HERO: blanco con resplandor cian */
    TextoNeon(
        "HERO",
        154,
        1,
        3,
        C_BLANCO,
        C_CIAN
    );

    /* Instruccion inferior */
    TextoNeon(
        "PRESIONA START",
        270,
        0,
        2,
        C_AMARILLO,
        C_CIAN
    );
}


/* ---------- Pantalla de Inicio ---------- */
/* ---------- Pantalla de Inicio ---------- */
//static void Dibujar_Inicio(void)
//{
//    /* Imagen de fondo RGB565 de 240x320 */
//    LCD_Bitmap(0, 0, ANCHO, ALTO_PANT, fondoT);
//
//    /* Panel sólido detrás del título */
//    FillRectFast(20, 85, ANCHO - 40, 70, C_AZUL_OSCURO);
//
//    /* Título principal */
//    Texto("MUSICA", 100, 1, C_VERDE, C_AZUL_OSCURO);
//    Texto("GAME", 130, 1, C_CIAN, C_AZUL_OSCURO);
//
//    /* Instrucción */
//    Texto("PRESIONA START", 240, 0,
//          C_AMARILLO, C_MORADO);
//}

///////////////////////////////////////////////////////////

//static void Dibujar_Canciones(uint8_t marcada, uint8_t cuantas, uint8_t primera, uint8_t completa, uint8_t lista) {
//	char t[12];
//	if (completa) {
//		Marco();
//		// NUEVO: Encabezado tipo Arcade
//		FillRectFast(3, 3, ANCHO - 6, 45, C_AZUL_OSCURO);
//		Texto("- TRACKLIST -", 16, 1, C_AMARILLO, C_AZUL_OSCURO);
//		FillRectFast(3, 48, ANCHO - 6, 2, C_CIAN); // Línea divisoria inferior
//
//		Texto("FLECHAS: MOVER   A: JUGAR", 292, 0, C_AMARILLO, C_FONDO);
//	}
//
//	uint8_t filas = (cuantas > primera) ? (uint8_t)(cuantas - primera) : 0;
//	if (filas > GH_MENU_FILAS) filas = GH_MENU_FILAS;
//
//	for (uint8_t f = 0; f < GH_MENU_FILAS; f++) {
//		uint8_t i = (uint8_t)(primera + f);
//		if (f >= filas) {
//			if (lista) FillRectFast(8, LISTA_Y + f * PASO, ANCHO - 16, 30, C_FONDO);
//			continue;
//		}
//		if (lista || i == marcada || i == marcada_dib)
//			Opcion(f, LISTA_Y, titulos[f][0] ? titulos[f] : "?", i == marcada);
//	}
//
//	if (lista) {
//		Flecha(LISTA_Y - 9, 1, primera > 0);
//		Flecha(LISTA_Y + GH_MENU_FILAS * PASO - 3, 0, primera + GH_MENU_FILAS < cuantas);
//	}
//
//	snprintf(t, sizeof(t), "%u/%u", (unsigned)(marcada + 1), (unsigned)cuantas);
//	FillRectFast(12, 16, 56, 16, C_AZUL_OSCURO); // Fondo coincide con el encabezado
//	if (cuantas > GH_MENU_FILAS) LCD_Print(t, 12, 16, 1, C_BLANCO, C_AZUL_OSCURO);
//}
//////////////////////////////////////////////////////////


#define PREVIEW_W  96
#define PREVIEW_H  96

static const uint16_t *Portada_De_Cancion(uint8_t idx) {
    switch (idx) {
        case 0: return portada0;
        case 1: return portada1;
        case 2: return portada2;
        case 3: return portada3;
        default: return portada_default;
    }
}

///////////////////////////////////////////////////////////////////




/* Lista de canciones: se ven GH_MENU_FILAS a partir de 'primera'; titulos[f] es la fila visible f */
//
//static void Dibujar_Canciones(uint8_t marcada, uint8_t cuantas, uint8_t primera, uint8_t completa, uint8_t lista) {
//	char t[12];
//	if (completa) {
//		Marco();
//		Texto("ELIGE", 22, 1, C_MAGENTA, C_FONDO);
//		Texto("UNA CANCION", 42, 1, C_CIAN, C_FONDO);
//		Texto("FLECHAS: MOVER   A: JUGAR", 292, 0, C_AMARILLO, C_FONDO);
//	}
//	uint8_t filas = (cuantas > primera) ? (uint8_t)(cuantas - primera) : 0;
//	if (filas > GH_MENU_FILAS) filas = GH_MENU_FILAS;
//	for (uint8_t f = 0; f < GH_MENU_FILAS; f++) {
//		uint8_t i = (uint8_t)(primera + f);
//		if (f >= filas) {                                         /* fila sin canción */
//			if (lista) FillRectFast(8, LISTA_Y + f * PASO, ANCHO - 16, 30, C_FONDO);
//			continue;
//		}
//		if (lista || i == marcada || i == marcada_dib)
//			Opcion(f, LISTA_Y, titulos[f][0] ? titulos[f] : "?", i == marcada);
//	}
//	if (lista) {
//		Flecha(LISTA_Y - 9, 1, primera > 0);                      /* hay más arriba */
//		Flecha(LISTA_Y + GH_MENU_FILAS * PASO - 3, 0, primera + GH_MENU_FILAS < cuantas);   /* hay más abajo */
//	}
//	snprintf(t, sizeof(t), "%u/%u", (unsigned)(marcada + 1), (unsigned)cuantas);   /* número de canción, arriba a la izquierda */
//	FillRectFast(12, 10, 56, 12, C_FONDO);
//	if (cuantas > GH_MENU_FILAS) LCD_Print(t, 12, 10, 1, C_GRIS, C_FONDO);
//}

//////////////////////////////////////////////////////////////////////

static void Dibujar_Canciones(uint8_t marcada, uint8_t cuantas, uint8_t primera, uint8_t completa, uint8_t lista) {
    char t[12];

    if (completa) {

        /* Fondo estrellado de BEAT HERO */
        LCD_Bitmap(0, 0, ANCHO, ALTO_PANT, fondoT);

        /* Titulo del menu con efecto neon */
        TextoNeon(
            "TRACKLIST",
            15,
            0,
            2,
            C_BLANCO,
            C_MAGENTA
        );

        /* Instrucciones sin fondo solido */
        TextoNeon(
            "FLECHAS: MOVER A: JUGAR",
            290,
            0,
            1,
            C_AMARILLO,
            C_CIAN
        );
    }

    uint8_t filas = (cuantas > primera) ? (uint8_t)(cuantas - primera) : 0;
    if (filas > GH_MENU_FILAS) filas = GH_MENU_FILAS;

    for (uint8_t f = 0; f < GH_MENU_FILAS; f++) {
        uint8_t i = (uint8_t)(primera + f);

        if (f >= filas) {
            if (lista) FillRectFast(LISTA_X, LISTA_Y + f * PASO, LISTA_W, 24, C_FONDO);
            continue;
        }

        if (lista || i == marcada || i == marcada_dib) {
//            Opcion(f, LISTA_Y, titulos[f][0] ? titulos[f] : "?", i == marcada);
        	Opcion_Cancion(f, LISTA_Y, titulos[f][0] ? titulos[f] : "?", i == marcada);
        }
    }

    if (lista) {
        Flecha(LISTA_Y - 9, 1, primera > 0);
        Flecha(LISTA_Y + GH_MENU_FILAS * PASO - 9, 0, primera + GH_MENU_FILAS < cuantas);
    }

    /* Contador */
    snprintf(t, sizeof(t), "%u/%u", (unsigned)(marcada + 1), (unsigned)cuantas);
    FillRectFast(12, 56, 56, 12, C_FONDO);
    LCD_Print(t, 12, 56, 1, C_GRIS, C_FONDO);

    /* Preview de canción seleccionada */
    if (completa || lista || marcada != marcada_dib) {
        Dibujar_Preview_Cancion(marcada, primera);
    }
}


////////////////////////////////////////////////////////////////////////

static void Dibujar_Resultados(uint8_t ganador, uint8_t yo) {
	char t[32];
	Marco();
	Texto("RESULTADOS", 24, 1, C_CIAN, C_FONDO);
	if (ganador == 0) {
		Texto("EMPATE", 80, 1, C_AMARILLO, C_FONDO);
	} else if (ganador == yo) {
		Texto("GANASTE!", 80, 1, C_VERDE, C_FONDO);
	} else {
		Texto("PERDISTE", 80, 1, C_ROJO, C_FONDO);
	}
	if (ganador) { snprintf(t, sizeof(t), "GANADOR: JUGADOR %u", ganador); Texto(t, 112, 0, C_BLANCO, C_FONDO); }

	FillRectFast(20, 140, ANCHO - 40, 1, C_MAGENTA);
	snprintf(t, sizeof(t), "TUS PUNTOS");     Texto(t, 156, 0, C_GRIS, C_FONDO);
	snprintf(t, sizeof(t), "%lu", (unsigned long)pts_mios);  Texto(t, 172, 1, C_BLANCO, C_FONDO);
	snprintf(t, sizeof(t), "RIVAL");          Texto(t, 206, 0, C_GRIS, C_FONDO);
	snprintf(t, sizeof(t), "%lu", (unsigned long)pts_rival); Texto(t, 222, 1, C_GRIS, C_FONDO);
	FillRectFast(20, 252, ANCHO - 40, 1, C_MAGENTA);
	Texto("A: CONTINUAR", 292, 0, C_AMARILLO, C_FONDO);
}

/* Opción del menú final: caja de todo el ancho (224 x 30), texto centrado; la marcada va rellena y con flecha.
 * (antes usaba la Opcion() angosta de la lista con preview y los textos se cortaban). */
static void Opcion_Fin(int i, int y0, const char *txt, uint8_t marcada) {
	int y = y0 + i * 40;
	uint16_t fondo = marcada ? C_CIAN : C_MORADO;
	uint16_t letra = marcada ? C_FONDO : C_BLANCO;
	FillRectFast(8, y, ANCHO - 16, 30, fondo);
	if (!marcada) {                                           /* contorno magenta en las no marcadas */
		FillRectFast(8, y, ANCHO - 16, 1, C_MAGENTA);
		FillRectFast(8, y + 29, ANCHO - 16, 1, C_MAGENTA);
	}
	Texto(txt, y + 9, 0, letra, fondo);
	if (marcada) LCD_Print(">", 16, y + 9, 1, C_FONDO, fondo);
}

static void Dibujar_Fin(uint8_t marcada, uint8_t completa) {
	if (completa) {
		Marco();
		Texto("Y AHORA?", 40, 1, C_MAGENTA, C_FONDO);
		Texto("FLECHAS: MOVER   A: ELEGIR", 292, 0, C_AMARILLO, C_FONDO);
	}
	Opcion_Fin(0, 120, "REVANCHA", marcada == 0);
	Opcion_Fin(1, 120, "SALIR", marcada == 1);
}

uint8_t GH_Menu_Paquete(const uint8_t *b) {
	switch (b[1]) {
		case GH_PKT_TEXTO: {
			uint8_t linea = b[2], pos = b[3];
			if (linea >= GH_TEXTO_LINEAS) return 1;
			for (uint8_t k = 0; k < 3; k++) {
				if (pos + k >= GH_TEXTO_LARGO) break;
				if (titulos[linea][pos + k] != (char)b[4 + k]) titulos_nuevos = 1;
				titulos[linea][pos + k] = (char)b[4 + k];
				if (!b[4 + k]) break;
			}
			titulos[linea][GH_TEXTO_LARGO] = 0;
			return 1;
		}
		case GH_PKT_RESULTADO: {
			uint32_t p = (uint32_t)b[3] | ((uint32_t)b[4] << 8) | ((uint32_t)b[5] << 16) | ((uint32_t)b[6] << 24);
			if (b[2] == 1) pts_mios = p; else pts_rival = p;
			return 1;
		}
		case GH_PKT_PANTALLA: {
			uint8_t nuevo = b[2], marcada = b[3], cuantas = b[4];
			uint8_t primera = (nuevo == GH_PANT_CANCIONES) ? b[5] : 0;
			uint8_t completa = (nuevo != modo) || GH_Pista_Activa();
			uint8_t lista = completa || cuantas != cuantas_dib || primera != primera_dib
			             || (nuevo == GH_PANT_CANCIONES && titulos_nuevos);
			/* MusicaGame la vuelve a mandar cada segundo en los menús: si no cambió nada, no se redibuja */
//			if (!lista && marcada == marcada_dib) return 1;
//			if (GH_Pista_Activa()) GH_Pista_Desactivar();

			///////////////////////////////////////////
			if (!lista && marcada == marcada_dib)
			    return 1;

			/* Reiniciar el texto desplazable si cambia la selección
			 * o si se actualiza la lista de canciones.
			 */
			if (nuevo == GH_PANT_CANCIONES &&
			    (completa || lista || marcada != marcada_dib)) {

			    paso_scroll = 0;
			    t_scroll = HAL_GetTick();
			}

			if (GH_Pista_Activa())
			    GH_Pista_Desactivar();
			//////////////////////////////////////////
			switch (nuevo) {
				case GH_PANT_CANCIONES:  Dibujar_Canciones(marcada, cuantas, primera, completa, lista); break;
				case GH_PANT_RESULTADOS: Dibujar_Resultados(b[5], b[6]);               break;
				case GH_PANT_FIN:        Dibujar_Fin(marcada, completa);               break;

				case GH_PANT_INICIO:     Dibujar_Inicio();

				default: break;
			}
			modo = nuevo;
			marcada_dib = marcada;
			cuantas_dib = cuantas;
			primera_dib = primera;
			if (nuevo == GH_PANT_CANCIONES) titulos_nuevos = 0;
			return 1;
		}
		case GH_PKT_CANCION:
			modo = 0;                                         /* la pista retoma la pantalla */
			return 0;
		default:
			return 0;
	}
}





////////////////////////////////////////


void GH_Menu_Actualizar(void)
{
    /* Animar únicamente durante la selección de canciones */
    if (modo != GH_PANT_CANCIONES)
        return;

    /* Comprobar que exista una canción seleccionada */
    if (marcada_dib >= cuantas_dib)
        return;

    if (marcada_dib < primera_dib)
        return;

    uint8_t fila = marcada_dib - primera_dib;

    if (fila >= GH_MENU_FILAS)
        return;

    const char *nombre = titulos[fila];

    unsigned int longitud = (unsigned int)strlen(nombre);

    /* Los textos cortos no necesitan animación */
    if (longitud <= TXT_LISTA_W / 8 &&
        longitud <= TXT_PREV_W / 8)
        return;

    uint32_t ahora = HAL_GetTick();

    /* Esperar 1 segundo al comienzo y 200 ms entre pasos */
    uint32_t intervalo = (paso_scroll == 0)
                       ? SCROLL_ESPERA_MS
                       : SCROLL_MS;

    if ((uint32_t)(ahora - t_scroll) < intervalo)
        return;

    t_scroll = ahora;

    /* Avanzar una letra */
    paso_scroll++;

    /* Al terminar, volver al inicio */
    if (paso_scroll > longitud + 2u)
        paso_scroll = 0;


// Actualizar texto de la lista

    int y_lista = LISTA_Y + fila * PASO;

    FillRectFast(TXT_LISTA_X,
                 y_lista + 6,
                 TXT_LISTA_W,
                 14,
                 C_CIAN);

    TextoVentana(nombre,
                 TXT_LISTA_X,
                 y_lista + 7,
                 TXT_LISTA_W,
                 C_FONDO,
                 C_CIAN,
                 paso_scroll);


//Actualizar texto del preview

    FillRectFast(TXT_PREV_X, PREV_Y + 114, TXT_PREV_W, 14, C_FONDO);

    TextoVentana(nombre, TXT_PREV_X, TXT_PREV_Y, TXT_PREV_W, C_BLANCO, C_FONDO, paso_scroll);
}



////////////////////////////////////////////////////

#define PREV_X      136
#define PREV_Y       68
#define PREV_W      100
#define PREV_H      132

#define PREV_IMG_X  138
#define PREV_IMG_Y   86

static void Dibujar_Preview_Cancion(uint8_t marcada, uint8_t primera) {
    const uint16_t *img = Portada_De_Cancion(marcada);
    const char *titulo = "?";

    if (marcada >= primera && marcada < primera + GH_MENU_FILAS) {
        uint8_t fila = (uint8_t)(marcada - primera);
        if (titulos[fila][0]) titulo = titulos[fila];
    }

    /* Limpiar área preview */
    FillRectFast(PREV_X, PREV_Y, PREV_W, PREV_H, C_FONDO);

    /* Marco */
    FillRectFast(PREV_X, PREV_Y, PREV_W, 2, C_MAGENTA);
    FillRectFast(PREV_X, PREV_Y + PREV_H - 2, PREV_W, 2, C_MAGENTA);
    FillRectFast(PREV_X, PREV_Y, 2, PREV_H, C_MAGENTA);
    FillRectFast(PREV_X + PREV_W - 2, PREV_Y, 2, PREV_H, C_MAGENTA);

    /* Encabezado */
    FillRectFast(PREV_X + 2, PREV_Y + 2, PREV_W - 4, 14, C_AZUL_OSCURO);
    TextoCaja("PREVIEW", PREV_X + 2, PREV_Y + 5, PREV_W - 4, 0, C_AMARILLO, C_AZUL_OSCURO);

    /* Imagen */
    LCD_Bitmap(PREV_IMG_X, PREV_IMG_Y, PREVIEW_W, PREVIEW_H, img);

    /* Título de la canción */
//    FillRectFast(PREV_X + 2, PREV_Y + 114, PREV_W - 4, 14, C_FONDO);
//    TextoCaja(titulo, PREV_X + 2, PREV_Y + 117, PREV_W - 4, 0, C_BLANCO, C_FONDO);

    FillRectFast(TXT_PREV_X, PREV_Y + 114, TXT_PREV_W, 14, C_FONDO);

    TextoVentana(titulo, TXT_PREV_X, TXT_PREV_Y, TXT_PREV_W, C_BLANCO, C_FONDO, 0);


}


////////////////////////////////////////////////////


