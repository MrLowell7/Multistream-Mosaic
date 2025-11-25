#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/videooverlay.h>
#include "StreamSlot.h"
#include "Watchdog.h"  // Incluye el header de watchdog

// Callback del bus GStreamer para mensajes de error y EOS
static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError *err;
            gchar *debug;
            gst_message_parse_error(msg, &err, &debug);
            g_printerr("[GStreamer error] %s\n", err->message);
            g_error_free(err);
            g_free(debug);
            break;
        }
        case GST_MESSAGE_EOS:
            g_print("[GStreamer] End of stream\n");
            break;
        default:
            break;
    }
    return TRUE;
}

// Callback para el probe que cuenta buffers y notifica watchdog
static GstPadProbeReturn buffer_probe_cb(GstPad *pad, GstPadProbeInfo *info, gpointer user_data) {
    Watchdog *wd = static_cast<Watchdog *>(user_data);
    wd->notify_buffer();
    return GST_PAD_PROBE_OK;
}

StreamSlot::StreamSlot() : container(nullptr), pipeline(nullptr), video_widget(nullptr), watchdog(nullptr) {
    // Crear watchdog con callback ligado a este StreamSlot
    watchdog = new Watchdog([this](bool show_black){
        if (watchdog_enabled) {
            on_watchdog_event(show_black);
        }
    }, 5000);
}

StreamSlot::~StreamSlot() {
    // Detener watchdog para evitar callbacks mientras destruimos
    if (watchdog) {
        watchdog->stop();
        delete watchdog;
        watchdog = nullptr;
    }

    // Detener y liberar pipeline
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }

    // Remover container del padre (si lo tiene) y destruir de forma segura
    if (container) {
        GtkWidget *parent = gtk_widget_get_parent(container);
        if (parent && GTK_IS_CONTAINER(parent)) {
            // Remover del padre, esto evita que el padre intente usar un widget ya destruido
            gtk_container_remove(GTK_CONTAINER(parent), container);
        }
        if (GTK_IS_WIDGET(container)) {
            gtk_widget_destroy(container);
        }
        container = nullptr;
        video_widget = nullptr;
    }
}

// Callback del watchdog (seguro: comprobamos video_widget y GTK_IS_WIDGET)
void StreamSlot::on_watchdog_event(bool show_black) {
    g_print("[StreamSlot] Watchdog evento, show_black = %d\n", show_black);
    if (!video_widget) return;
    if (!GTK_IS_WIDGET(video_widget)) return;

    if (show_black) {
        gtk_widget_hide(video_widget);
    } else {
        gtk_widget_show(video_widget);
    }
}

// ===== Helpers internos =====
// Remueve de forma segura el video_widget actual del container (si existe)
void StreamSlot::remove_existing_video_widget() {
    if (video_widget && GTK_IS_WIDGET(video_widget)) {
        GtkWidget *parent = gtk_widget_get_parent(video_widget);
        if (parent && GTK_IS_CONTAINER(parent)) {
            // Quitar del container; esto también unparentea internamente
            gtk_container_remove(GTK_CONTAINER(parent), video_widget);
        } else {
            // Si no tiene parent pero es widget válido, destruirlo
            gtk_widget_destroy(video_widget);
        }
        video_widget = nullptr;
    }
}

