#include "window.h"
#include "key.h"
#include "util.h"
#include <dbus/dbus.h>
#include <stdio.h>
#include <string.h>
#include <yyjson.h>

void mzd_dbus_error_guard(DBusError *error) {
    if (dbus_error_is_set(error)) {
        fprintf(stderr, "%s", error->message);
        dbus_error_free(error);
        abort();
    }
}

DBusMessage *mzd_dbus_call_create(const char *method) {
    DBusMessage *query = dbus_message_new_method_call(
        "org.gnome.Shell",
        "/org/gnome/Shell/Extensions/Windows",
        "org.gnome.Shell.Extensions.Windows",
        method
    );

    return query;
}

DBusMessage *mzd_dbus_call_create_with_window(const char *method, const struct MzdWindow *window) {
    DBusMessage *query = mzd_dbus_call_create(method);
    dbus_message_append_args(query, DBUS_TYPE_UINT32, &window->id, DBUS_TYPE_INVALID);

    return query;
}



void mzd_window_free(const struct MzdWindow *window) {
    free((void *) window->class);
    free((void *) window->class_instance);
    free((void *) window);
}

void mzd_window_arr_free(const struct MzdWindow **windows) {
    for (int i = 0; windows[i]; i++)
        mzd_window_free(windows[i]);
    free(windows);
}

bool mzd_window_filter(struct MzdWindowFilter *window_filter, const struct MzdWindow *window) {
    return window_filter->call(window, window_filter->value);
}



void mzd_window_manipulator_dbus_connect(struct MzdWindowManipulator *window_manipulator) {
    DBusError dbus_error;
    dbus_error_init(&dbus_error);

    DBusConnection *connection = dbus_bus_get_private(DBUS_BUS_SESSION, &dbus_error);
    window_manipulator->dbus = connection;
    mzd_dbus_error_guard(&dbus_error);
}

void mzd_window_manipulator_setting_connect(struct MzdWindowManipulator *window_manipulator) {
}

void mzd_window_manipulator_setting_keybind(struct MzdWindowManipulator *window_manipulator) {
    GSettings *settings = mzd_keybind_settings();

    const mzd_key **minimize_keybindv = mzd_unsafe_keybindv_extract_minimize(settings);
    window_manipulator->minimize_keybind = minimize_keybindv[0];
    for (int i = 1; minimize_keybindv[i]; i++)
        free((void *) minimize_keybindv[i]);
    free(minimize_keybindv);

    const mzd_key **close_keybindv = mzd_unsafe_keybindv_extract_close(settings);
    window_manipulator->close_keybind = close_keybindv[0];
    for (int i = 1; close_keybindv[i]; i++)
        free((void *) close_keybindv[i]);
    free(close_keybindv);

    g_object_unref(settings);
}



void mzd_window_manipulator_input_attach(struct MzdWindowManipulator *window_manipulator) {
    window_manipulator->fd = mzd_uinput_connect();
    mzd_uinput_prepare(window_manipulator->fd, window_manipulator->minimize_keybind);
    mzd_uinput_prepare(window_manipulator->fd, window_manipulator->close_keybind);
    mzd_uinput_setup(window_manipulator->fd);
}

void mzd_window_manipulator_keybind_use_minimize(const struct MzdWindowManipulator *window_manipulator) {
    mzd_uinput_use(window_manipulator->fd, window_manipulator->minimize_keybind);
}



DBusMessage *mzd_unsafe_window_manipulator_dbus_call_send(const struct MzdWindowManipulator *window_manipulator, DBusMessage *query) {
    DBusError dbus_error;
    dbus_error_init(&dbus_error);

    DBusMessage *response = dbus_connection_send_with_reply_and_block(
        window_manipulator->dbus,
        query,
        500,
        &dbus_error
    );
    dbus_connection_flush(window_manipulator->dbus);
    mzd_dbus_error_guard(&dbus_error);

    dbus_error_free(&dbus_error);
    return response;
}

