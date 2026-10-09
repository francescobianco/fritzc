#!/usr/bin/env python3
"""Invia sequenze di tasti alla finestra X11 attiva tramite XTest.

Uso: xkeys.py "ctrl+F5" "F1" "Down" "Return" ...
Ogni argomento è una combinazione di keysym X11 separati da '+'.
Un argomento "sleep:N" attende N secondi.
Serve ad automatizzare DOSBox durante l'analisi (catture, menu).
"""
import ctypes
import sys
import time

x11 = ctypes.CDLL('libX11.so.6')
xtst = ctypes.CDLL('libXtst.so.6')
x11.XOpenDisplay.restype = ctypes.c_void_p
x11.XStringToKeysym.restype = ctypes.c_ulong
x11.XKeysymToKeycode.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
xtst.XTestFakeKeyEvent.argtypes = [ctypes.c_void_p, ctypes.c_uint,
                                   ctypes.c_int, ctypes.c_ulong]
x11.XFlush.argtypes = [ctypes.c_void_p]

ALIASES = {'ctrl': 'Control_L', 'alt': 'Alt_L', 'shift': 'Shift_L',
           'esc': 'Escape', 'enter': 'Return', 'space': 'space'}


def keycode(dpy, name: str) -> int:
    sym = x11.XStringToKeysym(ALIASES.get(name.lower(), name).encode())
    code = x11.XKeysymToKeycode(dpy, sym)
    if not code:
        raise SystemExit(f'tasto sconosciuto: {name}')
    return code


def main() -> None:
    dpy = x11.XOpenDisplay(None)
    if not dpy:
        raise SystemExit('impossibile aprire il display X')
    for arg in sys.argv[1:]:
        if arg.startswith('sleep:'):
            time.sleep(float(arg[6:]))
            continue
        codes = [keycode(dpy, k) for k in arg.split('+')]
        for c in codes:
            xtst.XTestFakeKeyEvent(dpy, c, 1, 0)
            x11.XFlush(dpy)
            time.sleep(0.03)
        for c in reversed(codes):
            xtst.XTestFakeKeyEvent(dpy, c, 0, 0)
            x11.XFlush(dpy)
            time.sleep(0.03)
        time.sleep(0.25)


if __name__ == '__main__':
    main()
