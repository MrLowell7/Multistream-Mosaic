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

StreamSlot

Cada espacio del mosaico.
Responsabilidades:

Crear el pipeline GStreamer.

Manejar estados PLAYING/PAUSED/NULL.

Detectar reconexiones.

Insertar y reinstalar el buffer_probe.

Coordinarse con Watchdog.

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/videooverlay.h>
#include "StreamSlot.h"
#include "Watchdog.h"




