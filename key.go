package main

// #cgo pkg-config: glibmm-2.4 giomm-2.4
// #include <gio/gio.h>
// #include "gsettings.h"
// #include <stdlib.h>
import "C"

import (
	"strings"
	"unsafe"

	"github.com/bendahl/uinput"
)

var (
	defaultMinimizeKeybind = []int{uinput.KeyLeftmeta, uinput.KeyH}
	defaultCloseKeybind    = []int{uinput.KeyLeftalt, uinput.KeyF4}

	keyMap = map[string]int{
		"1":                uinput.Key1,
		"2":                uinput.Key2,
		"3":                uinput.Key3,
		"4":                uinput.Key4,
		"5":                uinput.Key5,
		"6":                uinput.Key6,
		"7":                uinput.Key7,
		"8":                uinput.Key8,
		"9":                uinput.Key9,
		"0":                uinput.Key0,
		"minus":            uinput.KeyMinus,
		"equal":            uinput.KeyEqual,
		"backspace":        uinput.KeyBackspace,
		"tab":              uinput.KeyTab,
		"q":                uinput.KeyQ,
		"w":                uinput.KeyW,
		"e":                uinput.KeyE,
		"r":                uinput.KeyR,
		"t":                uinput.KeyT,
		"y":                uinput.KeyY,
		"u":                uinput.KeyU,
		"i":                uinput.KeyI,
		"o":                uinput.KeyO,
		"p":                uinput.KeyP,
		"leftbrace":        uinput.KeyLeftbrace,
		"rightbrace":       uinput.KeyRightbrace,
		"return":           uinput.KeyEnter,
		"control":          uinput.KeyLeftctrl,
		"a":                uinput.KeyA,
		"s":                uinput.KeyS,
		"d":                uinput.KeyD,
		"f":                uinput.KeyF,
		"g":                uinput.KeyG,
		"h":                uinput.KeyH,
		"j":                uinput.KeyJ,
		"k":                uinput.KeyK,
		"l":                uinput.KeyL,
		"semicolon":        uinput.KeySemicolon,
		"apostrophe":       uinput.KeyApostrophe,
		"grave":            uinput.KeyGrave,
		"shift":            uinput.KeyLeftshift,
		"backslash":        uinput.KeyBackslash,
		"z":                uinput.KeyZ,
		"x":                uinput.KeyX,
		"c":                uinput.KeyC,
		"v":                uinput.KeyV,
		"b":                uinput.KeyB,
		"n":                uinput.KeyN,
		"m":                uinput.KeyM,
		"comma":            uinput.KeyComma,
		"period":           uinput.KeyDot,
		"slash":            uinput.KeySlash,
		"kp_multiply":      uinput.KeyKpasterisk,
		"alt":              uinput.KeyLeftalt,
		"space":            uinput.KeySpace,
		"f1":               uinput.KeyF1,
		"f2":               uinput.KeyF2,
		"f3":               uinput.KeyF3,
		"f4":               uinput.KeyF4,
		"f5":               uinput.KeyF5,
		"f6":               uinput.KeyF6,
		"f7":               uinput.KeyF7,
		"f8":               uinput.KeyF8,
		"f9":               uinput.KeyF9,
		"f10":              uinput.KeyF10,
		"kp_subtract":      uinput.KeyKpminus,
		"kp_add":           uinput.KeyKpplus,
		"f11":              uinput.KeyF11,
		"f12":              uinput.KeyF12,
		"kp_divide":        uinput.KeyKpslash,
		"home":             uinput.KeyHome,
		"up":               uinput.KeyUp,
		"page_up":          uinput.KeyPageup,
		"left":             uinput.KeyLeft,
		"right":            uinput.KeyRight,
		"end":              uinput.KeyEnd,
		"down":             uinput.KeyDown,
		"page_down":        uinput.KeyPagedown,
		"insert":           uinput.KeyInsert,
		"delete":           uinput.KeyDelete,
		"audiomute":        uinput.KeyMute,
		"audiolowervolume": uinput.KeyVolumedown,
		"audioraisevolume": uinput.KeyVolumeup,
		"super":            uinput.KeyLeftmeta,
		"calculator":       uinput.KeyCalc,
	}
)

func ConvertKeyString(keyString string) int {
	return keyMap[strings.ToLower(keyString)]
}

func SplitKeybindString(keybindString string) []string {
	return strings.FieldsFunc(keybindString, func(r rune) bool {
		return strings.ContainsRune("<>", r)
	})
}

func ConvertKeybindString(keybindString string) []int {
	keyStrings := SplitKeybindString(keybindString)
	keybind := make([]int, len(keyStrings))
	for i, keyString := range keyStrings {
		keybind[i] = ConvertKeyString(keyString)
	}
	return keybind
}

func convertNativeKeybindStrings(nativeKeybindStrings **C.char) [][]int {
	length := C.g_strv_length(nativeKeybindStrings)
	keybinds := make([][]int, length)
	for i, nativeKeybindString := range unsafe.Slice(nativeKeybindStrings, length) {
		keybinds[i] = ConvertKeybindString(C.GoString(nativeKeybindString))
	}
	return keybinds
}

func ExtractMinimizeKeybinds() [][]int {
	nativeKeybindStrings := C.mzd_keybinds_minimize()
	defer C.free(unsafe.Pointer(nativeKeybindStrings))
	return convertNativeKeybindStrings(nativeKeybindStrings)
}

func ExtractCloseKeybinds() [][]int {
	nativeKeybindStrings := C.mzd_keybinds_close()
	defer C.free(unsafe.Pointer(nativeKeybindStrings))
	return convertNativeKeybindStrings(nativeKeybindStrings)
}

func useKeybind(keyboard uinput.Keyboard, keybind []int) error {
	if len(keybind) < 1 {
		return nil
	}

	for _, key := range keybind[:len(keybind)-1] {
		if err := keyboard.KeyDown(key); err != nil {
			return err
		}
	}
	if err := keyboard.KeyPress(keybind[len(keybind)-1]); err != nil {
		return err
	}
	for i := range keybind[:len(keybind)-1] {
		if err := keyboard.KeyUp(keybind[len(keybind)-2-i]); err != nil {
			return err
		}
	}
	return nil
}
