#ifndef IRC_H_
#define IRC_H_

#include <openssl/ssl.h>
#include <openssl/err.h>

typedef struct {
    SSL *ssl;
    SSL_CTX *ctx;
    int sd;
} Irc;

Irc irc_connect(const char *host, const char *service);
void irc_destroy(Irc irc);

#endif // IRC_H_
