#ifndef MZD_WINDOW_H
#define MZD_WINDOW_H

#define __GIO_GIO_H_INSIDE__

#include "key.h"
#include <dbus/dbus.h>

struct MzdWindow {
    bool in_current_workspace;
    const char *class;
    const char *class_instance;
    pid_t pid;
    unsigned int id;
    unsigned int frame_type;
    unsigned int type;
    bool focus;
};

struct MzdWindowManipulator {
    DBusConnection *dbus;
    GSettings *gsettings;
    const mzd_key *minimize_keybind;
    const mzd_key *close_keybind;
    int fd;
};

static const struct MzdWindowManipulator mzd_window_manipulator_default = { 0 };

typedef bool (*MzdWindowFilterCall)(const struct MzdWindow *, void *);

struct MzdWindowFilter {
    MzdWindowFilterCall call;
    void *value;
};

#define mzd_window_filter_init(call, value) &(struct MzdWindowFilter) { (MzdWindowFilterCall) call, value }

DBusMessage *mzd_dbus_call_create(const char *method);
DBusMessage *mzd_dbus_call_create_with_window(const char *method, const struct MzdWindow *window);

void mzd_window_free(const struct MzdWindow *window);
void mzd_window_arr_free(const struct MzdWindow **windows);
bool mzd_window_filter(struct MzdWindowFilter *window_filter, const struct MzdWindow *window);

void mzd_window_manipulator_dbus_connect(struct MzdWindowManipulator *window_manipulator);
void mzd_window_manipulator_setting_connect(struct MzdWindowManipulator *window_manipulator);
void mzd_window_manipulator_setting_keybind(struct MzdWindowManipulator *window_manipulator);

void mzd_window_manipulator_uinput_attach(struct MzdWindowManipulator *window_manipulator);
void mzd_window_manipulator_uinput_use_minimize(const struct MzdWindowManipulator *window_manipulator);

DBusMessage *mzd_unsafe_window_manipulator_dbus_call_send(const struct MzdWindowManipulator *window_manipulator, DBusMessage *query);
const struct MzdWindow **mzd_unsafe_window_manipulator_call_list(const struct MzdWindowManipulator *window_manipulator, DBusMessage *query);
void mzd_unsafe_window_manipulator_call(const struct MzdWindowManipulator *window_manipulator, DBusMessage *query);
void mzd_unsafe_window_manipulator_call_with_window(const struct MzdWindowManipulator *window_manipulator, const char *method, const struct MzdWindow *window);

const struct MzdWindow **mzd_window_manipulator_list(const struct MzdWindowManipulator *window_manipulator);
void mzd_window_manipulator_minimize(const struct MzdWindowManipulator *window_manipulator, const struct MzdWindow *window);
void mzd_window_manipulator_focus(const struct MzdWindowManipulator *window_manipulator, const struct MzdWindow *window);
void mzd_window_manipulator_match(const struct MzdWindowManipulator *window_manipulator, struct MzdWindowFilter *window_filter);

void mzd_window_manipulator_free(const struct MzdWindowManipulator *window_manipulator);

#endif
