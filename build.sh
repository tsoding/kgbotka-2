#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -std=c11 -pedantic -ggdb"

cc $CFLAGS -o kgbotka main.c -lm
