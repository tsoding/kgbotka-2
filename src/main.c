#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>

#include "./arena.h"
#include "./command.h"
#include "./log.h"
#include "./secret.h"
#include "./irc.h"

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

    const String_View secret_conf_path = sv_from_cstr(shift(&argc, &argv));

    Secret secret = secret_from_file(&secret_arena, secret_conf_path);
    Log log = log_to_handle(stdout);

    Irc irc = irc_connect(HOST, PORT);

    // TODO: no support for Twitch IRC tags
    irc_pass(&irc, secret.password);
    irc_nick(&irc, secret.nickname);
    irc_join(&irc, secret.channel);

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

    // TODO: autoreconnect

#define BUFFER_DROPS_THRESHOLD 5
    char buffer[4096];
    size_t buffer_size = 0;

    int read_size = SSL_read(irc.ssl, buffer + buffer_size, sizeof(buffer) - buffer_size);
    size_t buffer_drops_count = 0;
    while (read_size > 0 && buffer_drops_count < BUFFER_DROPS_THRESHOLD) {
        buffer_size += read_size;

        {
            String_View buffer_view = {
                .count = buffer_size,
                .data = buffer,
            };

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
                        irc_pong(&irc, param);
                    } else {
                        irc_pong(&irc, SV("tmi.twitch.tv"));
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
                            irc_privmsg(&irc, channel, def.response);
                        } else {
                            log_warning(&log, "Could not find command `"SV_Fmt"`", SV_Arg(command_call.name));
                        }
                    }
                }
            }

            if (buffer_view.count == sizeof(buffer)) {
                // NOTE: if after filling up the buffer completely we still
                // could not process any valid IRC messages, we assume that server
                // sent garbage and dropping it.
                buffer_drops_count += 1;
                log_warning(&log, "[%zu/%zu] Server sent garbage.",
                            buffer_drops_count, (size_t) BUFFER_DROPS_THRESHOLD);
                buffer_size = 0;
            } else {
                memmove(buffer, buffer_view.data, buffer_view.count);
                buffer_size = buffer_view.count;
                buffer_drops_count = 0;
            }
        }

        read_size = SSL_read(irc.ssl, buffer + buffer_size, sizeof(buffer) - buffer_size);
    }

    if (buffer_drops_count >= BUFFER_DROPS_THRESHOLD) {
        log_error(&log, "Server sent too much garbage");
    }

    if (read_size <= 0) {
        char buf[512] = {0};
        ERR_error_string_n(SSL_get_error(irc.ssl, read_size), buf, sizeof(buf));
        log_error(&log, "SSL failed with error: %s\n", buf);
    }

    irc_destroy(irc);

    return 0;
}
