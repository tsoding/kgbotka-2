#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -std=c11 -pedantic -ggdb"
SSL_CFLAGS=`pkg-config --cflags openssl`
SSL_LIBS=`pkg-config --libs openssl`

cc $CFLAGS $SSL_CFLAGS -o kgbotka src/main.c src/sv.c src/log.c src/irc.c -lm $SSL_LIBS
