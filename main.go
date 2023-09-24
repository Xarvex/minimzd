package main

import (
	"fmt"
	"os"
	"os/exec"

	"github.com/spf13/pflag"
)

func main() {
	errExit := func(err error) {
		fmt.Fprintln(os.Stderr, err)
		os.Exit(1)
	}

	c := fromArgs(true)
	if c.Help {
		pflag.PrintDefaults()
		return
	}

	dbusManager := WindowManipulator{}
	if err := dbusManager.Connect(c.Keybind); err != nil {
		errExit(err)
	}
	defer dbusManager.Close()

	var cmd *exec.Cmd
	if c.Command != "" {
		cmd = exec.Command(c.Command, c.CommandArgs...)
		if err := cmd.Start(); err != nil {
			errExit(fmt.Errorf("failed to execute command: %w", err))
		}
		if c.MatchPid && c.Pid == 0 {
			c.Pid = cmd.Process.Pid
		}
	}

	if c.ExtractKeybind {
		dbusManager.ExtractKeybinds()
	}

	var windowMatcher WindowMatcher
	if c.Pid != 0 {
		windowMatcher = func(w *Window) bool {
			return w.Pid == c.Pid
		}
	} else if c.WindowClassInstance != "" {
		windowMatcher = func(w *Window) bool {
			return w.ClassInstance == c.WindowClassInstance
		}
	} else if c.WindowClass != "" {
		windowMatcher = func(w *Window) bool {
			return w.Class == c.WindowClass
		}
	}
	if windowMatcher != nil {
		if c.First {
			if _, err := dbusManager.MinimizeFirstMatch(c, windowMatcher); err != nil {
				errExit(err)
			}
		} else {
			if _, err := dbusManager.MinimizeMatch(c, windowMatcher); err != nil {
				errExit(err)
			}
		}
	}
}
