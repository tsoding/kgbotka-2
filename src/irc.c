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

Irc irc_connect(const char *host, const char *service)
{
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo *addrs;
    if (getaddrinfo(host, service, &hints, &addrs) < 0) {
        fprintf(stderr, "ERROR: Could not get address of `%s`: %s",
                host, strerror(errno));
        exit(1);
    }

    Irc result = {0};

    result.sd = 0;
    for (struct addrinfo *addr = addrs; addr != NULL; addr = addr->ai_next) {
        // TODO: don't recreate socket on each attempt
        // Just create a single socket with the appropriate family and type
        // and keep using it.
        result.sd = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);

        if (result.sd == -1) {
            break;
        }

        if (connect(result.sd, addr->ai_addr, addr->ai_addrlen) == 0) {
            break;
        }

        close(result.sd);
        result.sd = -1;
    }
    freeaddrinfo(addrs);

    if (result.sd == -1) {
        fprintf(stderr, "ERROR: Could not connect to %s:%s: %s",
                host, service, strerror(errno));
        exit(1);
    }

    //////////////////////////////

    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    result.ctx = SSL_CTX_new(TLS_client_method());

    if (result.ctx == NULL) {
        fprintf(stderr, "ERROR: Could not initialize the SSL context: %s",
                strerror(errno));
        exit(1);
    }

    result.ssl = SSL_new(result.ctx);
    SSL_set_fd(result.ssl, result.sd);

    if (SSL_connect(result.ssl) < 0) {
        fprintf(stderr, "ERROR: Could not connect via SSL: %s",
                strerror(errno));
        exit(1);
    }

    return result;
}

void irc_destroy(Irc irc)
{
    SSL_set_shutdown(irc.ssl, SSL_RECEIVED_SHUTDOWN | SSL_SENT_SHUTDOWN);
    SSL_shutdown(irc.ssl);
    SSL_free(irc.ssl);
    SSL_CTX_free(irc.ctx);

    close(irc.sd);
}

void irc_join(Irc *irc, String_View channel)
{
    SSL_write(irc->ssl, "JOIN ", 5);
    SSL_write(irc->ssl, channel.data, channel.count);
    SSL_write(irc->ssl, "\n", 1);
}

void irc_pass(Irc *irc, String_View password)
{
    SSL_write(irc->ssl, "PASS ", 5);
    SSL_write(irc->ssl, password.data, password.count);
    SSL_write(irc->ssl, "\n", 1);
}

void irc_nick(Irc *irc, String_View nickname)
{
    SSL_write(irc->ssl, "NICK ", 5);
    SSL_write(irc->ssl, nickname.data, nickname.count);
    SSL_write(irc->ssl, "\n", 1);
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
