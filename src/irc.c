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
#include "./net.h"

int irc_read(Irc *irc, void *buf, size_t count)
{
    if (irc->ssl) {
        return SSL_read(irc->ssl, buf, count);
    } else {
        return read(irc->sd, buf, count);
    }
}

int irc_write(Irc *irc, const void *buf, size_t count)
{
    if (irc->ssl) {
        return SSL_write(irc->ssl, buf, count);
    } else {
        return write(irc->sd, buf, count);
    }
}

bool irc_read_again(Irc *irc, int ret)
{
    if (irc->ssl) {
        return SSL_get_error(irc->ssl, ret) == SSL_ERROR_WANT_READ;
    } else {
        return errno == EAGAIN || errno == EWOULDBLOCK;
    }
}

void irc_write_cstr(Irc *irc, const char *cstr)
{
    irc_write(irc, cstr, strlen(cstr));
}

void irc_write_sv(Irc *irc, String_View sv)
{
    irc_write(irc, sv.data, sv.count);
}

bool irc_connect_plain(Log *log, Irc *irc,
                       const char *host, const char *service,
                       bool nonblocking)
{
    irc_destroy(irc);

    irc->sd = net_connect_plain(log, host, service, nonblocking);
    if (irc->sd < 0) {
        goto error;
    }

    return true;
error:
    irc_destroy(irc);
    return false;
}

bool irc_connect_secure(Log *log, Irc *irc, SSL_CTX *ctx,
                        const char *host, const char *service,
                        bool nonblocking)
{
    irc_destroy(irc);

    irc->sd = net_connect_plain(log, host, service, false);
    if (irc->sd < 0) {
        goto error;
    }

    irc->ssl = net_upgrade_to_secure(log, irc->sd, ctx, nonblocking);
    if (irc->ssl == NULL) {
        goto error;
    }

    return true;
error:
    irc_destroy(irc);
    return false;
}

void irc_destroy(Irc *irc)
{
    if (irc->ssl) {
        SSL_set_shutdown(irc->ssl, SSL_RECEIVED_SHUTDOWN | SSL_SENT_SHUTDOWN);
        SSL_shutdown(irc->ssl);
        SSL_free(irc->ssl);
        irc->ssl = NULL;
    }

#ifdef _WIN32
    closesocket(irc->sd);
#else
    close(irc->sd);
#endif
    irc->sd = 0;
}

void irc_join(Irc *irc, String_View channel)
{
    irc_write_cstr(irc, "JOIN ");
    irc_write_sv(irc, channel);
    irc_write_cstr(irc, "\n");
}

void irc_pass(Irc *irc, String_View password)
{
    irc_write_cstr(irc, "PASS ");
    irc_write_sv(irc, password);
    irc_write_cstr(irc, "\n");
}

void irc_nick(Irc *irc, String_View nickname)
{
    irc_write_cstr(irc, "NICK ");
    irc_write_sv(irc, nickname);
    irc_write_cstr(irc, "\n");
}

void irc_privmsg(Irc *irc, String_View channel, String_View message)
{
    irc_write_cstr(irc, "PRIVMSG ");
    irc_write_sv(irc, channel);
    irc_write_cstr(irc, " :");
    irc_write_sv(irc, message);
    irc_write_cstr(irc, "\n");
}

void irc_pong(Irc *irc, String_View response)
{
    irc_write_cstr(irc, "PONG :");
    irc_write_sv(irc, response);
    irc_write_cstr(irc, "\n");
}

void irc_cap_req(Irc *irc, String_View capabilities)
{
    irc_write_cstr(irc, "CAP REQ :");
    irc_write_sv(irc, capabilities);
    irc_write_cstr(irc, "\n");
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
