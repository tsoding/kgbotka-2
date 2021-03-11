#define _POSIX_C_SOURCE 200112L

#include <string.h>

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "./irc.h"

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
