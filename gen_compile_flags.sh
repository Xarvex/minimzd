#!/bin/sh

echo `pkg-config --cflags --libs glibmm-2.4 giomm-2.4` | tr ' ' '\n' > compile_flags.txt
