#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <string.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "./irc.h"

void SSL_write_cstr(SSL *ssl, const char *cstr)
{
    SSL_write(ssl, cstr, strlen(cstr));
}

void SSL_write_sv(SSL *ssl, String_View sv)
{
    SSL_write(ssl, sv.data, sv.count);
}

bool irc_connect(Log *log, Irc *irc, const char *host, const char *service)
{
    // Resources to destroy at the end
    struct addrinfo *addrs = NULL;

    // Destroy irc just in case before trying to create a new one
    {
        irc_destroy(irc);
    }

    // Plain socket connection
    {
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
            // TODO: don't recreate socket on each attempt
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
    }

    // Upgrade to SSL connection
    {
        // TODO: move this global ssl initialization to main.c
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        irc->ctx = SSL_CTX_new(TLS_client_method());

        if (irc->ctx == NULL) {
            // TODO: SSL_CTX_new error is not located in errno
            log_error(log, "Could not initialize the SSL context: %s",
                      strerror(errno));
            goto error;
        }

        // TODO: SSL_new() can fail
        irc->ssl = SSL_new(irc->ctx);

        // TODO: SSL_set_fd() can fail
        SSL_set_fd(irc->ssl, irc->sd);

        if (SSL_connect(irc->ssl) < 0) {
            // TODO: SSL_connect is not located in errno
            log_error(log, "Could not connect via SSL: %s",
                      strerror(errno));
            goto error;
        }
    }

// ok:
    if (addrs) {
        freeaddrinfo(addrs);
    }

    return true;

error:
    if (addrs) {
        freeaddrinfo(addrs);
    }

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

    if (irc->ctx) {
        SSL_CTX_free(irc->ctx);
        irc->ctx = NULL;
    }

    close(irc->sd);
    irc->sd = 0;
}

void irc_join(Irc *irc, String_View channel)
{
    SSL_write_cstr(irc->ssl, "JOIN ");
    SSL_write_sv(irc->ssl, channel);
    SSL_write_cstr(irc->ssl, "\n");
}

void irc_pass(Irc *irc, String_View password)
{
    SSL_write_cstr(irc->ssl, "PASS ");
    SSL_write_sv(irc->ssl, password);
    SSL_write_cstr(irc->ssl, "\n");
}

void irc_nick(Irc *irc, String_View nickname)
{
    SSL_write_cstr(irc->ssl, "NICK ");
    SSL_write_sv(irc->ssl, nickname);
    SSL_write_cstr(irc->ssl, "\n");
}

void irc_privmsg(Irc *irc, String_View channel, String_View message)
{
    SSL_write_cstr(irc->ssl, "PRIVMSG ");
    SSL_write_sv(irc->ssl, channel);
    SSL_write_cstr(irc->ssl, " :");
    SSL_write_sv(irc->ssl, message);
    SSL_write_cstr(irc->ssl, "\n");
}

void irc_pong(Irc *irc, String_View response)
{
    SSL_write_cstr(irc->ssl, "PONG :");
    SSL_write_sv(irc->ssl, response);
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
