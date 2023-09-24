package main

import (
	"encoding/json"
	"fmt"
	"time"

	"github.com/bendahl/uinput"
	"github.com/godbus/dbus/v5"
)

const (
	dbusDest    = "org.gnome.Shell"
	dbusObject  = "/org/gnome/Shell/Extensions/Windows"
	dbusMethods = "org.gnome.Shell.Extensions.Windows"
)

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
	DbusConnection  *dbus.Conn
	Keyboard        uinput.Keyboard
	MinimizeKeybind []int
	CloseKeybind    []int
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

func (m *WindowManipulator) ExtractKeybinds() {
	m.MinimizeKeybind = ExtractMinimizeKeybinds()[0]
	m.CloseKeybind = ExtractCloseKeybinds()[0]
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

func (m *WindowManipulator) list() (string, error) {
	return m.call("List")
}

func (m *WindowManipulator) minimize(window *Window) error {
	_, err := m.call("Minimize", window.Id)
	return err
}

func (m *WindowManipulator) activate(window *Window) error {
	_, err := m.call("Activate", window.Id)
	return err
}

func (m *WindowManipulator) close(window *Window) error {
	_, err := m.call("Close", window.Id)
	return err
}

func (m *WindowManipulator) List() ([]*Window, error) {
	output, err := m.list()
	if err != nil {
		return nil, err
	}

	var windows []*Window
	if err := json.Unmarshal([]byte(output), &windows); err != nil {
		return nil, fmt.Errorf("failed to unmarshal JSON output: %w", err)
	}

	return windows, nil
}

func (m *WindowManipulator) useKeybind(keybind []int) error {
	return useKeybind(m.Keyboard, keybind)
}

func (m *WindowManipulator) Minimize(context Context, window *Window) (bool, error) {
	if !window.Focus && context.Keybind {
		return false, nil
	}

	var keybind []int
	if !context.CloseWindow {
		if !context.Keybind {
			if err := m.minimize(window); err != nil {
				return false, err
			}
		} else {
			keybind = m.MinimizeKeybind
			if keybind == nil {
				keybind = defaultMinimizeKeybind
			}
		}
	} else {
		if !context.Keybind {
			if err := m.close(window); err != nil {
				return false, err
			}
		} else {
			keybind = m.CloseKeybind
			if keybind == nil {
				keybind = defaultCloseKeybind
			}
		}
	}

	if keybind != nil {
		if err := m.activate(window); err != nil {
			return false, err
		}
		if err := m.useKeybind(keybind); err != nil {
			return false, err
		}
	}

	closed := true
	if context.Verify {
		windows, _ := m.List()
		for _, windowCheck := range windows {
			if window.Id == windowCheck.Id {
				closed = !(context.CloseWindow || window.Focus)
				break
			}
		}
	}

	return closed, nil
}

func (m *WindowManipulator) MinimizeFocused(context Context, windows []*Window) ([]*Window, error) {
	if windows == nil {
		var err error
		windows, err = m.List()
		if err != nil {
			return nil, err
		}
	}

	var minimized []*Window
	for _, window := range windows {
		if !window.Focus && context.CloseWindow {
			if err := m.activate(window); err != nil {
				return nil, err
			}
		} else {
			if success, err := m.Minimize(context, window); err != nil {
				return minimized, err
			} else if success {
				minimized = append(minimized, window)
			}
		}
	}

	return minimized, nil
}

func (m *WindowManipulator) MinimizeAll(context Context, windows []*Window) ([]*Window, error) {
	if windows == nil {
		var err error
		windows, err = m.List()
		if err != nil {
			return nil, err
		}
	}

	var minimized []*Window
	for _, window := range windows {
		if !window.Focus && context.Keybind {
			if err := m.activate(window); err != nil {
				return nil, err
			}
		} else {
			if success, err := m.Minimize(context, window); err != nil {
				return minimized, err
			} else if success {
				minimized = append(minimized, window)
			}
		}
	}

	return minimized, nil
}

type WindowMatcher func(*Window) bool

func (m *WindowManipulator) MinimizeMatch(context Context, windowMatcher WindowMatcher) ([]*Window, error) {
	if context.Timeout == 0 {
		context.Timeout = defaultTimeout
	}
	var timeout *time.Timer
	if context.Timeout != -1 {
		timeout = time.NewTimer(context.Timeout)
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
					if !window.Focus && context.Keybind {
						if err := m.activate(window); err != nil {
							return nil, err
						}
					} else {
						if success, err := m.Minimize(context, window); err != nil {
							return minimized, err
						} else if success {
							minimized = append(minimized, window)
						}
					}
				}
			}
		}
	}
}

func (m *WindowManipulator) MinimizeFirstMatch(context Context, windowMatcher WindowMatcher) (*Window, error) {
	if context.Timeout == 0 {
		context.Timeout = defaultTimeout
	}
	var timeout *time.Timer
	if context.Timeout != -1 {
		timeout = time.NewTimer(context.Timeout)
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
					if !window.Focus && context.Keybind {
						if err := m.activate(window); err != nil {
							return nil, err
						}
					} else {
						if success, err := m.Minimize(context, window); err != nil {
							return nil, err
						} else if success {
							return window, nil
						}
					}
				}
			}
		}
	}
}

func (m *WindowManipulator) Unminimize(window *Window) error {
	if err := m.activate(window); err != nil {
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
