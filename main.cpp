#include <gtk/gtk.h>
#include "StreamSlot.h"
#include <vector>
#include <memory>

enum class StreamMode {
    SRT_MOSAIC,
    UDP_SAFE,   // antes UDP_MULTISTREAM
    UDP_FAST,   // antes UDP_SINGLE
};

struct AppData {
    GtkWidget *window;
    GtkWidget *grid;

    int active_slots = 1;
    bool layout_locked = false;
    bool is_fullscreen = false;

    StreamMode mode = StreamMode::SRT_MOSAIC;
    std::vector<std::shared_ptr<StreamSlot>> slots;
};

// ---------- FUNCIONES AUXILIARES ----------

void update_layout(AppData* app) {
    for (int i = 0; i < 6; ++i) {
        GtkWidget* w = app->slots[i]->get_widget();
        if (!GTK_IS_WIDGET(w)) {
            g_print("[ERROR] Slot %d get_widget() NO es válido\n", i + 1);
            continue;
        }

        if (i < app->active_slots) {
            gtk_widget_show(w);
        } else {
            gtk_widget_hide(w);
        }
    }
}

static void apply_black_background(GtkWidget *widget) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        R"css(
            * {
                background-color: black;
                color: white;
            }
            window, grid, frame {
                background-color: black;
                border: none;
            }
        )css",
        -1, nullptr);

    GdkScreen *screen = gtk_window_get_screen(GTK_WINDOW(widget));
    gtk_style_context_add_provider_for_screen(screen, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

// Reconstruye los pipelines dependiendo del modo
void rebuild_pipelines(AppData* app) {
    g_print("[INFO] Reconstruyendo pipelines en modo: %s\n",
            app->mode == StreamMode::SRT_MOSAIC ? "SRT Mosaico" :
            app->mode == StreamMode::UDP_SAFE? "UDP Safe" : "UDP Fast");

    std::vector<std::string> stream_ids = {
        "live.sls.com/live/stream1",
        "live.sls.com/live/stream2",
        "live.sls.com/live/stream3",
        "live.sls.com/live/stream4",
        "live.sls.com/live/stream5",
        "live.sls.com/live/stream6"
    };

    std::vector<std::string> udp_ports = {
        "5000", "5001", "5002", "5003", "5004", "5005"
    };

    for (int i = 0; i < 6; i++) {
        switch (app->mode) {
            case StreamMode::SRT_MOSAIC:
                app->slots[i]->init_with_streamid(stream_ids[i]);
                gtk_widget_show(app->slots[i]->get_widget());
                break;

        case StreamMode::UDP_SAFE:
            app->slots[i]->init_with_udp_port_safe(udp_ports[i]);
            gtk_widget_show(app->slots[i]->get_widget());
            break;

        case StreamMode::UDP_FAST:
            app->slots[i]->init_with_udp_port_fast(udp_ports[i]);
            gtk_widget_show(app->slots[i]->get_widget());
            break;
        }
    }

    update_layout(app);
}


// ---------- EVENTOS ----------

static gboolean on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data) {
    AppData *app = static_cast<AppData *>(user_data);
    guint keyval = event->keyval;

    // Bloquear layout con 'L'
    if (keyval == GDK_KEY_l || keyval == GDK_KEY_L) {
        app->layout_locked = !app->layout_locked;
        g_print("[INFO] Layout bloqueado: %s\n", app->layout_locked ? "sí" : "no");
        return TRUE;
    }

    // --- Modo UDP seguro (latencia normal) ---
    if (keyval == GDK_KEY_v || keyval == GDK_KEY_V) {
        app->mode = StreamMode::UDP_SAFE;
        app->active_slots = 6;
        rebuild_pipelines(app);
        update_layout(app);
        return TRUE;
    }

    // --- Modo UDP rápido (ultra low latency) ---
    if (keyval == GDK_KEY_u || keyval == GDK_KEY_U) {
        app->mode = StreamMode::UDP_FAST;
        app->active_slots = 6;
        rebuild_pipelines(app);
        update_layout(app);
        return TRUE;
    }

    if (keyval == GDK_KEY_m || keyval == GDK_KEY_M) {
        app->mode = StreamMode::SRT_MOSAIC;
        rebuild_pipelines(app);
        update_layout(app);
        return TRUE;
    }

    if (keyval == GDK_KEY_F11) {
        if (app->is_fullscreen) {
            gtk_window_unfullscreen(GTK_WINDOW(app->window));
            app->is_fullscreen = false;
            g_print("[INFO] Saliendo de pantalla completa\n");
        } else {
            gtk_window_fullscreen(GTK_WINDOW(app->window));
            app->is_fullscreen = true;
            g_print("[INFO] Modo pantalla completa activado\n");
        }
        return TRUE;
    }

    if (app->layout_locked)
        return TRUE;

    // Control de cantidad de slots (1–6)
    if (keyval >= GDK_KEY_1 && keyval <= GDK_KEY_6) {
        int slots = keyval - GDK_KEY_0;
        app->active_slots = slots;
        g_print("[INFO] Cambiando a %d espacios activos\n", slots);
        update_layout(app);
        return TRUE;
    }

    return FALSE;
}

// ---------- SETUP PRINCIPAL ----------

static void on_activate(GtkApplication *gtk_app, gpointer user_data) {
    AppData *app = new AppData();

    app->window = gtk_application_window_new(gtk_app);
    gtk_window_set_title(GTK_WINDOW(app->window), "Mosaico de Streams");
    gtk_window_set_default_size(GTK_WINDOW(app->window), 800, 600);
    apply_black_background(app->window);

    app->grid = gtk_grid_new();
    gtk_widget_set_hexpand(app->grid, TRUE);
    gtk_widget_set_vexpand(app->grid, TRUE);
    gtk_container_add(GTK_CONTAINER(app->window), app->grid);

    // Posiciones para 6 slots
    const std::vector<std::pair<int, int>> posiciones = {
        {0, 0}, {0, 1},
        {1, 0}, {1, 1},
        {2, 0}, {2, 1}
    };

    // Crear los slots iniciales
    for (int i = 0; i < 6; i++) {
        auto slot = std::make_shared<StreamSlot>();
        app->slots.push_back(slot);

        GtkWidget* w = slot->get_widget();
        if (!w) {
            g_printerr("[ERROR] El widget del slot %d es nulo\n", i + 1);
            continue;
        }

        gtk_grid_attach(GTK_GRID(app->grid), w, posiciones[i].first, posiciones[i].second, 1, 1);
    }


    // Inicializar en modo mosaico SRT por defecto
    rebuild_pipelines(app);

    g_signal_connect(app->window, "key-press-event", G_CALLBACK(on_key_press), app);
    gtk_widget_show_all(app->window);
}

// ---------- MAIN ----------

int main(int argc, char **argv) {
    gst_init(&argc, &argv);

    GtkApplication *app = gtk_application_new("com.mosaic.streamviewer", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);

    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}
