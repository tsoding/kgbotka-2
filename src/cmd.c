#include <assert.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "./cmd.h"
#include "./http.h"

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

    // TODO(#22): use asynchronous CURL

    String_View wttr = {0};

    CURLcode res = curl_get(curl, wttr_url.data, memory, &wttr);
    if (res != CURLE_OK) {
        log_error(log, "COMMAND: could not perform wttr query\n");
        return;
    }

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
