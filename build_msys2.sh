#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -std=c11 -pedantic -ggdb"
SSL_CFLAGS=`pkg-config --cflags openssl --static`
SSL_LIBS=`pkg-config --libs openssl --static`
# static curl linking have problem 2020: https://github.com/msys2/MINGW-packages/issues/6769
CURL_CFLAGS=`pkg-config --cflags libcurl --static`
CURL_LIBS=`pkg-config --libs libcurl`

cc $CFLAGS $SSL_CFLAGS $CURL_CFLAGS -o kgbotka src/main.c src/sv.c src/log.c src/irc.c src/cmd.c src/region.c -lm $SSL_LIBS $CURL_LIBS -lpthread
