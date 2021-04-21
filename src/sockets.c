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

#include <assert.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "./sockets.h"

struct Socket {
    SSL *ssl;
    int sd;
};

Socket *socket_plain_connect(Log *log,
                             const char *host,
                             const char *service,
                             bool nonblocking)
{
    struct addrinfo *addrs = NULL;
    Socket *result = calloc(1, sizeof(Socket));
    if (result == NULL) {
        log_error(log, "Could not allocate memory for the socket structure: %s",
                  strerror(errno));
        goto error;
    }

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(host, service, &hints, &addrs) != 0) {
        log_error(log, "Could not get address of `%s`: %s", host, strerror(errno));
        goto error;
    }

    for (struct addrinfo *addr = addrs; addr != NULL; addr = addr->ai_next) {
        // TODO(#23): don't recreate socket on each attempt
        // Just create a single socket with the appropriate family and type
        // and keep using it.
        result->sd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if (result->sd == -1) {
            break;
        }

        if (connect(result->sd, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }

        close(result->sd);
        result->sd = -1;
    }

    if (result->sd == -1) {
        log_error(log, "Could not connect to %s:%s: %s",
                  host, service, strerror(errno));
        goto error;
    }

    if (nonblocking) {
#ifndef _WIN32
        int flag = fcntl(result->sd, F_GETFL);
        if (flag < 0) {
            log_error(log, "Could not get flags of socket: %s\n",
                      strerror(errno));
            goto error;
        }

        if (fcntl(result->sd, F_SETFL, flag | O_NONBLOCK) < 0) {
            log_error(log, "Could not make the socket non-blocking: %s\n",
                      strerror(errno));
            goto error;
        }
#else
        u_long mode = 1;
        int i_res = ioctlsocket(result->sd, FIONBIO, &mode);
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

    return result;
error:

    if (addrs) {
        freeaddrinfo(addrs);
    }

    if (result) {
        free(result);
    }

    return NULL;
}

Socket *socket_secure_connect(Log *log,
                              SSL_CTX *ctx,
                              const char *host,
                              const char *service,
                              bool nonblocking)
{
    Socket *result = socket_plain_connect(log, host, service, false);
    if (result == NULL) {
        goto error;
    }

    // Upgrade to SSL connection
    {
        assert(result->ssl == NULL);
        result->ssl = SSL_new(ctx);
        if(!result->ssl) {
            char buf[512] = {0};
            ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
            log_error(log, "Could not create a SSL structure: %s", buf);
            goto error;
        }

        if (!SSL_set_fd(result->ssl, result->sd)) {
            char buf[512] = {0};
            ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
            log_error(log, "Could not set the SSL file descriptor: %s", buf);
            goto error;
        }

        int ret = SSL_connect(result->ssl);
        if (ret < 0) {
            char buf[512] = {0};
            ERR_error_string_n(SSL_get_error(result->ssl, ret), buf, sizeof(buf));
            log_error(log, "Could not connect via SSL: %s", buf);
            goto error;
        }

    }

    // Mark it as non-blocking
    if (nonblocking) {
#ifndef _WIN32
        int flag = fcntl(result->sd, F_GETFL);
        if (flag < 0) {
            log_error(log, "Could not get flags of socket: %s\n",
                      strerror(errno));
            goto error;
        }

        if (fcntl(result->sd, F_SETFL, flag | O_NONBLOCK) < 0) {
            log_error(log, "Could not make the socket non-blocking: %s\n",
                      strerror(errno));
            goto error;
        }
#else
        u_long mode = 1;
        int i_res = ioctlsocket(result->sd, FIONBIO, &mode);
        if (i_res != NO_ERROR) {
            log_error(log, "Could not make the socket non-blocking: %d", i_res);
            goto error;
        }
#endif
        log_info(log, "Marked the socket as non-blocking");
    }

    return result;
error:
    if (result) {
        socket_destroy(result);
    }
    return NULL;
}

void socket_destroy(Socket *socket)
{
    if (socket->ssl) {
        SSL_set_shutdown(socket->ssl, SSL_RECEIVED_SHUTDOWN | SSL_SENT_SHUTDOWN);
        SSL_shutdown(socket->ssl);
        SSL_free(socket->ssl);
    }

    if (socket->sd >= 0) {
#ifdef _WIN32
        closesocket(socket->sd);
#else
        close(socket->sd);
#endif
    }

    free(socket);
}

int socket_write(Socket *socket, const void *buffer, size_t buffer_size)
{
    if (socket->ssl) {
        return SSL_write(socket->ssl, buffer, buffer_size);
    } else {
        return write(socket->sd, buffer, buffer_size);
    }
}
int socket_write_cstr(Socket *socket, const char *cstr)
{
    return socket_write(socket, cstr, strlen(cstr));
}

int socket_write_sv(Socket *socket, String_View sv)
{
    return socket_write(socket, sv.data, sv.count);
}

int socket_read(Socket *socket, void *buffer, size_t buffer_size)
{
    if (socket->ssl) {
        return SSL_read(socket->ssl, buffer, buffer_size);
    } else {
        return read(socket->sd, buffer, buffer_size);
    }
}

void socket_log_last_error(Log *log, const Socket *socket, int ret)
{
    if (socket->ssl) {
        char buf[512] = {0};
        ERR_error_string_n(SSL_get_error(socket->ssl, ret), buf, sizeof(buf));
        log_error(log, "SSL failed with error: %s", buf);
    } else {
        log_error(log, "Plain connection failed with error: %s", strerror(errno));
    }
}

bool socket_read_again(Socket *socket, int ret)
{
    if (socket->ssl) {
        return SSL_get_error(socket->ssl, ret) == SSL_ERROR_WANT_READ;
    } else {
        return errno == EAGAIN || errno == EWOULDBLOCK;
    }
}
