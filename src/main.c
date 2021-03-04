#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "./arena.h"

#define HOST "irc.chat.twitch.tv"
// #define PORT "6667"
#define PORT "6697"
#define SECRET_CONF "secret.conf"

char *shift(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

void join(SSL *ssl, String_View channel)
{
    SSL_write(ssl, "JOIN ", 5);
    SSL_write(ssl, channel.data, channel.count);
    SSL_write(ssl, "\n", 1);
}

void pass(SSL *ssl, String_View password)
{
    SSL_write(ssl, "PASS ", 5);
    SSL_write(ssl, password.data, password.count);
    SSL_write(ssl, "\n", 1);
}

void nick(SSL *ssl, String_View nickname)
{
    SSL_write(ssl, "NICK ", 5);
    SSL_write(ssl, nickname.data, nickname.count);
    SSL_write(ssl, "\n", 1);
}

void SSL_write_cstr(SSL *ssl, const char *cstr)
{
    SSL_write(ssl, cstr, strlen(cstr));
}

void SSL_write_sv(SSL *ssl, String_View sv)
{
    SSL_write(ssl, sv.data, sv.count);
}

void privmsg(SSL *ssl, String_View channel, String_View message)
{
    SSL_write_cstr(ssl, "PRIVMSG ");
    SSL_write_sv(ssl, channel);
    SSL_write_cstr(ssl, " :");
    SSL_write_sv(ssl, message);
    SSL_write_cstr(ssl, "\n");
}

int main(int argc, char **argv)
{
    const char *const program = shift(&argc, &argv);        // skip program

    if (argc == 0) {
        fprintf(stderr, "Usage: %s <secret.conf>\n", program);
        fprintf(stderr, "ERROR: path to secret.conf expected\n");
        exit(1);
    }

    const String_View secret_conf = sv_from_cstr(shift(&argc, &argv));

    Arena arena = {0};
    String_View content = {0};
    if (arena_slurp_file(&arena, secret_conf, &content) < 0) {
        fprintf(stderr, "ERROR: could not read "SV_Fmt": %s\n",
                SV_Arg(secret_conf), strerror(errno));
        exit(1);
    }

    String_View nickname = SV_NULL;
    String_View password = SV_NULL;
    String_View channel  = SV_NULL;

    while (content.count > 0) {
        String_View line = sv_trim(sv_chop_by_delim(&content, '\n'));
        if (line.count > 0) {
            String_View key = sv_trim(sv_chop_by_delim(&line, '='));
            String_View value = sv_trim(line);
            if (sv_eq(key, SV("nickname"))) {
                nickname = value;
            } else if (sv_eq(key, SV("password"))) {
                password = value;
            } else if (sv_eq(key, SV("channel"))) {
                channel = value;
            } else {
                fprintf(stderr, "ERROR: unknown key `"SV_Fmt"`\n", SV_Arg(key));
                exit(1);
            }
        }
    }

    if (nickname.data == NULL) {
        fprintf(stderr, "ERROR: `nickname` was not provided\n");
        exit(1);
    }

    if (password.data == NULL) {
        fprintf(stderr, "ERROR: `password` was not provided\n");
        exit(1);
    }

    if (channel.data == NULL) {
        fprintf(stderr, "ERROR: `channel` was not provided\n");
        exit(1);
    }

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *addrs;
    if (getaddrinfo(HOST, PORT, &hints, &addrs) < 0) {
        fprintf(stderr, "Could not get address of `"HOST"`: %s\n",
                strerror(errno));
        exit(1);
    }

    int sd = 0;
    for (struct addrinfo *addr = addrs; addr != NULL; addr = addr->ai_next) {
        // TODO: don't recreate socket on each attempt
        // Just create a single socket with the appropriate family and type
        // and keep using it.
        sd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if (sd == -1) {
            break;
        }

        if (connect(sd, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }

        close(sd);
        sd = -1;
    }
    freeaddrinfo(addrs);

    if (sd == -1) {
        fprintf(stderr, "Could not connect to "HOST":"PORT": %s\n",
                strerror(errno));
        exit(1);
    }

    //////////////////////////////

    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());

    if (ctx == NULL) {
        fprintf(stderr, "ERROR: could not initialize the SSL context: %s\n",
                strerror(errno));
        exit(1);
    }

    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sd);

    if (SSL_connect(ssl) < 0) {
        fprintf(stderr, "ERROR: could not connect via SSL: %s\n",
                strerror(errno));
        exit(1);
    }

    //////////////////////////////

    pass(ssl, password);
    nick(ssl, nickname);
    join(ssl, channel);
    privmsg(ssl, channel, SV("what's up nerds :)"));

    char buffer[1024];
    ssize_t n = SSL_read(ssl, buffer, sizeof(buffer));
    while (n > 0) {
        fwrite(buffer, 1, n, stdout);
        n = SSL_read(ssl, buffer, sizeof(buffer));
    }

    return 0;
}
