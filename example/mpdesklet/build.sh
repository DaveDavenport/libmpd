#!/bin/sh
gcc -o mpdesklet mpdesklet.c egg-background-monitor.c misc.c strfsong.c `pkg-config --libs --cflags libmpd-0.01 gtk+-2.0`

