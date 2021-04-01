#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -std=c11 -pedantic -ggdb"
SSL_CFLAGS=`pkg-config --cflags openssl`
SSL_LIBS=`pkg-config --libs openssl`
CURL_CFLAGS=`pkg-config --cflags libcurl`
CURL_LIBS=`pkg-config --libs libcurl`

cc $CFLAGS $SSL_CFLAGS $CURL_CFLAGS -o kgbotka src/main.c src/sv.c src/log.c src/irc.c src/cmd.c src/region.c src/http.c -lm $SSL_LIBS $CURL_LIBS -lpthread
