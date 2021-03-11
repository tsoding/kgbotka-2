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

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "./arena.h"
#include "./buffer.h"
#include "./command.h"
#include "./log.h"

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

void pong(SSL *ssl, String_View response)
{
    SSL_write_cstr(ssl, "PONG :");
    SSL_write_sv(ssl, response);
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

void usage(const char *program, FILE *stream)
{
    fprintf(stream, "Usage: %s <state/> <secret.conf>\n", program);
}

Arena secret_arena = {0};
Arena commands_arena = {0};

int main(int argc, char **argv)
{
    const char *const program = shift(&argc, &argv);        // skip program

    if (argc == 0) {
        usage(program, stderr);
        fprintf(stderr, "ERROR: path to state/ folder expected\n");
        exit(1);
    }

    const String_View state_path = sv_from_cstr(shift(&argc, &argv));

    if (argc == 0) {
        usage(program, stderr);
        fprintf(stderr, "ERROR: path to secret.conf file expected\n");
        exit(1);
    }

    const String_View secret_conf = sv_from_cstr(shift(&argc, &argv));

    String_View nickname = SV_NULL;
    String_View password = SV_NULL;
    String_View channel  = SV_NULL;

    {
        String_View content = {0};
        if (arena_slurp_file(&secret_arena, secret_conf, &content) < 0) {
            fprintf(stderr, "ERROR: could not read "SV_Fmt": %s\n",
                    SV_Arg(secret_conf), strerror(errno));
            exit(1);
        }

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

    Log log = log_to_handle(stdout);

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *addrs;
    if (getaddrinfo(HOST, PORT, &hints, &addrs) < 0) {
        log_unlucky(&log, "Could not get address of `"HOST"`: %s",
                    strerror(errno));
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
        log_unlucky(&log, "Could not connect to "HOST":"PORT": %s",
                    strerror(errno));
    }

    //////////////////////////////

    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());

    if (ctx == NULL) {
        log_unlucky(&log, "Could not initialize the SSL context: %s",
                    strerror(errno));
    }

    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, sd);

    if (SSL_connect(ssl) < 0) {
        log_unlucky(&log, "Could not connect via SSL: %s",
                    strerror(errno));
    }

    //////////////////////////////

    pass(ssl, password);
    nick(ssl, nickname);
    join(ssl, channel);

    // TODO: hot reloading of the commands is not implemented
    Commands commands = {0};
    arena_clean(&commands_arena);
    String_View commands_file_path = SV_CONCAT(&commands_arena, state_path, SV("/commands.txt"));
    if (!load_commands_file(&commands_arena, &log, commands_file_path, &commands)) {
        arena_clean(&commands_arena);
        memset(&commands, 0, sizeof(commands));
    } else {
        log_info(&log, "Loaded %zu commands", commands.count);
        for (size_t i = 0; i < commands.count; ++i) {
            log_info(&log, "    "SV_Fmt, SV_Arg(commands.command_defs[i].name));
        }
    }

    // TODO: no support for Twitch IRC tags

    // TODO: autoreconnect
    Buffer buffer = {0};
    char chunk[512];
    ssize_t chunk_size = SSL_read(ssl, chunk, sizeof(chunk));
    while (chunk_size > 0) {
        buffer_write(&buffer, chunk, chunk_size);

        // TODO: we need to handle situations when the buffer starts to grow indefinitely due to the server not sending any \r\n
        {
            String_View buffer_view = sv_from_buffer(buffer);
            String_View line = {0};
            while (sv_try_chop_by_delim(&buffer_view, '\n', &line)) {
                line = sv_trim(line);
                log_info(&log, SV_Fmt, SV_Arg(line));

                if (sv_starts_with(line, SV(":"))) {
                    String_View prefix = sv_chop_by_delim(&line, ' ');
                    (void) prefix;
                }

                String_View command = sv_chop_by_delim(&line, ' ');

                String_View params = line;

                if (sv_eq(command, SV("PING"))) {
                    String_View param = {0};
                    if (params_next(&params, &param)) {
                        pong(ssl, param);
                    } else {
                        pong(ssl, SV("tmi.twitch.tv"));
                    }
                } else if (sv_eq(command, SV("PRIVMSG"))) {
                    String_View channel = {0};
                    params_next(&params, &channel);

                    String_View message = {0};
                    params_next(&params, &message);

                    Command_Call command_call = {0};
                    if (command_call_parse(SV("%"), message, &command_call)) {
                        Command_Def def = {0};
                        if (commands_find_def(&commands, command_call.name, &def)) {
                            privmsg(ssl, channel, def.response);
                        } else {
                            log_warn(&log, "Could not find command `"SV_Fmt"`", SV_Arg(command_call.name));
                        }
                    }
                }
            }
            memmove(buffer.data, buffer_view.data, buffer_view.count);
            buffer.size = buffer_view.count;
        }

        chunk_size = SSL_read(ssl, chunk, sizeof(chunk));
    }

    // TODO: finalize the connection

    return 0;
}
