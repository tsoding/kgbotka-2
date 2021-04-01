@echo off
rem run this from msvc-enabled console

mkdir msvc-build
cd msvc-build

set CC=cl
set CFLAGS=/FC /nologo
set SOURCES=../src/main.c ../src/sv.c ../src/log.c ../src/irc.c ../src/cmd.c ../src/region.c
set INCLUDES=/I../vendor/openssl/include /I../vendor/curl/include
set LIBS=/DCURL_STATICLIB /link ../vendor/curl/lib/libcurl_a.lib ../vendor/openssl/lib/libcrypto.lib ../vendor/openssl/lib/libssl.lib Ws2_32.lib Wldap32.lib MSVCRT.lib Advapi32.lib Crypt32.lib Normaliz.lib User32.lib

%CC% %CFLAGS% /Fekgbotka.exe %SOURCES% %INCLUDES% %LIBS%
cd ../