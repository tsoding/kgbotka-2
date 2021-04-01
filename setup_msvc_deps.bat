@echo off
rem run this from msvc-enabled console

set CURL_V=7.76.0
set OPENSSL_V=1.1.1k
set NASM_V=2.15.05
set PERL_V=5.14.4.1

mkdir vendor\curl
mkdir vendor\openssl\lib

if not exist curl-%CURL_V%.zip curl -fsSL -o curl-%CURL_V%.zip https://curl.se/download/curl-%CURL_V%.zip
if not exist curl-%CURL_V% tar -xf curl-%CURL_V%.zip
cd curl-%CURL_V%/winbuild
nmake /f Makefile.vc mode=static
cd ../builds/libcurl-vc-x86-release-static-ipv6-sspi-schannel

move include ../../../vendor/curl
move lib ../../../vendor/curl

cd ../../../

if not exist nasm-%NASM_V%-win32.zip curl -fsSL -o nasm-%NASM_V%-win32.zip https://www.nasm.us/pub/nasm/releasebuilds/%NASM_V%/win32/nasm-%NASM_V%-win32.zip
if not exist nasm-%NASM_V%-win32 tar -xf nasm-%NASM_V%-win32.zip
cd nasm-%NASM_V%

if not exist strawberry-perl-%PERL_V%-32bit-portable.zip curl -fsSL -o strawberry-perl-%PERL_V%-32bit-portable.zip https://strawberryperl.com/download/%PERL_V%/strawberry-perl-%PERL_V%-32bit-portable.zip
tar -xf strawberry-perl-%PERL_V%-32bit-portable.zip
move nasm.exe perl/bin
cd perl/bin

if not exist openssl-%OPENSSL_V%.tar.g curl -fsSL -o openssl-%OPENSSL_V%.tar.gz https://www.openssl.org/source/openssl-%OPENSSL_V%.tar.gz
if not exist openssl-%OPENSSL_V% tar -xf openssl-%OPENSSL_V%.tar.gz
robocopy openssl-%OPENSSL_V% . /E /MOVE
perl Configure VC-WIN32 --prefix=C:\Build-OpenSSL-VC-32 no-shared
nmake

move include ../../../vendor/openssl
move libcrypto.lib ../../../vendor/openssl/lib
move libssl.lib ../../../vendor/openssl/lib

cd ../../../

rmdir /s /q curl-%CURL_V%
rmdir /s /q nasm-%NASM_V%
del curl-%CURL_V%.zip
del nasm-%NASM_V%-win32.zip