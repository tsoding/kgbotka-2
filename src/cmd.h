#ifndef CMD_H_
#define CMD_H_

#include <curl/curl.h>

#include "./irc.h"
#include "./log.h"
#include "./sv.h"
#include "./region.h"

#define CMD_PREFIX "^"

typedef void (*Cmd_Run)(Irc *irc, Log *log, CURL *curl, Region *memory, String_View channel, String_View args);
Cmd_Run find_cmd_by_name(String_View name);
void cmd_ping(Irc *irc, Log *log, CURL *curl, Region *memory, String_View channel, String_View args);
void cmd_wttr(Irc *irc, Log *log, CURL *curl, Region *memory, String_View channel, String_View args);

#endif // CMD_H_
