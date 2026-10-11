/*
 * esp32_controles.ino
 * Autores: Abner Quiej y Edwin Parada
 */
#include <Bluepad32.h>

/* ---------- UART hacia el STM32 ---------- */
#define UART_STM   Serial2
#define UART_BAUD  115200
#define PIN_RX2    16      
#define PIN_TX2    17

#define MAX_JUGADORES 2

/* ---------- Botones que se usan en el juego ---------- */
enum { GRUPO_BOTONES, GRUPO_DPAD, GRUPO_MISC };

struct Boton {
    uint8_t     grupo;
    uint16_t    mascara;
    const char *nombre;
};

/* Cada color tiene DOS botones del control: cualquiera de los dos vale.
 * La máscara junta los dos; solo se manda "presionado" cuando se aprieta el primero de los dos
 * y "_off" cuando se suelta el último (así el STM nunca ve un _off mientras el otro sigue apretado). */
static const Boton botones[] = {
    { GRUPO_BOTONES, 0x0001 | 0x0040, "A"     },   /* verde:    A  o 0x0040 */
    { GRUPO_BOTONES, 0x0002 | 0x0010, "B"     },   /* rojo:     B  o 0x0010 */
    { GRUPO_BOTONES, 0x0004 | 0x0020, "X"     },   /* amarillo: X  o 0x0020 */
    { GRUPO_BOTONES, 0x0008 | 0x0080, "Y"     },   /* azul:     Y  o 0x0080 */
    { GRUPO_DPAD,    0x01,   "Up"    },
    { GRUPO_DPAD,    0x02,   "Down"  },
    { GRUPO_DPAD,    0x04,   "Right" },
    { GRUPO_DPAD,    0x08,   "Left"  },
    { GRUPO_MISC,    0x04,   "Start" },
};

/* ---------- Estado de cada jugador ---------- */
struct Estado {
    uint16_t botones;
    uint8_t  dpad;
    uint8_t  misc;
};

static ControllerPtr jugadores[MAX_JUGADORES] = { nullptr, nullptr };
static Estado        previo[MAX_JUGADORES];

static uint16_t leerGrupo(const Estado &e, uint8_t grupo) {
    switch (grupo) {
        case GRUPO_BOTONES: return e.botones;
        case GRUPO_DPAD:    return e.dpad;
        default:            return e.misc;
    }
}

/* Manda "pN_nombre\n" (presionado) o "pN_nombre_off\n" (soltado) al STM32 y al monitor serie */
static void enviar(int jugador, const char *nombre, bool presionado) {
    char linea[20];
    snprintf(linea, sizeof(linea), "p%d_%s%s\n", jugador + 1, nombre, presionado ? "" : "_off");
    UART_STM.print(linea);
    Serial.print(linea);
}

/* Si un control se desconecta con botones apretados, avisa que se soltaron
   (si no, el STM se quedaría creyendo que siguen presionados) */
static void soltarTodo(int j) {
    for (const Boton &b : botones)
        if ((leerGrupo(previo[j], b.grupo) & b.mascara) != 0)
            enviar(j, b.nombre, false);
    previo[j] = { 0, 0, 0 };
}

/* ---------- Conexión / desconexión ---------- */
void onConnectedController(ControllerPtr ctl) {
    for (int i = 0; i < MAX_JUGADORES; i++) {
        if (jugadores[i] == nullptr) {
            jugadores[i] = ctl;
            previo[i] = { 0, 0, 0 };
            Serial.printf("Jugador %d conectado (%s)\n", i + 1, ctl->getModelName().c_str());
            return;
        }
    }
    Serial.println("Ya hay 2 jugadores: control rechazado");
    ctl->disconnect();
}

void onDisconnectedController(ControllerPtr ctl) {
    for (int i = 0; i < MAX_JUGADORES; i++) {
        if (jugadores[i] == ctl) {
            jugadores[i] = nullptr;
            soltarTodo(i);
            Serial.printf("Jugador %d desconectado\n", i + 1);
            return;
        }
    }
}

/* ---------- Detectar cambios: cada botón por separado (varios a la vez funcionan) ---------- */
static void revisarJugador(int j) {
    ControllerPtr ctl = jugadores[j];
    if (ctl == nullptr || !ctl->isConnected() || !ctl->hasData() || !ctl->isGamepad())
        return;

    Estado ahora = { ctl->buttons(), ctl->dpad(), ctl->miscButtons() };

    for (const Boton &b : botones) {
        bool estaba = (leerGrupo(previo[j], b.grupo) & b.mascara) != 0;   /* cualquiera de sus botones */
        bool esta   = (leerGrupo(ahora,     b.grupo) & b.mascara) != 0;
        if (esta != estaba)             /* cambió: se presionó o se soltó */
            enviar(j, b.nombre, esta);
    }
    previo[j] = ahora;
}

void setup() {
    Serial.begin(115200);
    UART_STM.begin(UART_BAUD, SERIAL_8N1, PIN_RX2, PIN_TX2);

    Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
    BP32.setup(&onConnectedController, &onDisconnectedController);

    /* Borra los emparejamientos en cada reinicio: los controles hay que volver a ponerlos en
       modo emparejar. Si comentas esta línea, se reconectan solos. */
    BP32.forgetBluetoothKeys();

    BP32.enableVirtualDevice(false);
}

void loop() {
    if (BP32.update()) {
        for (int j = 0; j < MAX_JUGADORES; j++)
            revisarJugador(j);
    }
    delay(1);   /* cede el CPU (evita el watchdog). El ejemplo traía 150 ms: demasiado lento para el juego */
}
