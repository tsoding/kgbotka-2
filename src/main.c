#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <signal.h>

#include <curl/curl.h>

#include "./log.h"
#include "./irc.h"
#include "./cmd.h"
#include "./region.h"

#define ARRAY_LEN(xs) (sizeof(xs) / sizeof((xs)[0]))

#define HOST "irc.chat.twitch.tv"
#define SECURE_PORT "6697"
#define PLAIN_PORT "6667"

// http://www.iso-9899.info/n1570.html#5.1.2.3p5
volatile sig_atomic_t sigint = 0;

void sig_handler(int signo)
{
    if (signo == SIGINT) {
        sigint = 1;
    }
}

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
    Log log = log_to_handle(stdout);

    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        log_warning(&log, "Could not setup signal handler for SIGINT: %s",
                    strerror(errno));
    }

    // Resource to destroy at the end
    bool curl_global_initalized = false;
    CURL *curl = NULL;
    char *secret_conf_content = NULL;
    Irc irc = {0};
    SSL_CTX *ctx = NULL;
    Region *cmd_region = NULL;

    // Secret configuration
    String_View secret_nickname = SV_NULL;
    String_View secret_password = SV_NULL;
    String_View secret_channel  = SV_NULL;

    // Parse secret.conf
    {
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

        String_View content = sv_from_cstr(secret_conf_content);

        while (content.count > 0) {
            String_View line = sv_trim(sv_chop_by_delim(&content, '\n'));
            if (line.count > 0) {
                String_View key = sv_trim(sv_chop_by_delim(&line, '='));
                String_View value = sv_trim(line);
                if (sv_eq(key, SV("nickname"))) {
                    secret_nickname = value;
                } else if (sv_eq(key, SV("password"))) {
                    secret_password = value;
                } else if (sv_eq(key, SV("channel"))) {
                    switch(value.data[0]) {
                    case '#':
                    case '&':
                        break;
                    default:
                        log_error(&log, "`channel` must start with a '#' or '&' character");
                        goto error;
                    }
                    secret_channel = value;
                } else {
                    log_error(&log, "unknown key `"SV_Fmt"`", SV_Arg(key));
                    goto error;
                }
            }
        }

        if (secret_nickname.data == NULL) {
            log_error(&log, "`nickname` was not provided");
            goto error;
        }

        if (secret_password.data == NULL) {
            log_error(&log, "`password` was not provided");
            goto error;
        }

        if (secret_channel.data == NULL) {
            log_error(&log, "`channel` was not provided");
            goto error;
        }

        log_info(&log, "Parsed `%s` successfully", secret_conf_path);
    }

    // Initialize CURL
    {
        if (curl_global_init(CURL_GLOBAL_DEFAULT) != 0) {
            log_error(&log, "Could not initialize global CURL state for some reason");
            goto error;
        }
        curl_global_initalized = true;

        curl = curl_easy_init();
        if (curl == NULL) {
            log_error(&log, "Could not initialize CURL context for some reason");
            goto error;
        }

        log_info(&log, "Initialized CURL successfully");
    }

    // Initialize SSL context
    {
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        ctx = SSL_CTX_new(TLS_client_method());

        if (ctx == NULL) {
            // TODO: SSL_CTX_new error is not located in errno
            log_error(&log, "Could not initialize the SSL context: %s",
                      strerror(errno));
            goto error;
        }

        log_info(&log, "Initialized SSL successfully");
    }

    // Allocate memory region for commands
    {
        // NOTE: command that needs more than 10MB of memory is sus ngl
        const size_t CMD_REGION_CAPACITY = 10 * 1000 * 1000;
        // NOTE: the lifetime of this region is a single command execution.
        // After a command finished its execution the entire region is cleaned up.
        cmd_region = region_new(CMD_REGION_CAPACITY);
        if (cmd_region == NULL) {
            log_error(&log, "Could not allocate memory for commands");
            goto error;
        }

        log_info(&log, "Successfully allocated memory for commands");
    }

    // Connect to IRC
    {
        if (!irc_connect_secure(&log, &irc, ctx, HOST, SECURE_PORT, true)) {
            goto error;
        }

        log_info(&log, "Connected to Twitch IRC successfully");

        // TODO: no support for Twitch IRC tags
        irc_pass(&irc, secret_password);
        irc_nick(&irc, secret_nickname);
        irc_join(&irc, secret_channel);
    }

    // IRC event loop
    {
        // TODO: autoreconnect
#define BUFFER_DROPS_THRESHOLD 5
        char buffer[4096];
        size_t buffer_size = 0;

        int read_size = irc_read(&irc, buffer + buffer_size, sizeof(buffer) - buffer_size);
        size_t buffer_drops_count = 0;
        while ((read_size > 0 || irc_read_again(&irc, read_size)) &&
                buffer_drops_count < BUFFER_DROPS_THRESHOLD) {
            if (sigint) {
                log_warning(&log, "Interrupted by the user");
                goto error;
            }
            if (read_size > 0) {
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

                        // Process the IRC messsage
                        {
                            if (sv_starts_with(line, SV(":"))) {
                                String_View prefix = sv_chop_by_delim(&line, ' ');
                                (void) prefix;
                            }

                            String_View irc_command = sv_chop_by_delim(&line, ' ');
                            String_View irc_params = line;

                            if (sv_eq(irc_command, SV("PING"))) {
                                String_View param = {0};
                                if (params_next(&irc_params, &param)) {
                                    irc_pong(&irc, param);
                                } else {
                                    irc_pong(&irc, SV("tmi.twitch.tv"));
                                }
                            } else if (sv_eq(irc_command, SV("PRIVMSG"))) {
                                String_View irc_channel = {0};
                                params_next(&irc_params, &irc_channel);

                                String_View message = {0};
                                params_next(&irc_params, &message);

                                // Handle user command
                                {
                                    message = sv_trim_left(message);
                                    if (sv_starts_with(message, SV(CMD_PREFIX))) {
                                        sv_chop_left(&message, 1);
                                        message = sv_trim_left(message);
                                        String_View cmd_name = sv_trim(sv_chop_by_delim(&message, ' '));
                                        String_View cmd_args = message;

                                        Cmd_Run cmd_run = find_cmd_by_name(cmd_name);
                                        if (cmd_run) {
                                            cmd_run(&irc, &log, curl, cmd_region, irc_channel, cmd_args);
                                            region_clean(cmd_region);
                                        } else {
                                            log_warning(&log, "Could not find command `"SV_Fmt"`", SV_Arg(cmd_name));
                                        }
                                    }
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
            }

            read_size = irc_read(&irc, buffer + buffer_size, sizeof(buffer) - buffer_size);
        }

        if (buffer_drops_count >= BUFFER_DROPS_THRESHOLD) {
            log_error(&log, "Server sent too much garbage");
            goto error;
        }

        if (read_size <= 0) {
            if (irc.ssl) {
                char buf[512] = {0};
                ERR_error_string_n(SSL_get_error(irc.ssl, read_size), buf, sizeof(buf));
                log_error(&log, "SSL failed with error: %s", buf);
            } else {
                log_error(&log, "Plain connection failed with error: %s", strerror(errno));
            }
            goto error;
        }
    }

    // Destroy resources and exit with success
    {
        if (secret_conf_content) {
            free(secret_conf_content);
        }

        irc_destroy(&irc);

        if (ctx) {
            SSL_CTX_free(ctx);
        }

        if (curl) {
            curl_easy_cleanup(curl);
        }

        if (curl_global_initalized) {
            curl_global_cleanup();
        }

        if (cmd_region) {
            region_free(cmd_region);
        }
    }

    return 0;

error:

    // Destroy resources and exit with failure
    {
        if (secret_conf_content) {
            free(secret_conf_content);
        }

        irc_destroy(&irc);

        if (ctx) {
            SSL_CTX_free(ctx);
        }

        if (curl) {
            curl_easy_cleanup(curl);
        }

        if (curl_global_initalized) {
            curl_global_cleanup();
        }

        if (cmd_region) {
            region_free(cmd_region);
        }
    }

    return -1;
}
