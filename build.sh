#!/bin/bash

CC=clang
CFLAGS="-Wall -O2"

gperf -S 1 src/prim.gperf > src/prim.h
$CC $CFLAGS src/lith.c src/main.c -ledit -o out/lith