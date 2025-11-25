# Multistream-Mosaic
Aplicación en C++ con GTK y GStreamer para visualizar múltiples transmisiones SRT/UDP en un mosaico dinámico con reconexión automática. Cada stream es manejado por un StreamSlot independiente con un sistema de watchdog que monitora buffers, detecta desconexiones y despliega una pantalla negra tras 5s sin señal.

---

# Multistream Mosaic (GTK + GStreamer)

**Multistream Mosaic** es una aplicación en C++ diseñada para recibir, organizar y visualizar múltiples transmisiones de video en tiempo real utilizando **GStreamer**.  
La interfaz gráfica está desarrollada con **GTK**, y cada transmisión es manejada de manera independiente por un sistema modular basado en la clase `StreamSlot`.

El proyecto está optimizado para transmisiones **SRT** y **UDP**, así como para sistemas donde la latencia y la estabilidad son críticas.

---

## Características principales

- Visualización de múltiples streams en mosaico.
- Soporte para **SRT (caller/listener)** y **UDP unicast**.
- Reproducción mediante `srtclientsrc`, `udpsrc`, `decodebin`, `autovideosink`, etc.
- Detección automática de desconexión.
- **Watchdog integrado**:
  - Monitorea buffers.
  - Muestra pantalla negra tras 5s sin señal.
  - Restaura el stream automáticamente.
- Arquitectura modular con:
  - `StreamSlot`
  - `Watchdog`

---

## Estructura del Código

```
/multistream_mosaic
├─ main.cpp
├─ StreamSlot.cpp
├─ StreamSlot.h
├─ Watchdog.cpp
├─ Watchdog.h
└─ …
```

---

## **Main**

- Inicializa GTK.
- Crea la ventana principal.
- Genera un arreglo de `StreamSlot`.
- Maneja el loop principal.

Incluye:

```cpp
#include <gtk/gtk.h>
#include "StreamSlot.h"
#include <vector>
#include <memory>
```

---

## **StreamSlot**

Componente que administra individualmente cada transmisión.

Responsable de:

- Construir el pipeline de GStreamer.
- Supervisar estados `PLAYING`, `PAUSED`, `NULL`.
- Insertar *buffer probes*.
- Coordinarse con el Watchdog para:
  - pantalla negra,
  - restaurar stream,
  - reinstalar probes al reconectar.

Incluye:

```cpp
#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/videooverlay.h>
#include "StreamSlot.h"
#include "Watchdog.h"
```

---

## **Watchdog**

Hilo auxiliar encargado de monitorear cada stream.

Funciones:

- Detectar ausencia de buffers.
- Notificar al `StreamSlot` cuando se excede el tiempo máximo.
- Solicitar pantalla negra o restauración.

Incluye:

```cpp
#include <gst/gst.h>
#include <atomic>
#include <thread>
#include <functional>
```

---

## Dependencias

### En Linux (Ubuntu/Debian)

```bash
sudo apt install g++ make cmake
sudo apt install libgtk-3-dev
sudo apt install libgstreamer1.0-dev \
                 libgstreamer-plugins-base1.0-dev \
                 gstreamer1.0-plugins-good \
                 gstreamer1.0-plugins-bad \
                 gstreamer1.0-plugins-ugly
```

---

## Compilación

### Usando CMake
```bash
mkdir build
cd build
cmake ..
make
```

### O usando Makefile
```bash
make
```

---

## Transmisión SRT (cliente)

Ejemplo en OBS:

```bash
srt://<ip-servidor>:<puerto>?streamid=uplive.sls.com/live/stream1
```

---

## Transmisión UDP (cliente)

Ejemplo usando GStreamer:

```bash
gst-launch-1.0 -v d3d11screencapturesrc ! video/x-raw,framerate=60/1 ! queue ! videoconvert ! video/x-raw,format=NV12 ! nvh264enc rc-mode=cbr bitrate=4000 zerolatency=true ! h264parse config-interval=-1 ! rtph264pay config-interval=1 pt=96 ! udpsink host=<IP-servidor> port=<puerto-libre>
```

---

## Notas adicionales

- Para baja latencia, usar UDP.
- El watchdog no introduce retardo perceptible.
- El mosaico puede ampliarse o migrarse a layouts dinámicos.
