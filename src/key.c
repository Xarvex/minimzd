#include "key.h"
#include "util.h"
#include <ctype.h>
#include <fcntl.h>
#include <keymap.h>
#include <linux/input-event-codes.h>
#include <linux/uinput.h>

const mzd_key mzd_keycode(const char *key_name) {
    if (!strncmp (key_name,"XF86", 4))
        key_name += 4;

    int c = strlen(key_name);
    char *name = malloc(c + 1);
    name[c] = 0;
    for(int i = 0; i < c; i++)
        name[i] = tolower(key_name[i]);

    const char **result = bsearch(&name, mzd_keymap_names, MZD_KEYMAP_LEN, sizeof(char **), mzd_strptr_cmp);
    if (name)
        free(name);

    if (result)
        return mzd_keymap_codes[(result - mzd_keymap_names)];
    return 0;
}

void mzd_unsafe_key_emit(const int fd, const int type, const mzd_key keycode, const int val) {
    struct input_event ie = { .type= type, keycode, val, .time = { 0, 0 } };
    write(fd, &ie, sizeof(ie));
}

void mzd_key_emit(const int fd, const mzd_key keycode, bool state) {
    mzd_unsafe_key_emit(fd, EV_KEY, keycode, state);
}

void mzd_key_press(const int fd, const mzd_key keycode) {
    mzd_key_emit(fd, keycode, true);
}

void mzd_key_unpress(const int fd, const mzd_key keycode) {
    mzd_key_emit(fd, keycode, false);
}

void mzd_key_report(const int fd) {
    mzd_unsafe_key_emit(fd, EV_SYN, SYN_REPORT, 0);
}



const mzd_key *mzd_keybind_keycode(const char *keybind_str) {
    char *s = strdup(keybind_str);
    char *p = s;

    int c = 0;
    char *last = p;
    while ((p = strchr(p, '>'))) {
        if (strlen(p) > 1) {
            *p++ = 0;
            c++;
        } else
            *p = 0;
        last = p;
    }
    if (last)
        c++;

    mzd_key *keybind = malloc((c + 1) * sizeof(mzd_key));
    p = s;
    for (int i = 0; i < c - (last ? 1 : 0); i++) {
        keybind[i] = mzd_keycode(strchr(p, '<') + 1);
        p = p + strlen(p) + 1;
    }
    if (last)
        keybind[c - 1] = mzd_keycode(last);
    keybind[c] = 0;

    free(s);
    return keybind;
}



const mzd_key **mzd_unsafe_keybindv_extract(GSettings *settings, const char *keybind) {
    char **keybinds_strv = g_settings_get_strv(settings, keybind);
    const int c = g_strv_length(keybinds_strv);
    if (c) {
        const mzd_key **keybindv = malloc((c + 1) * sizeof(mzd_key *));
        for (int i = 0; i < c; i++)
            keybindv[i] = mzd_keybind_keycode(keybinds_strv[i]);
        keybindv[c] = 0;

        // yes, separate path, but cleanest way to return as const
        mzd_strv_free(keybinds_strv);
        return keybindv;
    }

    mzd_strv_free(keybinds_strv);
    return 0;
}

const mzd_key **mzd_unsafe_keybindv_extract_minimize(GSettings *settings) {
    return mzd_unsafe_keybindv_extract(settings, "minimize");
}

const mzd_key **mzd_unsafe_keybindv_extract_close(GSettings *settings) {
    return mzd_unsafe_keybindv_extract(settings, "close");
}



const mzd_key **mzd_keybindv_extract(const char *keybind_str) {
    GSettings *settings = mzd_keybind_settings();
    const mzd_key **keybindv = mzd_unsafe_keybindv_extract(settings, keybind_str);

    g_object_unref(settings);
    return keybindv;
}

const mzd_key ** mzd_keybindv_extract_minimize() {
    return mzd_keybindv_extract("minimize");
}

const mzd_key ** mzd_keybindv_extract_close() {
    return mzd_keybindv_extract("close");
}

void mzd_keybindv_free(const mzd_key **keybindv) {
    mzd_ptrptr_free((void **) keybindv);
}



const int mzd_uinput_connect() {
    return open("/dev/uinput", O_WRONLY | O_NONBLOCK);
}

void mzd_uinput_prepare(const int fd, const mzd_key *keybind) {
    ioctl(fd, UI_SET_EVBIT, EV_KEY);

    for (int i = 0; keybind[i]; i++)
        ioctl(fd, UI_SET_KEYBIT, keybind[i]);
}

void mzd_uinput_setup(const int fd) {
    struct uinput_setup usetup;
    memset(&usetup, 0, sizeof(usetup));

    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x5676;  // "Xv"
    usetup.id.product = 0x6D64; // "md"
    strncpy(usetup.name, "minimzd uinput", UINPUT_MAX_NAME_SIZE);

    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);
}

void mzd_uinput_use(const int fd, const mzd_key *keybind) {
    sleep(1);

    int c = 0;
    for (; keybind[c]; c++)
        mzd_key_press(fd, keybind[c]);
    mzd_key_report(fd);

    for (int i = c; i > 0; --i)
        mzd_key_unpress(fd, keybind[i]);
    mzd_key_report(fd);

    sleep(1);
}

void mzd_uinput_free(const int fd) {
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
}
