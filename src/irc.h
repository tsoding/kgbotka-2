#ifndef IRC_H_
#define IRC_H_

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "./sv.h"

typedef struct {
    SSL *ssl;
    SSL_CTX *ctx;
    int sd;
} Irc;

Irc irc_connect(const char *host, const char *service);
void irc_destroy(Irc irc);

void irc_pass(SSL *ssl, String_View password);
void irc_join(SSL *ssl, String_View channel);
void irc_nick(SSL *ssl, String_View nickname);
void irc_privmsg(SSL *ssl, String_View channel, String_View message);
void irc_pong(SSL *ssl, String_View response);

bool params_next(String_View *params, String_View *output);

#endif // IRC_H_
