# Multistream-Mosaic
Aplicación en C++ con GTK y GStreamer para visualizar múltiples transmisiones SRT/UDP en un mosaico dinámico con reconexión automática. Cada stream es manejado por un StreamSlot independiente con un sistema de watchdog que monitorea buffers, detecta desconexiones y despliega una pantalla negra tras 5s sin señal.

# Multistream Mosaic (GTK + GStreamer)

**Multistream Mosaic** es una aplicación en C++ diseñada para recibir, organizar y visualizar múltiples transmisiones de video en tiempo real utilizando **GStreamer**.  
La interfaz gráfica está desarrollada con **GTK**, y cada transmisión es manejada de manera independiente por un sistema modular basado en la clase `StreamSlot`.

El proyecto está optimizado para transmisiones **SRT** y **UDP**, así como para sistemas donde la latencia y la estabilidad son críticas (p. ej., entornos de realidad aumentada, telemetría, visión remota o supervisión distribuida).

---

## Características principales

- Visualización de múltiples streams en mosaico.
- Soporte para **SRT (caller/listener)** y **UDP unicast**.
- Reproducción de video mediante `srtclientsrc`, `udpsrc`, `decodebin`, `autovideosink`, etc.
- Detección automática de desconexión de señal.
- **Watchdog integrado**:
  - Monitorea la llegada de buffers.
  - Activa pantalla negra tras 5 segundos sin video.
  - Restaura automáticamente el stream al reconectar.
- Modularidad a través de:
  - `StreamSlot` (gestor de cada stream)
  - `Watchdog` (detección de actividad)
- Compatible con OBS, ffmpeg y herramientas de streaming.
- Código limpio y listo para expandir a overlays, teclas de control o reenvío de streams.

---

## Estructura del Código

/src
├─ main.cpp
├─ StreamSlot.cpp
├─ StreamSlot.h
├─ Watchdog.cpp
├─ Watchdog.h
└─ …


### **main.cpp**
- Inicializa GTK.
- Crea la ventana principal.
- Genera un arreglo de `StreamSlot` para los mosaicos.
- Maneja el loop principal de la aplicación.

Incluye:
```cpp
#include <gtk/gtk.h>
#include "StreamSlot.h"
#include <vector>
#include <memory>

### **StreamSlot**
- Componente que administra individualmente cada transmisión.
- Responsable de:
  - Construir el pipeline de GStreamer.
  - Supervisar los estados `PLAYING`, `PAUSED` y `NULL`.
  - Insertar *buffer probes* para detectar actividad.
  - Coordinarse con el Watchdog para:
    - mostrar pantalla negra,
    - restaurar el stream,
    - reinstalar probes al reconectar.

Incluye:
```cpp
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/videooverlay.h>
#include "StreamSlot.h"
#include "Watchdog.h"

Watchdog

Hilo auxiliar encargado de monitorear cada stream.
Funciones:

Detectar inactividad en la llegada de buffers.

Notificar al StreamSlot cuando excede el tiempo máximo sin video.

Solicitar la pantalla negra o la restauración del stream.

Incluye:
#include <gst/gst.h>
#include <atomic>
#include <thread>
#include <functional>

Dependencias
En Linux (Ubuntu/Debian)

sudo apt install g++ make cmake
sudo apt install libgtk-3-dev
sudo apt install libgstreamer1.0-dev \
                 libgstreamer-plugins-base1.0-dev \
                 gstreamer1.0-plugins-good \
                 gstreamer1.0-plugins-bad \
                 gstreamer1.0-plugins-ugly


Compilación
Usando CMake
mkdir build
cd build
cmake ..
make

O usando Makefile
make

Los clientes pueden transmitir usando:

SRT

Ejemplo en OBS o ffmpeg:
srt://<ip-servidor>:<puerto>?streamid=uplive.sls.com/live/stream1

Notas adicionales

Para baja latencia, usar SRT en modo caller desde OBS.

El watchdog no introduce retardo perceptible.

El mosaico puede ampliarse, reducirse o migrarse a layouts dinámicos.
