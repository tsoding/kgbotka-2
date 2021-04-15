#ifndef _WIN32
#define _POSIX_C_SOURCE 200112L
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <fcntl.h>

#include "./irc.h"

void irc_join(Irc *irc, String_View channel)
{
    socket_write_cstr(irc->socket, "JOIN ");
    socket_write_sv(irc->socket, channel);
    socket_write_cstr(irc->socket, "\n");
}

void irc_pass(Irc *irc, String_View password)
{
    socket_write_cstr(irc->socket, "PASS ");
    socket_write_sv(irc->socket, password);
    socket_write_cstr(irc->socket, "\n");
}

void irc_nick(Irc *irc, String_View nickname)
{
    socket_write_cstr(irc->socket, "NICK ");
    socket_write_sv(irc->socket, nickname);
    socket_write_cstr(irc->socket, "\n");
}

void irc_privmsg(Irc *irc, String_View channel, String_View message)
{
    socket_write_cstr(irc->socket, "PRIVMSG ");
    socket_write_sv(irc->socket, channel);
    socket_write_cstr(irc->socket, " :");
    socket_write_sv(irc->socket, message);
    socket_write_cstr(irc->socket, "\n");
}

void irc_pong(Irc *irc, String_View response)
{
    socket_write_cstr(irc->socket, "PONG :");
    socket_write_sv(irc->socket, response);
    socket_write_cstr(irc->socket, "\n");
}

void irc_cap_req(Irc *irc, String_View capabilities)
{
    socket_write_cstr(irc->socket, "CAP REQ :");
    socket_write_sv(irc->socket, capabilities);
    socket_write_cstr(irc->socket, "\n");
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
