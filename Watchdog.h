#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <gst/gst.h>
#include <atomic>
#include <thread>
#include <functional>

class Watchdog {
public:
    // callback para mostrar pantalla negra
    using Callback = std::function<void(bool show_black)>;

    Watchdog(Callback callback, int timeout_ms = 3000);
    ~Watchdog();

    // Indicar que se recibió un buffer (reset del contador)
    void notify_buffer();

    // Iniciar y detener el thread del watchdog
    void start();
    void stop();

private:
    void run();

    std::atomic<int> buffer_count;
    int timeout_ms;
    std::thread watchdog_thread;
    bool running;
    Callback callback;
};

#endif // WATCHDOG_H
