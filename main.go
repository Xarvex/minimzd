package main

// #cgo pkg-config: glibmm-2.4 giomm-2.4
// #include <gio/gio.h>
import "C"

import (
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
	"strings"
	"time"
	"unsafe"

	"github.com/bendahl/uinput"
	"github.com/godbus/dbus/v5"
	"github.com/spf13/pflag"
)

const dbusDest = "org.gnome.Shell"
const dbusObject = "/org/gnome/Shell/Extensions/Windows"
const dbusMethods = "org.gnome.Shell.Extensions.Windows"

const defaultTimeout = 10 * time.Second

func ExtractCloseKeybind() []int {
	settings := C.g_settings_new(C.CString("org.gnome.desktop.wm.keybindings"))
	nativeKeybindStrings := C.g_settings_get_strv(settings, C.CString("close"))
	nativeKeybindString := (*[1 << 30]*C.char)(unsafe.Pointer(nativeKeybindStrings))[0]
	var keybind []int
	if nativeKeybindString != nil {
		keybindString := C.GoString(nativeKeybindString)
		keyStrings := strings.FieldsFunc(keybindString, func(r rune) bool {
			return strings.ContainsRune("<>", r)
		})
		keybind = make([]int, len(keyStrings))
		for i, keyString := range keyStrings {
			var key int
			switch keyString {
			case "Alt":
				key = uinput.KeyLeftalt
			case "F4":
				key = uinput.KeyF4
			}
			keybind[i] = key
		}
	}
	C.free(unsafe.Pointer(nativeKeybindString))
	C.free(unsafe.Pointer(nativeKeybindStrings))
	return keybind
}

type Window struct {
	InCurrentWorkspace bool   `json:"in_current_workspace"`
	Class              string `json:"wm_class"`
	ClassInstance      string `json:"wm_class_instance"`
	Pid                int    `json:"pid"`
	Id                 uint   `json:"id"`
	FrameType          int    `json:"frame_type"`
	Type               int    `json:"window_type"`
	Focus              bool   `json:"focus"`
}

type WindowManipulator struct {
	DbusConnection *dbus.Conn
	Keyboard       uinput.Keyboard
	CloseKeybind   []int
}

func (m *WindowManipulator) Connect(attachKeyboard bool) error {
	var err error

	m.DbusConnection, err = dbus.ConnectSessionBus()
	if err != nil {
		return fmt.Errorf("failed to connect to dbus session: %w", err)
	}

	if attachKeyboard {
		m.Keyboard, err = uinput.CreateKeyboard("/dev/uinput", []byte("minimzd"))
		if err != nil {
			return err
		}
	}

	return nil
}

func (m *WindowManipulator) ExtractCloseKeybind() {
	m.CloseKeybind = ExtractCloseKeybind()
}

func (m *WindowManipulator) call(method string, args ...interface{}) (string, error) {
	obj := m.DbusConnection.Object(dbusDest, dbusObject)
	call := obj.Call(dbusMethods+"."+method, 0, args...)
	if call.Err != nil {
		return "", fmt.Errorf("failed to call function, make sure the GNOME extension is installed and enabled: %w", call.Err)
	}

	var output string
	if len(call.Body) > 0 {
		if err := call.Store(&output); err != nil {
			return "", fmt.Errorf("failed to extract value from call: %w", err)
		}
	}

	return output, nil
}

func (m *WindowManipulator) List() ([]*Window, error) {
	output, err := m.call("List")
	if err != nil {
		return nil, err
	}

	var windows []*Window
	if err := json.Unmarshal([]byte(output), &windows); err != nil {
		return nil, fmt.Errorf("failed to unmarshal JSON output: %w", err)
	}

	return windows, nil
}

func (m *WindowManipulator) Minimize(window *Window, closeWindow bool) error {
	if closeWindow {
		if _, err := m.call("Activate", window.Id); err != nil {
			return err
		}

		keybind := m.CloseKeybind
		if keybind == nil {
			keybind = []int{uinput.KeyLeftalt, uinput.KeyF4}
		}
		if len(keybind) > 0 {
			for _, key := range keybind[:len(keybind)-1] {
				if err := m.Keyboard.KeyDown(key); err != nil {
					return err
				}
			}
			if err := m.Keyboard.KeyPress(keybind[len(keybind)-1]); err != nil {
				return err
			}
			for i := range keybind[:len(keybind)-1] {
				if err := m.Keyboard.KeyUp(keybind[len(keybind)-2-i]); err != nil {
					return err
				}
			}
		}
	} else {
		if _, err := m.call("Minimize", window.Id); err != nil {
			return err
		}
	}

	return nil
}

