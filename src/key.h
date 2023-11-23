#ifndef MZD_KEY_H
#define MZD_KEY_H

#include <gio/gio.h>
#include <stdbool.h>

#define mzd_keybind_gsettings() g_settings_new("org.gnome.desktop.wm.keybindings")

typedef unsigned int mzd_key;

const mzd_key mzd_keycode(const char *key_name);

void mzd_unsafe_key_emit(const int fd, const int type, const mzd_key keycode, const int val);
void mzd_key_emit(const int fd, const mzd_key keycode, bool state);
void mzd_key_press(const int fd, const mzd_key keycode);
void mzd_key_unpress(const int fd, const mzd_key keycode);
void mzd_key_report(const int fd);

const mzd_key *mzd_keybind_keycode(const char *keybind_str);

const mzd_key **mzd_unsafe_keybindv_extract(GSettings *settings, const char *keybind);
const mzd_key **mzd_unsafe_keybindv_extract_minimize(GSettings *settings);
const mzd_key **mzd_unsafe_keybindv_extract_close(GSettings *settings);

const mzd_key **mzd_keybindv_extract(GSettings *settings, const char *keybind_str);
const mzd_key **mzd_keybindv_extract_minimize(GSettings *settings);
const mzd_key **mzd_keybindv_extract_close(GSettings *settings);

void mzd_keybindv_free(const mzd_key **keybindv);

const int mzd_uinput_connect(void);
void mzd_uinput_prepare(const int fd, const mzd_key *keybind);
void mzd_uinput_setup(const int fd);
void mzd_uinput_use(const int fd, const mzd_key *keybind);
void mzd_uinput_free(const int fd);

#endif
