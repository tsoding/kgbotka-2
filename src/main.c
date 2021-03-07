#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "./arena.h"
#include "./buffer.h"
#include "./tls_imp.h"

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

void join(struct tls_imp_client *client, String_View channel)
{
    tls_imp_write(client, "JOIN ", 5);
    tls_imp_write(client, channel.data, channel.count);
    tls_imp_write(client, "\n", 1);
}

void pass(struct tls_imp_client *client, String_View password)
{
    tls_imp_write(client, "PASS ", 5);
    tls_imp_write(client, password.data, password.count);
    tls_imp_write(client, "\n", 1);
}

void nick(struct tls_imp_client *client, String_View nickname)
{
    tls_imp_write(client, "NICK ", 5);
    tls_imp_write(client, nickname.data, nickname.count);
    tls_imp_write(client, "\n", 1);
}

void tls_imp_write_cstr(struct tls_imp_client *client, const char *cstr)
{
    tls_imp_write(client, cstr, strlen(cstr));
}

void tls_imp_write_sv(struct tls_imp_client *client, String_View sv)
{
    tls_imp_write(client, sv.data, sv.count);
}

void privmsg(struct tls_imp_client *client, String_View channel,
             String_View message)
{
    tls_imp_write_cstr(client, "PRIVMSG ");
    tls_imp_write_sv(client, channel);
    tls_imp_write_cstr(client, " :");
    tls_imp_write_sv(client, message);
    tls_imp_write_cstr(client, "\n");
}

void pong(struct tls_imp_client *client, String_View response)
{
    tls_imp_write_cstr(client, "PONG :");
    tls_imp_write_sv(client, response);
}

String_View sv_from_buffer(Buffer buffer)
{
    return (String_View) {
        .count = buffer.size,
        .data = buffer.data
    };
}

bool params_next(String_View *params, String_View *output)
{
    assert(params);

    if (params->count > 0) {
        String_View param = {0};

        if (*params->data == ':') {
            sv_chop_left(params, 1);
            size_t n = params->count;
            param = sv_chop_left(params, n);
        } else {
            param = sv_chop_by_delim(params, ' ');
        }

        if (output) {
            *output = param;
        }

        return true;
    }

    return false;
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
                fprintf(stderr, "ERROR: unknown key `"SV_Fmt"`\n",
                        SV_Arg(key));
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

    struct tls_imp_global global;
    if (!tls_imp_init(&global)) {
        fprintf(stderr, "ERROR: could not initialize the SSL context: %s\n",
                strerror(errno));
        exit(1);
    }

    struct tls_imp_client client;
    if (!tls_imp_client_init(&global, &client, sd, HOST)) {
        fprintf(stderr, "ERROR: could not connect via SSL: %s\n",
                strerror(errno));
        exit(1);
    }

    //////////////////////////////

    pass(&client, password);
    nick(&client, nickname);
    join(&client, channel);

    // TODO: autoreconnect
    Buffer buffer = {0};
    char chunk[512];
    ssize_t chunk_size = tls_imp_read(&client, chunk, sizeof(chunk));
    while (chunk_size > 0) {
        buffer_write(&buffer, chunk, chunk_size);

        // TODO: we need to handle situations when the buffer starts to grow indefinitely due to the server not sending any \r\n
        {
            String_View buffer_view = sv_from_buffer(buffer);
            String_View line = {0};
            while (sv_try_chop_by_delim(&buffer_view, '\n', &line)) {
                line = sv_trim(line);
                if (sv_starts_with(line, SV(":"))) {
                    String_View prefix = sv_chop_by_delim(&line, ' ');
                    printf("Prefix: "SV_Fmt"\n", SV_Arg(prefix));
                }

                String_View command = sv_chop_by_delim(&line, ' ');
                printf("Command: "SV_Fmt"\n", SV_Arg(command));

                // TODO: Params & params_from_line is not defined
                Params params = params_from_line(line);

                if (sv_eq(command, SV("PING"))) {
                    String_View param = {0};
                    if (params_next(&params, &param)) {
                        pong(&client, param);
                    } else {
                        pong(&client, SV("tmi.twitch.tv"));
                    }
                }
            }
            memmove(buffer.data, buffer_view.data, buffer_view.count);
            buffer.size = buffer_view.count;
        }

        chunk_size = tls_imp_read(&client, chunk, sizeof(chunk));
    }

    tls_imp_client_close(&client);
    close(sd);
    tls_imp_free(&global);

    return 0;
}
