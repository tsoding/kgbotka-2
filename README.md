# KGBotka 2: Eclectic Boogaloo

## Dependencies

We depend on [OpenSSL](https://www.openssl.org/). Feel free to submit PRs to make the project more compatible with alternatives like [LibreSSL](https://www.libressl.org/). ('Cause I know nothing about SSL stuff).

## Quick Start

```console
$ ./build.sh
$ ./kgbotka ./state/ ./path/to/secret.conf
```

<!-- TODO: windows build is not documented -->

## secret.conf

```conf
nickname = <nickname>
password = <password>
channel = <channel>
```
