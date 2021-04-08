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

#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <fcntl.h>

#include "./net.h"

int net_connect_plain(Log *log, const char *host, const char *service, bool nonblocking)
{
    struct addrinfo *addrs = NULL;

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host, service, &hints, &addrs) < 0) {
        log_error(log, "Could not get address of `%s`: %s", host, strerror(errno));
        goto error;
    }

    int sd = 0;
    for (struct addrinfo *addr = addrs; addr != NULL; addr = addr->ai_next) {
        // TODO(#23): don't recreate socket on each attempt
        // Just create a single socket with the appropriate family and type
        // and keep using it.
        sd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if (sd == -1) {
            break;
        }

        if (connect(sd, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }

        close(sd);
        sd = -1;
    }

    if (sd == -1) {
        log_error(log, "Could not connect to %s:%s: %s",
                  host, service, strerror(errno));
        goto error;
    }

    if (nonblocking) {
#ifndef _WIN32
        int flag = fcntl(sd, F_GETFL);
        if (flag < 0) {
            log_error(log, "Could not get flags of socket: %s\n",
                      strerror(errno));
            goto error;
        }

        if (fcntl(sd, F_SETFL, flag | O_NONBLOCK) < 0) {
            log_error(log, "Could not make the socket non-blocking: %s\n",
                      strerror(errno));
            goto error;
        }
#else
        u_long mode = 1;
        int i_res = ioctlsocket(sd, FIONBIO, &mode);
        if (i_res != NO_ERROR) {
            log_error(log, "Could not make the socket non-blocking: %d", i_res);
            goto error;
        }
#endif
        log_info(log, "Marked the socket as non-blocking");
    }

    if (addrs) {
        freeaddrinfo(addrs);
    }

    return sd;
error:

    if (addrs) {
        freeaddrinfo(addrs);
    }

    return -1;
}

SSL *net_upgrade_to_secure(Log *log, int sd, SSL_CTX *ctx, bool nonblocking)
{
    SSL *ssl = NULL;

    // Upgrade to SSL connection
    {
        ssl = SSL_new(ctx);
        if(!ssl) {
            char buf[512] = {0};
            ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
            log_error(log, "Could not create a SSL structure: %s", buf);
            goto error;
        }

        if (!SSL_set_fd(ssl, sd)) {
            char buf[512] = {0};
            ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
            log_error(log, "Could not set the SSL file descriptor: %s", buf);
            goto error;
        }

        int ret = SSL_connect(ssl);
        if (ret < 0) {
            char buf[512] = {0};
            ERR_error_string_n(SSL_get_error(ssl, ret), buf, sizeof(buf));
            log_error(log, "Could not connect via SSL: %s", buf);
            goto error;
        }

    }

    // Mark it as non-blocking
    if (nonblocking) {
#ifndef _WIN32
        int flag = fcntl(sd, F_GETFL);
        if (flag < 0) {
            log_error(log, "Could not get flags of socket: %s\n",
                      strerror(errno));
            goto error;
        }

        if (fcntl(sd, F_SETFL, flag | O_NONBLOCK) < 0) {
            log_error(log, "Could not make the socket non-blocking: %s\n",
                      strerror(errno));
            goto error;
        }
#else
        u_long mode = 1;
        int i_res = ioctlsocket(sd, FIONBIO, &mode);
        if (i_res != NO_ERROR) {
            log_error(log, "Could not make the socket non-blocking: %d", i_res);
            goto error;
        }
#endif
        log_info(log, "Marked the socket as non-blocking");
    }

    return ssl;
error:
    if (ssl) {
        SSL_set_shutdown(ssl, SSL_RECEIVED_SHUTDOWN | SSL_SENT_SHUTDOWN);
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = NULL;
    }

    return ssl;
}
