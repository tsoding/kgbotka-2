#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include "./cmd.h"

char hex_char(char x)
{
    if (0 <= x && x < 10) return x + '0';
    if (10 <= x && x < 16) return x - 10 + 'A';
    return '0';
}

String_View url_encode(Region *memory, String_View sv)
{
    const size_t WIDTH = 3;
    char *result = region_alloc(memory, sv.count * WIDTH);
    if (result == NULL) {
        return SV_NULL;
    }

    for (size_t i = 0; i < sv.count; ++i) {
        result[i * WIDTH + 0] = '%';
        result[i * WIDTH + 1] = hex_char(((sv.data[i]) >> 4) & 0xf);
        result[i * WIDTH + 2] = hex_char((sv.data[i]) & 0xf);
    }

    return (String_View) {
        .count = sv.count * WIDTH,
        .data = result,
    };
}

void cmd_ping(Irc *irc, Log *log, CURL *curl, Region *memory, String_View channel, String_View args)
{
    (void) log;
    (void) args;
    (void) curl;
    (void) memory;
    irc_privmsg(irc, channel, SV("pong"));
}

void cmd_wttr(Irc *irc, Log *log, CURL *curl, Region *memory, String_View channel, String_View args)
{
    (void) irc;
    (void) channel;
    (void) args;
    (void) curl;
    String_View encoded_location = url_encode(memory, args);
    if (encoded_location.data == NULL) {
        log_error(log, "Not enough memory to build wttr URL");
        return;
    }

    String_View wttr_url = SV_CONCAT(memory, SV("https://wttr.in/"), encoded_location, SV("?format=4\0"));
    if (encoded_location.data == NULL) {
        log_error(log, "Not enough memory to build wttr URL");
        return;
    }

    // curl_easy_setopt(curl, CURLOPT_URL, wttr_url.data);
    // curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_string_buffer_write_callback);
    // curl_easy_setopt(curl, CURLOPT_WRITEDATA, string_buffer);
}

Cmd_Run find_cmd_by_name(String_View name)
{
    if (sv_eq(name, SV("ping"))) {
        return cmd_ping;
    } else if (sv_eq(name, SV("wttr"))) {
        return cmd_wttr;
    } else {
        return NULL;
    }
}
