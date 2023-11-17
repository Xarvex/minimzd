#include "args.hpp"
#include <cxxopts.hpp>
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif

#include "context.h"
#include "util.h"

void mzd_args_populate_context(struct MzdContext *context, const int argc, const char **argv) {
    cxxopts::Options options(
        "minimzd",
        "Simple utility for launch programs minimized or closed, when they otherwise don't like to"
    );

    options.add_options()
        ("command", "Command to be executed minimized", cxxopts::value<std::string>()->default_value(""))
        ("help,h", "Display usage", cxxopts::value<bool>()->default_value("false"))
        ("list,l", "Lists all detectable windows", cxxopts::value<bool>()->default_value("false"))
        ("first,F", "Stop after the first match is minimized", cxxopts::value<bool>()->default_value("false"))
        ("verify,V", "Verify that the window was actually minimized", cxxopts::value<bool>()->default_value("false"))
        ("background,B", "Run command in background and exit early", cxxopts::value<bool>()->default_value("false"))
        ("close-window,x", "Close the window, useful for applications that run in background", cxxopts::value<bool>()->default_value("false"))
        ("keybind,k", "Use a keybind for specified action, which may give alternative behavior", cxxopts::value<bool>()->default_value("false"))
        ("extract-keybind,e", "Extracts any needed keybind from GNOME settings", cxxopts::value<bool>()->default_value("false"))
        ("match-pid", "Match the pid of the executed command, must be a unique instance to function", cxxopts::value<bool>()->default_value("false"))
        ("match-process-name", "Match the process name", cxxopts::value<bool>()->default_value("false"))
        ("pid", "Pid to match, takes precedence over process name", cxxopts::value<int>()->default_value("0"))
        ("process-name", "Process name to match, takes precedence over window class", cxxopts::value<std::string>())
        ("window-class-instance", "Window class instance to match, takes precedence over window class", cxxopts::value<std::string>())
        ("window-class", "Window class to match, lowest precedence", cxxopts::value<std::string>())
        ("timeout,t", "Allowed amount of time to run", cxxopts::value<std::string>())
    ;
    options.parse_positional({ "command" });

    const auto result = options.parse(argc, argv);

    if (result.count("help")) {
        context->help = true;
        std::cout << options.help() << std::endl;
        return;
    }

    if (result.count("command"))
        context->command = mzd_str_split(result["command"].as<std::string>().c_str(), ' ', &context->command_ptr);
    context->first = result["first"].as<bool>();
    context->list = result["list"].as<bool>();
    context->verify = result["verify"].as<bool>();
    context->background = result["background"].as<bool>();
    context->close_window = result["close-window"].as<bool>();
    context->keybind = result["keybind"].as<bool>();
    context->extract_keybind = result["extract-keybind"].as<bool>();
    context->match_pid = result["match-pid"].as<bool>();
    context->match_process_name = result["match-process-name"].as<bool>();
    context->pid = result["pid"].as<int>();
    const char *fallback = context->command ? context->command[0] : 0;
    const char *process_name = result.count("process-name") ? result["process-name"].as<std::string>().c_str() : fallback;
    const char *window_class_instance = result.count("window-class-instance") ? result["window-class-instance"].as<std::string>().c_str() : fallback;
    const char *window_class = result.count("window-class") ? result["window-class"].as<std::string>().c_str() : fallback;
    context->process_name = process_name ? strdup(process_name) : 0;
    context->window_class_instance = window_class_instance ? strdup(window_class_instance) : 0;
    context->window_class = window_class ? strdup(window_class) : 0;
}

#ifdef __cplusplus
}
#endif
