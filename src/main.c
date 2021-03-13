#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>

#include "./log.h"
#include "./irc.h"

#define HOST "irc.chat.twitch.tv"
#define PORT "6697"

char *slurp_file(const char *file_path)
{
    FILE *f = NULL;
    char *buffer = NULL;

    f = fopen(file_path, "r");
    if (f == NULL) {
        goto error;
    }

    if (fseek(f, 0, SEEK_END) < 0) {
        goto error;
    }

    long m = ftell(f);
    if (m < 0) {
        goto error;
    }

    buffer = malloc((size_t) m + 1);
    if (buffer == NULL) {
        goto error;
    }

    if (fseek(f, 0, SEEK_SET) < 0) {
        goto error;
    }

    fread(buffer, 1, (size_t) m, f);
    if (ferror(f)) {
        goto error;
    }
    buffer[m] = '\0';

// ok:
    fclose(f);

    return buffer;

error:
    if (f) {
        fclose(f);
    }

    if (buffer) {
        free(buffer);
    }

    return NULL;
}

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
    fprintf(stream, "Usage: %s <secret.conf>\n", program);
}

int main(int argc, char **argv)
{
    char *secret_conf_content = NULL;
    Irc irc = {0};

    //////////////////////////////

    Log log = log_to_handle(stdout);

    const char *const program = shift(&argc, &argv);        // skip program

    if (argc == 0) {
        usage(program, stderr);
        log_error(&log, "path to secret.conf file expected");
        goto error;
    }

    const char *const secret_conf_path = shift(&argc, &argv);

    secret_conf_content = slurp_file(secret_conf_path);
    if (secret_conf_content == NULL) {
        log_error(&log, "Could not read file `%s`: %s",
                  secret_conf_path, strerror(errno));
        goto error;
    }

    String_View nickname = SV_NULL;
    String_View password = SV_NULL;
    String_View channel  = SV_NULL;

    {
        String_View content = sv_from_cstr(secret_conf_content);

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
                    log_error(&log, "unknown key `"SV_Fmt"`", SV_Arg(key));
                    goto error;
                }
            }
        }

        if (nickname.data == NULL) {
            log_error(&log, "`nickname` was not provided");
            goto error;
        }

        if (password.data == NULL) {
            log_error(&log, "`password` was not provided");
            goto error;
        }

        if (channel.data == NULL) {
            log_error(&log, "`channel` was not provided");
            goto error;
        }
    }

    if (!irc_connect(&log, &irc, HOST, PORT)) {
        goto error;
    }

    // TODO: no support for Twitch IRC tags
    irc_pass(irc.ssl, password);
    irc_nick(irc.ssl, nickname);
    irc_join(irc.ssl, channel);

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
                        irc_pong(irc.ssl, param);
                    } else {
                        irc_pong(irc.ssl, SV("tmi.twitch.tv"));
                    }
                } else if (sv_eq(command, SV("PRIVMSG"))) {
                    String_View channel = {0};
                    params_next(&params, &channel);

                    String_View message = {0};
                    params_next(&params, &message);
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
        goto error;
    }

    if (read_size <= 0) {
        char buf[512] = {0};
        ERR_error_string_n(SSL_get_error(irc.ssl, read_size), buf, sizeof(buf));
        log_error(&log, "SSL failed with error: %s", buf);
        goto error;
    }

// ok:
    if (secret_conf_content) {
        free(secret_conf_content);
    }

    irc_destroy(&irc);

    return 0;

error:
    if (secret_conf_content) {
        free(secret_conf_content);
    }

    irc_destroy(&irc);

    return -1;
}
