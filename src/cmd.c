#include "./cmd.h"

void cmd_ping(Irc *irc, Log *log, String_View channel, String_View args)
{
    (void) log;
    (void) args;
    irc_privmsg(irc, channel, SV("pong"));
}

void cmd_wttr(Irc *irc, Log *log, String_View channel, String_View args)
{
    (void) irc;
    (void) channel;
    (void) args;
    log_warning(log, "wttr command is not implemented yet");
}

Cmd_Run find_cmd_by_name(String_View name)
{
    if (sv_eq(name, SV("ping"))) {
        return cmd_ping;
    } else if (sv_eq(name, SV("wtter"))) {
        return cmd_wttr;
    } else {
        return NULL;
    }
}
