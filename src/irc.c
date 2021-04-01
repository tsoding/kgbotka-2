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

    // Resources to destroy at the end
    struct addrinfo *addrs = NULL;

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host, service, &hints, &addrs) < 0) {
        log_error(log, "Could not get address of `%s`: %s", host, strerror(errno));
        goto error;
    }

    irc->sd = 0;
    for (struct addrinfo *addr = addrs; addr != NULL; addr = addr->ai_next) {
        // TODO(#23): don't recreate socket on each attempt
        // Just create a single socket with the appropriate family and type
        // and keep using it.
        irc->sd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if (irc->sd == -1) {
            break;
        }

        if (connect(irc->sd, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }

        close(irc->sd);
        irc->sd = -1;
    }

    if (irc->sd == -1) {
        log_error(log, "Could not connect to %s:%s: %s",
                  host, service, strerror(errno));
        goto error;
    }

#ifndef _WIN32
    if (nonblocking) {
        int flag = fcntl(irc->sd, F_GETFL);
        if (flag < 0) {
            log_error(log, "Could not get flags of socket: %s\n",
                      strerror(errno));
            goto error;
        }

        if (fcntl(irc->sd, F_SETFL, flag | O_NONBLOCK) < 0) {
            log_error(log, "Could not make the socket non-blocking: %s\n",
                      strerror(errno));
            goto error;
        }

        log_info(log, "Marked the socket as non-blocking");
    }
#endif

    if (addrs) {
        freeaddrinfo(addrs);
    }

    return true;
error:

    if (addrs) {
        freeaddrinfo(addrs);
    }

    return false;
}

bool irc_connect_secure(Log *log, Irc *irc, SSL_CTX *ctx,
                        const char *host, const char *service,
                        bool nonblocking)
{
    irc_connect_plain(log, irc, host, service, false);

    // Upgrade to SSL connection
    {
        irc->ssl = SSL_new(ctx);
        if(!irc->ssl) {
            char buf[512] = {0};
            ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
            log_error(log, "Could not create a SSL structure: %s", buf);
            goto error;
        }

        if (!SSL_set_fd(irc->ssl, irc->sd)) {
            char buf[512] = {0};
            ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
            log_error(log, "Could not set the SSL file descriptor: %s", buf);
            goto error;
        }

        int ret = SSL_connect(irc->ssl);
        if (ret < 0) {
            char buf[512] = {0};
            ERR_error_string_n(SSL_get_error(irc->ssl, ret), buf, sizeof(buf));
            log_error(log, "Could not connect via SSL: %s", buf);
            goto error;
        }

    }

    // Mark it as non-blocking
#ifndef _WIN32
    // TODO(#26): nonblocking is not supported on windows
    if (nonblocking) {
        int flag = fcntl(irc->sd, F_GETFL);
        if (flag < 0) {
            log_error(log, "Could not get flags of socket: %s\n",
                      strerror(errno));
            goto error;
        }

        if (fcntl(irc->sd, F_SETFL, flag | O_NONBLOCK) < 0) {
            log_error(log, "Could not make the socket non-blocking: %s\n",
                      strerror(errno));
            goto error;
        }

        log_info(log, "Marked the socket as non-blocking");
    }
#endif

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
