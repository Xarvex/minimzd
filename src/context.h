#ifndef MZD_CONTEXT_H
#define MZD_CONTEXT_H

#include <stdbool.h>

struct MzdContext {
    const char *help;
    bool first;
    bool list;
    bool verify;
    bool background;
    bool close_window;
    bool keybind;
    bool extract_keybind;
    bool match_pid;
    bool match_process_name;
    int pid;
    const char *process_name;
    const char *window_class_instance;
    const char *window_class;
    long timeout;
    const char **command;
    void *command_ptr;
};

static const struct MzdContext mzd_context_default = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void mzd_context_free(struct MzdContext *context);

#endif
