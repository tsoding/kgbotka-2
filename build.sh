#!/bin/sh

set -xe

CFLAGS="-Wall -Wextra -std=c11 -pedantic -ggdb"

LIBRESSL_PATH="/usr/lib/libressl"
LIBRESSL_PKG_CONFIG_PATH="$LIBRESSL_PATH/pkgconfig"
SSL_CFLAGS=`PKG_CONFIG_PATH=$LIBRESSL_PKG_CONFIG_PATH pkg-config --cflags libtls`
SSL_LIBS=`PKG_CONFIG_PATH=$LIBRESSL_PKG_CONFIG_PATH pkg-config --libs libtls`
SSL_OPTION="-DB_SSL_LIBRESSL"

# SSL_CFLAGS=`pkg-config --cflags openssl`
# SSL_LIBS=`pkg-config --libs openssl`
# SSL_OPTION="-DB_SSL_OPENSSL"

cc $CFLAGS $SSL_CFLAGS $SSL_OPTION -o kgbotka src/main.c src/arena.c src/sv.c src/buffer.c src/tls_imp.c -lm $SSL_LIBS