// Coloca un placeholder negro (drawing area) en el container
void StreamSlot::place_black_placeholder() {
    // Primero quitar widget anterior
    remove_existing_video_widget();

    // Crear un drawing area simple (será negro por el CSS global que usas)
    GtkWidget *da = gtk_drawing_area_new();
    gtk_widget_set_hexpand(da, TRUE);
    gtk_widget_set_vexpand(da, TRUE);

    // Añadir al container
    if (!container) {
        container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_hexpand(container, TRUE);
        gtk_widget_set_vexpand(container, TRUE);
    }
    gtk_container_add(GTK_CONTAINER(container), da);
    video_widget = da;
    gtk_widget_show_all(container);
}
// ======================================================================================================================================
// === MODO SRT ===
void StreamSlot::init_with_streamid(const std::string &streamid) {
    
    watchdog_enabled = true;

    // Detener watchdog para evitar callbacks durante reconfiguración
    if (watchdog) watchdog->stop();

    // Detener y liberar pipeline previo
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }

    // Si existe video_widget previo, removerlo del container (sin destruir container)
    remove_existing_video_widget();

    // Asegurar que container exista (si fue destruido antes)
    if (!container) {
        container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_hexpand(container, TRUE);
        gtk_widget_set_vexpand(container, TRUE);
    }

    // Construir pipeline SRT
    std::string uri = "srt://172.23.193.99:8080?streamid=" + streamid;
    std::string pipeline_str =
        "srtclientsrc uri=" + uri +
        " ! decodebin ! videoconvert name=videoconvert ! gtksink name=videosink";

    GError *error = nullptr;
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (error) {
        g_printerr("[StreamSlot] Error creando pipeline SRT: %s\n", error->message);
        g_error_free(error);
        pipeline = nullptr;
        // Colocar placeholder negro para mantener UI consistente
        place_black_placeholder();
        if (watchdog) watchdog->start();
        return;
    }

    // Bus de mensajes
    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, this);
    gst_object_unref(bus);

    // Obtener el widget del gtksink (si existe)
    GstElement *videosink = gst_bin_get_by_name(GST_BIN(pipeline), "videosink");
    if (videosink) {
        g_object_get(G_OBJECT(videosink), "widget", &video_widget, NULL);
        gst_object_unref(videosink);
    } else {
        video_widget = nullptr;
    }

    if (video_widget && GTK_IS_WIDGET(video_widget)) {
        gtk_widget_set_hexpand(video_widget, TRUE);
        gtk_widget_set_vexpand(video_widget, TRUE);

        // Asegurarse de que no tenga padre
        GtkWidget *parent = gtk_widget_get_parent(video_widget);
        if (parent && GTK_IS_CONTAINER(parent)) {
            gtk_container_remove(GTK_CONTAINER(parent), video_widget);
        }

        // Añadir al container
        if (!container) {
            container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
            gtk_widget_set_hexpand(container, TRUE);
            gtk_widget_set_vexpand(container, TRUE);
        }
        gtk_container_add(GTK_CONTAINER(container), video_widget);
    } else {
        g_printerr("[StreamSlot] No se pudo obtener el widget de video (SRT)\n");
        place_black_placeholder();
    }

    // Agregar probe para watchdog (si existe videoconvert)
    GstElement* videoconvert = gst_bin_get_by_name(GST_BIN(pipeline), "videoconvert");
    if (videoconvert) {
        GstPad *sinkpad = gst_element_get_static_pad(videoconvert, "sink");
        if (sinkpad) {
            gst_pad_add_probe(sinkpad, GST_PAD_PROBE_TYPE_BUFFER, buffer_probe_cb, watchdog, NULL);
            gst_object_unref(sinkpad);
        }
        gst_object_unref(videoconvert);
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Reiniciar watchdog cuando todo esté listo
    if (watchdog) watchdog->start();

    // Mostrar container (si no está insertado en otro lado, el caller ya lo hace)
    if (container && GTK_IS_WIDGET(container))
        gtk_widget_show_all(container);
}
// ======================================================================================================================================
// === MODO UDP ===

// === MODO UDP SAFE ===
void StreamSlot::init_with_udp_port_safe(const std::string &port) {
    
    watchdog_enabled = true;

    setup_udp_pipeline(port,
        "udpsrc port=" + port + " ! "
        "application/x-rtp,media=video,encoding-name=H264,payload=96 ! "
        "rtph264depay ! h264parse ! avdec_h264 ! videoconvert name=videoconvert ! "
        "gtksink name=videosink");
}

