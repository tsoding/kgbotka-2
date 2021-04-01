# KGBotka 2: Eclectic Boogaloo

## Dependencies

We depend on [OpenSSL](https://www.openssl.org/). Feel free to submit PRs to make the project more compatible with alternatives like [LibreSSL](https://www.libressl.org/). ('Cause I know nothing about SSL stuff).

## Quick Start

`*NIX`:
```console
$ ./build.sh
$ ./kgbotka ./path/to/secret.conf
```

`Windows (Visual Studio)`:
```
> setup_msvc_deps.bat
> build_msvc.bat
> kgbotka.exe path/to/secret.conf
```

`Windows (MSYS2)`:

MSYS2 mingw-w64 packages:

```console
$ pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-openssl mingw-w64-x86_64-curl 
```

Build:

```regular windows console (cmd)
> build_msys2.bat
> kgbotka.exe path/to/secret.conf
```

## secret.conf

```conf
nickname = <nickname>
password = <password>
channel = <channel>
```
