# Multistream-Mosaic
Aplicación en C++ con GTK y GStreamer para visualizar múltiples transmisiones SRT/UDP en un mosaico dinámico con reconexión automática. Cada stream es manejado por un StreamSlot independiente con un sistema de watchdog que monitorea buffers, detecta desconexiones y despliega una pantalla negra tras 5s sin señal.
