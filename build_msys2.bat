@echo off
set CC=gcc
set CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb
set SOURCES=src/main.c src/sv.c src/log.c src/irc.c src/cmd.c src/region.c
set LIBS=-lssl -lcrypto -lcurl -lws2_32

%CC% %CFLAGS% %SOURCES% %INCLUDES% %LIBS% -o kgbotka