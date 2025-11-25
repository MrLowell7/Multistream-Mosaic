#include "Watchdog.h"
#include <chrono>
#include <thread>

Watchdog::Watchdog(Callback cb, int timeout) : callback(cb), timeout_ms(timeout), buffer_count(0), running(false) {}

Watchdog::~Watchdog() {
    stop();
}

void Watchdog::notify_buffer() {
    buffer_count++;
}

void Watchdog::start() {
    running = true;
    watchdog_thread = std::thread(&Watchdog::run, this);
}

void Watchdog::stop() {
    running = false;
    if (watchdog_thread.joinable()) {
        watchdog_thread.join();
    }
}

void Watchdog::run() {
    while (running) {
        int count_before = buffer_count.load();
        std::this_thread::sleep_for(std::chrono::milliseconds(timeout_ms));

        int count_after = buffer_count.load();
        if (count_after == count_before) {
            // No se recibieron buffers en el timeout
            callback(true); // mostrar pantalla negra
        } else {
            callback(false); // stream activo, restaurar video
        }
    }
}