const struct MzdWindow **mzd_unsafe_window_manipulator_call_list(const struct MzdWindowManipulator *window_manipulator, DBusMessage *query) {
    DBusError dbus_error;
    dbus_error_init(&dbus_error); 

    DBusMessage *response =  mzd_unsafe_window_manipulator_dbus_call_send(window_manipulator, query);
    const char *windows_str = 0;
    dbus_message_get_args(response, &dbus_error, DBUS_TYPE_STRING, &windows_str, DBUS_TYPE_INVALID);
    yyjson_doc *const doc = yyjson_read(windows_str, strlen(windows_str), 0);
    yyjson_val *const root = yyjson_doc_get_root(doc);

    yyjson_arr_iter arr_iter = yyjson_arr_iter_with(root);
    yyjson_val *obj;
    const size_t count = yyjson_arr_size(root);
    const struct MzdWindow **windows = malloc((count + 1) * sizeof(struct MzdWindow *));
    while ((obj = yyjson_arr_iter_next(&arr_iter))) {
        yyjson_obj_iter obj_iter = yyjson_obj_iter_with(obj);
        yyjson_val *key, *val;
        struct MzdWindow *const window = malloc(sizeof(struct MzdWindow));
        while ((key = yyjson_obj_iter_next(&obj_iter))) {
            val = yyjson_obj_iter_get_val(key);
            const char *key_str = key->uni.str;

            switch (mzd_str_djb2(key_str)) {
                case MZD_DJB2_IN_CURRENT_WORKSPACE:
                    window->in_current_workspace = unsafe_yyjson_get_bool(val);
                    break;
                case MZD_DJB2_WM_CLASS:
                    window->class = strdup(unsafe_yyjson_get_str(val));
                    break;
                case MZD_DJB2_WM_CLASS_INSTANCE:
                    window->class_instance = strdup(unsafe_yyjson_get_str(val));
                    break;
                case MZD_DJB2_PID:
                    window->pid = unsafe_yyjson_get_int(val);
                    break;
                case MZD_DJB2_ID:
                    window->id = unsafe_yyjson_get_uint(val);
                    break;
                case MZD_DJB2_FRAME_TYPE:
                    window->frame_type = unsafe_yyjson_get_uint(val);
                    break;
                case MZD_DJB2_WINDOW_TYPE:
                    window->type = unsafe_yyjson_get_uint(val);
                    break;
                case MZD_DJB2_FOCUS:
                    window->focus = unsafe_yyjson_get_bool(val);
                    break;
            }
        }
        windows[arr_iter.idx - 1] = window;
    }
    windows[count] = 0;

    yyjson_doc_free(doc);
    dbus_message_unref(response);
    dbus_error_free(&dbus_error);
    return windows;
}

void mzd_unsafe_window_manipulator_call(const struct MzdWindowManipulator *window_manipulator, DBusMessage *query) {
    dbus_message_unref(mzd_unsafe_window_manipulator_dbus_call_send(window_manipulator, query));
}

void mzd_unsafe_window_manipulator_call_with_window(const struct MzdWindowManipulator *window_manipulator, const char *method, const struct MzdWindow *window) {
    DBusMessage *query = mzd_dbus_call_create_with_window(method, window);
    mzd_unsafe_window_manipulator_call(window_manipulator, query);

    dbus_message_unref(query);
}



const struct MzdWindow **mzd_window_manipulator_list(const struct MzdWindowManipulator *window_manipulator) {
    DBusMessage *query = mzd_dbus_call_create("List");
    const struct MzdWindow **windows = mzd_unsafe_window_manipulator_call_list(window_manipulator, query);

    dbus_message_unref(query);
    return windows;
}

void mzd_window_manipulator_minimize(const struct MzdWindowManipulator *window_manipulator, const struct MzdWindow *window) {
    mzd_unsafe_window_manipulator_call_with_window(window_manipulator, "Minimize", window);
}

void mzd_window_manipulator_focus(const struct MzdWindowManipulator *window_manipulator, const struct MzdWindow *window) {
    mzd_unsafe_window_manipulator_call_with_window(window_manipulator, "Activate", window);
}

void mzd_window_manipulator_match(const struct MzdWindowManipulator *window_manipulator, struct MzdWindowFilter *window_filter) {
    const struct MzdWindow **windows = mzd_window_manipulator_list(window_manipulator);
    if (window_filter)
        for (int i = 0; windows[i]; i++)
            if (mzd_window_filter(window_filter, windows[i]))
                mzd_window_manipulator_minimize(window_manipulator, windows[i]);

    mzd_window_arr_free(windows);
}



void mzd_window_manipulator_free(const struct MzdWindowManipulator *window_manipulator) {
    dbus_connection_flush(window_manipulator->dbus);
    dbus_connection_close(window_manipulator->dbus);
    dbus_connection_unref(window_manipulator->dbus);
    dbus_shutdown();
    if (window_manipulator->minimize_keybind)
        free((void *)window_manipulator->minimize_keybind);
    if (window_manipulator->close_keybind)
        free((void *)window_manipulator->close_keybind);
    if (window_manipulator->fd)
        mzd_uinput_free(window_manipulator->fd);
}
