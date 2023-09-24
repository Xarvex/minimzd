package main

import (
	"strings"
	"time"

	"github.com/spf13/pflag"
)

const defaultTimeout = 10 * time.Second

type Context struct {
	Help                bool
	First               bool
	Verify              bool
	CloseWindow         bool
	Keybind             bool
	ExtractKeybind      bool
	MatchPid            bool
	MatchProcessName    bool
	Pid                 int
	ProcessName         string
	WindowClassInstance string
	WindowClass         string
	Timeout             time.Duration
	Command             string
	CommandArgs         []string
}

func (c *Context) PopulateFromArgs(earlyExit bool) {
	var processNameDefined bool
	var windowClassInstanceDefined bool
	var windowClassDefined bool
	{
		pflag.BoolP("help", "h", false, "displays usage")
		helpFlag := pflag.Lookup("help")
		pflag.BoolVarP(&c.First, "first", "F", false, "stop after the first match is minimized")
		pflag.BoolVarP(&c.Verify, "verify", "V", false, "verify that the window was actually minimized")
		pflag.BoolVarP(&c.CloseWindow, "close-window", "x", false, "close the window, useful for applications that run in background")
		pflag.BoolVarP(&c.Keybind, "keybind", "k", false, "use a keybind for specified action, which may give alternative behavior")
		pflag.BoolVarP(&c.ExtractKeybind, "extract-keybind", "e", false, "extracts any needed keybind from GNOME settings")
		pflag.BoolVar(&c.MatchPid, "match-pid", false, "match the pid of the executed command, must be a unique instance to function")
		pflag.BoolVar(&c.MatchProcessName, "match-process-name", false, "match the process name")
		pflag.IntVar(&c.Pid, "pid", 0, "pid to match, takes precedence over process name")
		pflag.Lookup("pid").DefValue = "result from matched pid, if enabled"
		pflag.StringVar(&c.ProcessName, "process-name", "", "process name to match, takes precedence over window class (default command name, if present)")
		processNameFlag := pflag.Lookup("process-name")
		pflag.StringVar(&c.WindowClassInstance, "window-class-instance", "", "window class instance to match, takes precedence over window class (default command name, if present)")
		windowClassInstanceFlag := pflag.Lookup("window-class-instance")
		pflag.StringVar(&c.WindowClass, "window-class", "", "window class to match, lowest precedence (default command name, if present)")
		windowClassFlag := pflag.Lookup("window-class")
		pflag.DurationVarP(&c.Timeout, "timeout", "t", defaultTimeout, "allowed amount of time to run")

		pflag.CommandLine.SortFlags = false
		pflag.ParseAll(func(flag *pflag.Flag, value string) error {
			switch flag {
			case helpFlag:
				c.Help = true
			case processNameFlag:
				processNameDefined = true
			case windowClassInstanceFlag:
				windowClassInstanceDefined = true
			case windowClassFlag:
				windowClassDefined = true
			}
			return nil
		})

		if earlyExit && c.Help {
			return
		}

		pflag.Parse()

		if commandArgs := strings.Split(pflag.Arg(0), " "); len(commandArgs) > 0 {
			c.Command = commandArgs[0]
			c.CommandArgs = commandArgs[1:]
		}

		if !processNameDefined {
			c.ProcessName = c.Command
		}
		if !windowClassInstanceDefined {
			c.WindowClassInstance = c.Command
		}
		if !windowClassDefined {
			c.WindowClass = c.Command
		}
	}
}

func fromArgs(earlyExit bool) Context {
	c := Context{}
	c.PopulateFromArgs(earlyExit)
	return c
}
