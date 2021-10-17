#!/bin/sh
set -e

CC=clang
CFLAGS="-Wall -O2"

build() {
    mkdir -p out
    gperf -S 1 src/prim.gperf > src/prim.h
    $CC $CFLAGS src/lith.c src/main.c -ledit -o out/lith
    $CC $CFLAGS src/lith.c src/test.c -o out/test
}

for command in "$@"
do
    case $command in
    check)
        checkbashisms $0
        ;;
    build)
        build
        ;;
    test)
        out/test
        ;;
    run)
        out/lith
        ;;
    clean)
        rm -rf out
        ;;
    esac
done