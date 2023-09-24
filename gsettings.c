#include <gio/gio.h>
#include "gsettings.h"
#include <stdlib.h>

static const char KEYBINDINGS[] = "org.gnome.desktop.wm.keybindings";

char** mzd_keybinds(const char *keybind) {
    GSettings *settings;
    gchar** keybinds;

    settings = g_settings_new(KEYBINDINGS);
    keybinds = g_settings_get_strv(settings, keybind);

    return keybinds;
}

char** mzd_keybinds_minimize() {
    return mzd_keybinds("minimize");
}

char** mzd_keybinds_close() {
    return mzd_keybinds("close");
}
