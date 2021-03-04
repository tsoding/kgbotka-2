#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -std=c11 -pedantic -ggdb"

cc $CFLAGS -o kgbotka src/main.c src/arena.c src/sv.c -lm
