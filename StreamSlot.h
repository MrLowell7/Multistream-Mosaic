#ifndef STREAMSLOT_H
#define STREAMSLOT_H

#include <gtk/gtk.h>
#include <gst/gst.h>
#include <string>
#include "Watchdog.h"

class StreamSlot {
public:
    StreamSlot();
    ~StreamSlot();

    void init(int index);
    void init_with_streamid(const std::string &streamid);
    // void init_with_udp_port(const std::string &port);
    void init_with_udp_port_safe(const std::string &port);
    void init_with_udp_port_fast(const std::string &port);

    GtkWidget* get_widget();

    bool watchdog_enabled = true;  // por defecto activo

private:
    GtkWidget* container = nullptr;
    GtkWidget* video_widget = nullptr;
    GstElement* pipeline = nullptr;
    Watchdog* watchdog = nullptr;

    void on_watchdog_event(bool show_black);
    void setup_udp_pipeline(const std::string &port, const std::string &pipeline_str);
    
    void remove_existing_video_widget();
    void place_black_placeholder();
    void init_with_black_screen();
};

#endif // STREAMSLOT_H
