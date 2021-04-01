#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include "./cmd.h"

char hex_char(char x)
{
    if (0 <= x && x < 10) return x + '0';
    if (10 <= x && x < 16) return x - 10 + 'A';
    return ')';
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

static size_t write_to_region(char *data, size_t size, size_t nmemb, Region *region)
{
    void *dest = region_alloc(region, size * nmemb);
    if (dest == NULL) {
        return 0;
    }
    memcpy(dest, data, size * nmemb);
    return nmemb;
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
    log_info(log, "COMMAND: wttr "SV_Fmt, SV_Arg(args));

    String_View encoded_location = url_encode(memory, args);
    if (encoded_location.data == NULL) {
        log_error(log, "COMMAND: Not enough memory to handle wttr command");
        return;
    }

    String_View wttr_url = SV_CONCAT(memory, SV("https://wttr.in/"), encoded_location, SV("?format=4\0"));
    if (encoded_location.data == NULL) {
        log_error(log, "COMMAND: Not enough memory to handle wttr command");
        return;
    }

    size_t begin_size = memory->size;

    // TODO(#22): use asynchronous CURL
    curl_easy_setopt(curl, CURLOPT_URL, wttr_url.data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_region);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, memory);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        log_error(log, "COMMAND: CURL GET query of "SV_Fmt" has failed failed: %s",
                  SV_Arg(wttr_url),
                  curl_easy_strerror(res));
        return;
    }

    assert(begin_size < memory->size);

    String_View wttr = {
        .count = memory->size - begin_size,
        .data = memory->buffer + begin_size,
    };

    irc_privmsg(irc, channel, wttr);
    log_info(log, "COMMAND: wttr handled successfully");
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