// === MODO UDP FAST ===
void StreamSlot::init_with_udp_port_fast(const std::string &port) {
    
    watchdog_enabled = false;  // desactivar watchdog para modo ultra rápido

    setup_udp_pipeline(port,
        "udpsrc port=" + port + " buffer-size=200000 ! "
        "application/x-rtp,media=video,encoding-name=H264,payload=96 ! "
        "rtpjitterbuffer latency=20 drop-on-latency=false ! "
        "rtph264depay ! h264parse ! avdec_h264 ! videoconvert ! "
        "gtksink sync=false max-lateness=0 qos=false name=videosink");
}


void StreamSlot::setup_udp_pipeline(const std::string &port, const std::string &pipeline_str) {
    if (watchdog) watchdog->stop();
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }

    remove_existing_video_widget();

    if (!container) {
        container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_hexpand(container, TRUE);
        gtk_widget_set_vexpand(container, TRUE);
    }

    GError *error = nullptr;
    pipeline = gst_parse_launch(pipeline_str.c_str(), &error);
    if (error) {
        g_printerr("[StreamSlot] Error creando pipeline UDP: %s\n", error->message);
        g_error_free(error);
        pipeline = nullptr;
        place_black_placeholder();
        if (watchdog) watchdog->start();
        return;
    }

    GstBus *bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, bus_call, this);
    gst_object_unref(bus);

    GstElement *videosink = gst_bin_get_by_name(GST_BIN(pipeline), "videosink");
    if (videosink) {
        g_object_get(G_OBJECT(videosink), "widget", &video_widget, NULL);
        gst_object_unref(videosink);
    } else {
        video_widget = nullptr;
    }

    if (video_widget && GTK_IS_WIDGET(video_widget)) {
        gtk_widget_set_hexpand(video_widget, TRUE);
        gtk_widget_set_vexpand(video_widget, TRUE);
        GtkWidget *parent = gtk_widget_get_parent(video_widget);
        if (parent && GTK_IS_CONTAINER(parent))
            gtk_container_remove(GTK_CONTAINER(parent), video_widget);
        gtk_container_add(GTK_CONTAINER(container), video_widget);
    } else {
        g_printerr("[StreamSlot] No se pudo obtener el widget de video (UDP)\n");
        place_black_placeholder();
    }

    GstElement *videoconvert = gst_bin_get_by_name(GST_BIN(pipeline), "videoconvert");
    if (videoconvert) {
        GstPad *sinkpad = gst_element_get_static_pad(videoconvert, "sink");
        if (sinkpad) {
            gst_pad_add_probe(sinkpad, GST_PAD_PROBE_TYPE_BUFFER, buffer_probe_cb, watchdog, NULL);
            gst_object_unref(sinkpad);
        }
        gst_object_unref(videoconvert);
    }

    gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (watchdog) watchdog->start();

    if (container && GTK_IS_WIDGET(container))
        gtk_widget_show_all(container);
}
// ======================================================================================================================================

// ===== modo "pantalla negra" para slots inactivos =====
void StreamSlot::init_with_black_screen() {
    // Detener pipeline y watchdog
    if (watchdog) watchdog->stop();
    if (pipeline) {
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
        pipeline = nullptr;
    }

    // Reemplazar widget por placeholder negro
    place_black_placeholder();

    if (watchdog) watchdog->start();
}

// Obtener widget contenedor (si no existe, crearlo)
GtkWidget* StreamSlot::get_widget() {
    if (!container) {
        container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_widget_set_hexpand(container, TRUE);
        gtk_widget_set_vexpand(container, TRUE);
        // colocar placeholder vacío hasta inicializar pipeline
        place_black_placeholder();
    }

    GtkWidget *parent = gtk_widget_get_parent(container);
    g_print("[StreamSlot::get_widget] Devuelvo widget %p con padre %p (%s)\n",
            container,
            parent,
            parent ? G_OBJECT_TYPE_NAME(parent) : "NULL");
    return container;
}
