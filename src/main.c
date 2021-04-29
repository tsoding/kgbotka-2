#ifndef _WIN32
#define _XOPEN_SOURCE 500
#define _POSIX_C_SOURCE 200112L
#include <unistd.h>
#endif

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
#include "./http.h"
#include "./discord.h"
#include "./json.h"

#define CWS_IMPLEMENTATION
#include "./thirdparty/cws.h"

#define TZOZEN_IMPLEMENTATION
#include "./thirdparty/tzozen.h"

#define JIM_IMPLEMENTATION
#include "./thirdparty/jim.h"

#define ARRAY_LEN(xs) (sizeof(xs) / sizeof((xs)[0]))

#define TWITCH_HOST "irc.chat.twitch.tv"
#define SECURE_TWITCH_PORT "6697"
#define PLAIN_TWITCH_PORT "6667"
#define MAX_RECONNECT_MS 4096000

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

void sleep_ms(unsigned int milliseconds)
{
#ifdef _WIN32
    Sleep(milliseconds);
#else
    usleep(milliseconds * 1000);
#endif
}

String_View cws_message_chunk_to_sv(Cws_Message_Chunk chunk)
{
    return (String_View) {
        .data = (const char *) chunk.payload,
        .count = chunk.payload_len,
    };
}

// TODO: Research on a possibility of using ETF for Discord instead of JSON
// https://discord.com/developers/docs/topics/gateway#etfjson

void connect_discord(CURL *curl, Region *memory, Log *log, SSL_CTX *ctx)
{
    // https://discord.com/developers/docs/topics/gateway#connecting-to-the-gateway
    Socket *discord_socket = NULL;

    const char *url = "https://discord.com/api/gateway";
    Json_Value body = {0};
    if (!curl_get_json(curl, url, memory, &body)) {
        log_error(log, "Could not retrieve Discord gateway");
        goto error;
    }

    String_View gateway_url = {0};
    if (!extract_discord_gateway_url(body, &gateway_url)) {
        log_error(log, "Could not extract discord gateway url from the JSON.");
        goto error;
    }

    if (!sv_cut_prefix(&gateway_url, SV("wss://"))) {
        log_error(log, "Incorrect gateway URL "SV_Fmt, SV_Arg(gateway_url));
        goto error;
    }

    const char *discord_host = region_sv_to_cstr(memory, gateway_url);
    if (!discord_host) {
        log_error(log, "Could not allocate enough memory to connect to Discord");
        goto error;
    }

    const char *discord_port = "443";

    log_info(log, "Trying to connect to Discord...");
    discord_socket = socket_secure_connect(
                         log,
                         ctx,
                         discord_host,
                         discord_port,
                         false);
    if (!discord_socket) {
        log_error(log, "Could not connect to %s:%s", discord_host, discord_port);
        goto error;
    }

    Cws cws = {
        .socket = discord_socket,
        .read = (Cws_Read) socket_read,
        .write = (Cws_Write) socket_write,
        .alloc = cws_malloc,
        .free = cws_free,
    };

    if (cws_client_handshake(&cws, discord_host) < 0) {
        log_error(log, "WebSocket handshake has failed");
        goto error;
    }

    log_info(log, "Connected to Discord successfully");

    Cws_Message message = {0};
    if (cws_read_message(&cws, &message) == 0) {
        Json_Result result = parse_json_value_region_sv(
                                 memory,
                                 cws_message_chunk_to_sv(*message.chunks));
        if (!result.is_error) {
            Discord_Payload payload = {0};
            if (discord_deserialize_payload(result.value, &payload)) {
                log_info(log, "DISCORD SENT: %s", discord_opcode_as_cstr(payload.opcode));
                if (payload.opcode == DISCORD_OPCODE_HELLO) {
                    log_info(log, "HEARTBEAT INTERVAL: %"PRIu64,
                             payload.hello.heartbeat_interval);
                }
            } else {
                log_error(log, "Could not deserialize message from Discord");
            }
        } else {
            log_error(log, "Could not parse WebSocket message from Discord");
        }
    } else {
        log_error(log, "Could not read a message from Discord: %s", cws_get_error_string(&cws));
    }

error:
    if (discord_socket) {
        socket_destroy(discord_socket);
    }
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
#ifdef _WIN32
    bool wsa_initialized = false;
#endif

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

#ifdef _WIN32
    {
        WSADATA wsaData;
        int i_res = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (i_res != 0) {
            log_error(&log, "WSAStartup failed with error: %d", i_res);
            goto error;
        }

        wsa_initialized = true;
    }
#endif

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
            char buf[512] = {0};
            ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
            log_error(&log, "Could not initialize the SSL context: %s", buf);
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

    unsigned int reconnect_ms = 0;
reconnect: {
        sleep_ms(reconnect_ms);
        // Connect to IRC
        {
            irc.socket = socket_secure_connect(&log, ctx, TWITCH_HOST, SECURE_TWITCH_PORT, true);

            if (!irc.socket) {
                if (reconnect_ms == 0) {
                    reconnect_ms = 1000;
                } else if (reconnect_ms < MAX_RECONNECT_MS) {
                    reconnect_ms *= 2;
                }
                log_info(&log, "Trying to reconnect.. (%dms)", reconnect_ms);
                goto reconnect;
            }

            log_info(&log, "Connected to Twitch IRC successfully");

            irc_pass(&irc, secret_password);
            irc_nick(&irc, secret_nickname);
            irc_cap_req(&irc, SV("twitch.tv/tags"));
            irc_join(&irc, secret_channel);
        }

        // Connect to Discord
        connect_discord(curl, cmd_region, &log, ctx);

        reconnect_ms = 0;
    }

    // IRC event loop
    {
#define BUFFER_DROPS_THRESHOLD 5
        char buffer[4096];
        size_t buffer_size = 0;

        int read_size = socket_read(irc.socket, buffer + buffer_size, sizeof(buffer) - buffer_size);
        size_t buffer_drops_count = 0;
        while ((read_size > 0 || socket_read_again(irc.socket, read_size)) &&
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
                            if (sv_starts_with(line, SV("@"))) {
                                String_View tags = sv_chop_by_delim(&line, ' ');
                                (void) tags;
                            }
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

            read_size = socket_read(irc.socket, buffer + buffer_size, sizeof(buffer) - buffer_size);
        }

        if (buffer_drops_count >= BUFFER_DROPS_THRESHOLD) {
            log_error(&log, "Server sent too much garbage");
            goto error;
        }

        if (read_size <= 0) {
            socket_log_last_error(&log, irc.socket, read_size);
            log_info(&log, "Trying to reconnect..");
            goto reconnect;
        }
    }

    // Destroy resources and exit with success
    {
        if (secret_conf_content) {
            free(secret_conf_content);
        }

        if (irc.socket) {
            socket_destroy(irc.socket);
        }

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

#ifdef _WIN32
        if (wsa_initialized) {
            WSACleanup();
        }
#endif
    }

    return 0;

error:

    // Destroy resources and exit with failure
    {
        if (secret_conf_content) {
            free(secret_conf_content);
        }

        if (irc.socket) {
            socket_destroy(irc.socket);
        }

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

#ifdef _WIN32
        if (wsa_initialized) {
            WSACleanup();
        }
#endif
    }

    return -1;
}
