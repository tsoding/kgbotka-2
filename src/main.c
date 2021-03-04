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

#include "./arena.h"

#define HOST "irc.chat.twitch.tv"
#define PORT "6667"
#define SECRET_CONF "secret.conf"

char *shift(int *argc, char ***argv)
{
    assert(*argc > 0);
    char *result = **argv;
    *argv += 1;
    *argc -= 1;
    return result;
}

void join(int sd, String_View channel)
{
    write(sd, "JOIN ", 5);
    write(sd, channel.data, channel.count);
    write(sd, "\n", 1);
}

void pass(int sd, String_View password)
{
    write(sd, "PASS ", 5);
    write(sd, password.data, password.count);
    write(sd, "\n", 1);
}

void nick(int sd, String_View nickname)
{
    write(sd, "NICK ", 5);
    write(sd, nickname.data, nickname.count);
    write(sd, "\n", 1);
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

    pass(sd, password);
    nick(sd, nickname);
    join(sd, channel);

    char buffer[1024];
    ssize_t n = read(sd, buffer, sizeof(buffer));
    while (n > 0) {
        fwrite(buffer, 1, n, stdout);
        n = read(sd, buffer, sizeof(buffer));
    }

    return 0;
}
