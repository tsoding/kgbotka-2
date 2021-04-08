@echo off
rem run this from msvc-enabled console (vcvarsall x64)

if not exist msvc-build mkdir msvc-build

set CC=cl
set CFLAGS=/FC /nologo /W4
set SOURCES=src/main.c src/sv.c src/log.c src/irc.c src/cmd.c src/region.c src/http.c src/discord.c src/net.c
set INCLUDES=/Ivendor/openssl/include /Ivendor/curl/include
set LIBS=/DCURL_STATICLIB /link vendor/curl/lib/libcurl_a.lib vendor/openssl/lib/libcrypto.lib vendor/openssl/lib/libssl.lib Ws2_32.lib Wldap32.lib MSVCRT.lib Advapi32.lib Crypt32.lib Normaliz.lib User32.lib

%CC% %CFLAGS% /Fomsvc-build/ %SOURCES% %INCLUDES% %LIBS% /out:kgbotka.exe /MAP:msvc-build/m.map