func (m *WindowManipulator) MinimizeFocused(windows []*Window, closeWindow bool) ([]*Window, error) {
	if windows == nil {
		var err error
		windows, err = m.List()
		if err != nil {
			return nil, err
		}
	}

	var minimized []*Window
	for _, window := range windows {
		if closeWindow || window.Focus {
			if err := m.Minimize(window, closeWindow); err != nil {
				return minimized, err
			}
			minimized = append(minimized, window)
		}
	}

	return minimized, nil
}

func (m *WindowManipulator) MinimizeAll(windows []*Window, closeWindow bool) ([]*Window, error) {
	if windows == nil {
		var err error
		windows, err = m.List()
		if err != nil {
			return nil, err
		}
	}

	var minimized []*Window
	for _, window := range windows {
		if err := m.Minimize(window, closeWindow); err != nil {
			return minimized, err
		}
		minimized = append(minimized, window)
	}

	return minimized, nil
}

type WindowMatcher func(*Window) bool

func (m *WindowManipulator) MinimizeMatch(timeoutDuration time.Duration, windowMatcher WindowMatcher, closeWindow bool) ([]*Window, error) {
	if timeoutDuration == 0 {
		timeoutDuration = defaultTimeout
	}
	var timeout *time.Timer
	if timeoutDuration != -1 {
		timeout = time.NewTimer(timeoutDuration)
		defer timeout.Stop()
	}
	ticker := time.NewTicker(500 * time.Millisecond)

	defer ticker.Stop()

	var minimized []*Window
	for {
		select {
		case <-timeout.C:
			return minimized, nil
		case <-ticker.C:
			windows, err := m.List()
			if err != nil {
				return minimized, err
			}
			for _, window := range windows {
				if windowMatcher(window) {
					if err := m.Minimize(window, closeWindow); err != nil {
						return minimized, err
					}
					minimized = append(minimized, window)
				}
			}
		}
	}
}

func (m *WindowManipulator) MinimizeFirstMatch(timeoutDuration time.Duration, windowMatcher WindowMatcher, closeWindow bool) (*Window, error) {
	if timeoutDuration == 0 {
		timeoutDuration = defaultTimeout
	}
	var timeout *time.Timer
	if timeoutDuration != -1 {
		timeout = time.NewTimer(timeoutDuration)
		defer timeout.Stop()
	}
	ticker := time.NewTicker(500 * time.Millisecond)

	defer ticker.Stop()

	for {
		select {
		case <-timeout.C:
			return nil, nil
		case <-ticker.C:
			windows, err := m.List()
			if err != nil {
				return nil, err
			}
			for _, window := range windows {
				if windowMatcher(window) {
					if err := m.Minimize(window, closeWindow); err != nil {
						return nil, err
					}
					return window, nil
				}
			}
		}
	}
}

func (m *WindowManipulator) Unminimize(window *Window) error {
	if _, err := m.call("Activate", window.Id); err != nil {
		return err
	}

	return nil
}

func (m *WindowManipulator) UnminimizeAll(windows []*Window) ([]*Window, error) {
	if windows == nil {
		var err error
		windows, err = m.List()
		if err != nil {
			return nil, err
		}
	}

	var unminimized []*Window
	for _, window := range windows {
		if err := m.Unminimize(window); err != nil {
			return unminimized, err
		}
		unminimized = append(unminimized, window)
	}

	return unminimized, nil
}

func (m *WindowManipulator) Close() {
	m.DbusConnection.Close()
	if m.Keyboard != nil {
		m.Keyboard.Close()
	}
}

