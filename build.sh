#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -std=c11 -pedantic -ggdb"
SSL_CFLAGS=`pkg-config --cflags openssl`
SSL_LIBS=`pkg-config --libs openssl`

cc $CFLAGS $SSL_CFLAGS -o kgbotka src/main.c src/arena.c src/sv.c -lm $SSL_LIBS
