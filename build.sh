#!/bin/bash

CC=clang
CFLAGS=""

gperf -S 1 src/prim.gperf > src/prim.h
$CC $CFLAGS src/lith.c src/main.c -ledit -o out/lith

# case "$1" in
#     "")
#         ;;
#     run)
#         out/lith
#         ;;
#     *)
#         echo "unknown command $1"
#         ;;
# esac