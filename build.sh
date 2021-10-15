#!/bin/bash

CC=clang
CFLAGS="-Wall -O2"

function build {
    mkdir -p out
    gperf -S 1 src/prim.gperf > src/prim.h
    $CC $CFLAGS src/lith.c src/main.c -ledit -o out/lith
}

for command in "$@"
do
    case $command in
    build)
        build
        ;;
    run)
        out/lith
        ;;
    clean)
        rm -rf out
        ;;
    esac
done