func main() {
	errExit := func(err error) {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}

	var first bool
	var closeWindow bool
	var extractKeybind bool
	var matchPid bool
	var matchProcessName bool
	var pid int
	var processName string
	var windowClassInstance string
	var windowClass string
	var timeoutDuration time.Duration
	var command string
	var commandArgs []string
	{
		var helpDefined bool
		var processNameDefined bool
		var windowClassInstanceDefined bool
		var windowClassDefined bool
		{
			pflag.BoolP("help", "h", false, "displays usage")
			helpFlag := pflag.Lookup("help")
			pflag.BoolVarP(&first, "first", "F", false, "stop after the first match is minimized")
			pflag.BoolVarP(&closeWindow, "close-window", "x", false, "uses a keyboard shortcut to close the window (useful for applications that run in background, note that focused windows will need to be temporarily minimized)")
			pflag.BoolVarP(&extractKeybind, "extract-close-keybind", "e", false, "extracts the close window keybind from GNOME settings")
			pflag.BoolVar(&matchPid, "match-pid", false, "will match the pid of the executed command (must be a unique instance to work)")
			pflag.BoolVar(&matchProcessName, "match-process-name", false, "will match the process name")
			pflag.IntVar(&pid, "pid", 0, "pid to match (for example if started from external source, will override match-pid)")
			pflag.StringVar(&processName, "process-name", "", "process name to match, takes precedence over window class (default command name, if present)")
			processNameFlag := pflag.Lookup("process-name")
			pflag.StringVar(&windowClassInstance, "window-class-instance", "", "window class instance to match, takes precedence over window class (default command name, if present)")
			windowClassInstanceFlag := pflag.Lookup("window-class-instance")
			pflag.StringVar(&windowClass, "window-class", "", "window class to match, lowest precedence (default command name, if present)")
			windowClassFlag := pflag.Lookup("window-class")
			pflag.DurationVarP(&timeoutDuration, "timeout", "t", defaultTimeout, "allowed amount of time to run")

			pflag.CommandLine.SortFlags = false
			pflag.ParseAll(func(flag *pflag.Flag, value string) error {
				switch flag {
				case helpFlag:
					helpDefined = true
				case processNameFlag:
					processNameDefined = true
				case windowClassInstanceFlag:
					windowClassInstanceDefined = true
				case windowClassFlag:
					windowClassDefined = true
				}
				return nil
			})
			pflag.Parse()
		}

		if helpDefined {
			fmt.Println(pflag.CommandLine.FlagUsages())
			return
		}

		commandArgs = strings.Split(pflag.Arg(0), " ")
		if len(commandArgs) > 0 {
			command = commandArgs[0]
			commandArgs = commandArgs[1:]
		}

		if !processNameDefined {
			processName = command
		}
		if !windowClassInstanceDefined {
			windowClassInstance = command
		}
		if !windowClassDefined {
			windowClass = command
		}
	}

	dbusManager := WindowManipulator{}
	if err := dbusManager.Connect(closeWindow); err != nil {
		errExit(err)
	}
	defer dbusManager.Close()

	var cmd *exec.Cmd
	if command != "" {
		cmd = exec.Command(command, commandArgs...)
		if err := cmd.Start(); err != nil {
			errExit(fmt.Errorf("failed to execute command: %w", err))
		}
		if matchPid && pid == 0 {
			pid = cmd.Process.Pid
		}
	}

	var tempMinimized []*Window
	if closeWindow {
		if extractKeybind {
			dbusManager.ExtractCloseKeybind()
		}

		var err error
		tempMinimized, err = dbusManager.MinimizeFocused(nil, false)
		if err != nil {
			errExit(err)
		}
	}

	var windowMatcher WindowMatcher
	if matchPid {
		windowMatcher = func(w *Window) bool {
			return w.Pid == pid
		}
	} else if windowClassInstance != "" {
		windowMatcher = func(w *Window) bool {
			return w.ClassInstance == windowClassInstance
		}
	} else if windowClass != "" {
		windowMatcher = func(w *Window) bool {
			return w.Class == windowClass
		}
	}
	if windowMatcher != nil {
		if first {
			if _, err := dbusManager.MinimizeFirstMatch(timeoutDuration, windowMatcher, closeWindow); err != nil {
				errExit(err)
			}
		} else {
			if _, err := dbusManager.MinimizeMatch(timeoutDuration, windowMatcher, closeWindow); err != nil {
				errExit(err)
			}
		}
	}

	if len(tempMinimized) > 0 {
		if _, err := dbusManager.UnminimizeAll(tempMinimized); err != nil {
			errExit(err)
		}
	}
}
