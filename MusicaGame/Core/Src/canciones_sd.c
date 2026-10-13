/*
 * canciones_sd.c
 * Lista y carga de las canciones .ghs de la microSD.
 */
#include "canciones_sd.h"
#include "ff.h"
#include "music.h"
#include "registro.h"
#include <string.h>

typedef struct {
	char archivo[FF_MAX_LFN + 12];
	char titulo[32];
} Ficha;

static FATFS   fs;
static uint8_t montada = 0;
static Ficha   lista[CANCIONES_SD_MAX];
static uint8_t total = 0;

/* Canción cargada */
static Note      notas[CANCIONES_SD_MAX_NOTAS];
static TrackDef  pistas[MUSIC_MAX_TRACKS];
static Song      cancion;
static uint8_t   p1[8], p2[8];
static char      titulo[32];
static SongEntry entrada;

static uint16_t U16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

static uint8_t Es_GHS(const char *n) {
	size_t l = strlen(n);
	if (l < 5) return 0;
	const char *e = n + l - 4;
	return e[0] == '.' && (e[1] | 0x20) == 'g' && (e[2] | 0x20) == 'h' && (e[3] | 0x20) == 's';
}

static uint8_t Leer_Titulo(const char *ruta, char *t) {
	FIL f; UINT n; uint8_t h[60];
	if (f_open(&f, ruta, FA_READ) != FR_OK) return 0;
	FRESULT r = f_read(&f, h, sizeof(h), &n);
	f_close(&f);
	if (r != FR_OK || n < sizeof(h) || memcmp(h, "GHS1", 4) != 0) return 0;
	memcpy(t, h + 28, 31); t[31] = 0;
	return 1;
}

static void Buscar_En(const char *carpeta) {
	DIR d; FILINFO fi;
	if (f_opendir(&d, carpeta) != FR_OK) return;
	while (total < CANCIONES_SD_MAX && f_readdir(&d, &fi) == FR_OK && fi.fname[0]) {
		if (fi.fattrib & AM_DIR) continue;
		if (!Es_GHS(fi.fname)) continue;
		Ficha *x = &lista[total];
		strcpy(x->archivo, carpeta);
		if (strcmp(carpeta, "/")) strcat(x->archivo, "/");
		strncat(x->archivo, fi.fname, sizeof(x->archivo) - strlen(x->archivo) - 1);
		if (Leer_Titulo(x->archivo, x->titulo)) total++;
		else Registro("SD: %s no es una cancion .ghs valida", x->archivo);
	}
	f_closedir(&d);
}

uint8_t CancionesSD_Iniciar(void) {
	total = 0;
	f_mount(0, "", 0);
	FRESULT r = f_mount(&fs, "", 1);
	montada = (r == FR_OK);
	if (!montada) { Registro("SD: no se pudo leer la tarjeta (error %u). Revisa conexion y formato FAT32.", r); return 0; }
	Buscar_En("/CANCIONES");
	if (!total) Buscar_En("/");
	/* ordenar por nombre de archivo */
	for (uint8_t i = 0; i + 1 < total; i++)
		for (uint8_t j = 0; j + 1 < total - i; j++)
			if (strcmp(lista[j].archivo, lista[j + 1].archivo) > 0) { Ficha t = lista[j]; lista[j] = lista[j + 1]; lista[j + 1] = t; }
	Registro("SD: %u canciones", total);
	for (uint8_t i = 0; i < total; i++) Registro("  %u  %s  (%s)", i, lista[i].titulo, lista[i].archivo);
	return total;
}

uint8_t CancionesSD_Total(void) { return total; }
uint8_t CancionesSD_Montada(void) { return montada; }
const char *CancionesSD_Titulo(uint8_t i) { return i < total ? lista[i].titulo : ""; }

const SongEntry *CancionesSD_Cargar(uint8_t i) {
	if (i >= total) return 0;
	FIL f; UINT n;
	uint8_t h[60];
	if (f_open(&f, lista[i].archivo, FA_READ) != FR_OK) { Registro("SD: no abre %s", lista[i].archivo); return 0; }
	if (f_read(&f, h, sizeof(h), &n) != FR_OK || n != sizeof(h) || memcmp(h, "GHS1", 4)) goto malo;

	uint8_t np = h[6];
	if (np == 0 || np > MUSIC_MAX_TRACKS) goto malo;
	uint8_t ph[MUSIC_MAX_TRACKS * 4];
	if (f_read(&f, ph, np * 4u, &n) != FR_OK || n != np * 4u) goto malo;

	uint32_t usadas = 0;
	for (uint8_t t = 0; t < np; t++) usadas += U16(&ph[t * 4 + 2]);
	if (usadas > CANCIONES_SD_MAX_NOTAS) {
		Registro("SD: %s tiene %lu notas, caben %u (CANCIONES_SD_MAX_NOTAS)", lista[i].archivo,
		         (unsigned long)usadas, CANCIONES_SD_MAX_NOTAS);
		f_close(&f);
		return 0;
	}

	/* las notas vienen en 5 bytes y se guardan como Note */
	uint8_t buf[500];
	uint32_t k = 0;
	for (uint8_t t = 0; t < np; t++) {
		uint16_t cuantas = U16(&ph[t * 4 + 2]);
		pistas[t].notes = &notas[k];
		pistas[t].length = cuantas;
		pistas[t].instrument = ph[t * 4];
		pistas[t].volume = ph[t * 4 + 1];
		uint32_t faltan = cuantas;
		while (faltan) {
			uint32_t lote = faltan > 100 ? 100 : faltan;
			if (f_read(&f, buf, lote * 5u, &n) != FR_OK || n != lote * 5u) goto malo;
			for (uint32_t j = 0; j < lote; j++) {
				const uint8_t *q = &buf[j * 5];
				notas[k].note = q[0];
				notas[k].dur = U16(q + 1);
				notas[k].vel = q[3];
				notas[k].flags = q[4];
				k++;
			}
			faltan -= lote;
		}
	}
	f_close(&f);

	uint8_t n1 = h[7] > 8 ? 8 : h[7], n2 = h[16] > 8 ? 8 : h[16];
	memcpy(p1, h + 8, 8); memcpy(p2, h + 17, 8);
	memcpy(titulo, h + 28, 31); titulo[31] = 0;
	cancion.name = titulo;
	cancion.bpm = U16(h + 4);
	cancion.num_tracks = np;
	cancion.tracks = pistas;
	entrada.song = &cancion;
	entrada.p1_tracks = n1 ? p1 : 0; entrada.p1_n = n1;
	entrada.p2_tracks = n2 ? p2 : 0; entrada.p2_n = n2;
	entrada.master = U16(h + 26);
	entrada.titulo = titulo;
	Registro("SD: cargada %s (%lu notas, %u pistas)", titulo, (unsigned long)k, np);
	return &entrada;

malo:
	f_close(&f);
	Registro("SD: %s esta danada o no es .ghs", lista[i].archivo);
	return 0;
}
