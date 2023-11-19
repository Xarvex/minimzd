#include "args.hpp"
#include "context.h"
#include "key.h"
#include "util.h"
#include "window.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ANSI_RED     "\x1b[31m"
#define ANSI_GREEN   "\x1b[32m"
#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN    "\x1b[36m"
#define ANSI_RESET   "\x1b[0m"

pid_t mzd_fork_execute(const char **command, const bool silent) {
    pid_t pid = fork();
    if (pid < 0)
        perror("Could not fork process");
    else if (!pid) {
        if (silent) {
            const int fd = open("/dev/null", O_WRONLY);
            dup2(fd, 1);
            dup2(fd, 2);
            close(fd);
        }
        // does not modify command
        execvp(command[0], (char *const *) command);
        perror("Could not execute command");
    } else
        return pid;
    abort();
}

bool mzd_window_filter_call_pid(const struct MzdWindow *window, int *pid) {
    return window->pid == *pid;
}

bool mzd_window_filter_call_class_instance(const struct MzdWindow *window, char **window_class_instance) {
    return !strcmp(window->class_instance, *window_class_instance);
}

bool mzd_window_filter_call_class(const struct MzdWindow *window, char **window_class) {
    return !strcmp(window->class, *window_class);
}

int main(const int argc, const char **argv) {
    struct MzdContext c = mzd_context_default;
    mzd_args_populate_context(&c, argc, argv);
    if (c.help)
        return 0;

    const bool color = isatty(fileno(stdout));

    struct MzdWindowManipulator wm = mzd_window_manipulator_default;
    mzd_window_manipulator_dbus_connect(&wm);

    if (c.list) {
        const struct MzdWindow **windows = mzd_window_manipulator_list(&wm);
        for (int i = 0; windows[i]; i++) {
            const struct MzdWindow *window = windows[i];
            printf(
                "%sClass: %s%s%s\nClass Instance: %s%s%s\nPID: %s%i%s\nID: %s%u%s\n",
                i ? "\n" : "",
                color ? ANSI_BLUE : "",
                window->class,
                color ? ANSI_RESET : "",
                color ? ANSI_BLUE : "",
                window->class_instance,
                color ? ANSI_RESET : "",
                color ? ANSI_MAGENTA: "",
                window->pid,
                color ? ANSI_RESET : "",
                color ? ANSI_GREEN : "",
                window->id,
                color ? ANSI_RESET : ""
            );
        }

        mzd_windowv_free(windows);
    }

    if (1) {
        printf("flags: %d\n", c.flags);
        printf("extract_keybind: %d\n", c.extract_keybind);
        printf("match_pid: %d\n", c.match_pid);
        printf("match_process_name: %d\n", c.match_process_name);
        printf("pid: %d\n", c.pid);
        printf("process-name: %s\n", c.process_name);
        printf("window-class-instance: %s\n", c.window_class_instance);
        printf("window_class: %s\n", c.window_class);
        printf("command:");
        if (c.command) {
            for (int i = 0; c.command[i]; i++)
                printf(" %s", c.command[i]);
        }
        puts("");
    }

    pid_t pid = 0;
    if (c.command) {
        pid = mzd_fork_execute(c.command, c.background);
        if (c.match_pid)
            c.pid = pid;
    }

    if (c.extract_keybind) {
        mzd_window_manipulator_setting_connect(&wm);
        mzd_window_manipulator_setting_keybind(&wm);
    }

    if (mzd_flags_has(c.flags, MZD_KEYBIND))
        mzd_window_manipulator_uinput_attach(&wm);

    struct MzdWindowFilter *window_filter = 0;
    if (c.pid)
        window_filter = mzd_window_filter_init(&mzd_window_filter_call_pid, &c.pid);
    else if (c.window_class_instance)
        window_filter = mzd_window_filter_init(&mzd_window_filter_call_class_instance, &c.window_class_instance);
    else if (c.window_class)
        window_filter = mzd_window_filter_init(&mzd_window_filter_call_class, &c.window_class);

    if (window_filter)
        mzd_window_manipulator_match(&wm, window_filter, c.flags);

    mzd_window_manipulator_free(&wm);
    mzd_context_free(&c);

    int status = 0;
    if (pid && !c.background)
        waitpid(pid, &status, 0);
    exit(status);
}
