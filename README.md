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
La aplicación puede compilarse tanto en Linux como en Windows utilizando MSYS2 MinGW64.
En ambos casos se usa el mismo comando, gracias al soporte de pkg-config.

### Compilación en Linux (Ubuntu / Debian / Fedora / Arch)
1. Instalar dependencias

En Ubuntu/Debian:

```bash
sudo apt install build-essential pkg-config \
    libgtk-3-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev
```

2. Compilar
 ```bash
g++ main.cpp StreamSlot.cpp Watchdog.cpp -o multistream_mosaic $(pkg-config --cflags --libs gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0)
```

### Compilación en Windows (MSYS2 MinGW64)
En MSYS2, con GTK y GStreamer instalados, compile utilizando:

1. Instalar MSYS2 y dependencias

Abrir MSYS2 MinGW64 e instalar lo necesario:

```bash
pacman -Syu
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-pkg-config
pacman -S mingw-w64-x86_64-gtk3
pacman -S mingw-w64-x86_64-gstreamer
pacman -S mingw-w64-x86_64-gst-plugins-base
```
Importante: Todo debe instalarse desde MSYS2.
No sirven las versiones de GTK o GStreamer instaladas por ejecutable .msi.

2. Compilar

Dentro de la shell MSYS2 MinGW64, ejecutar:

```bash
g++ main.cpp StreamSlot.cpp Watchdog.cpp -o main.exe $(pkg-config --cflags --libs gtk+-3.0 gstreamer-1.0 gstreamer-video-1.0)
```

### VS Code Configuration

El proyecto incluye una carpeta `.vscode` con:
- `c_cpp_properties.json` (rutas de include y estándar C++)
- `tasks.json` (comandos de compilación usando MSYS2)
- `settings.json` (formatos, encoding y ajustes recomendados)

La carpeta `.vscode` incluida en este repositorio contiene configuraciones de Visual Studio Code que sirven únicamente como ejemplo de cómo compilar y ejecutar la aplicación desde el editor.

Estas configuraciones no son obligatorias y pueden modificarse o eliminarse según las necesidades del usuario.
El proyecto puede compilarse sin ellas utilizando los comandos descritos en la sección de compilación.

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
