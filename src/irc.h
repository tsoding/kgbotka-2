#ifndef IRC_H_
#define IRC_H_

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "./sv.h"
#include "./log.h"

typedef struct {
    SSL *ssl;
    int sd;
} Irc;

bool irc_connect_plain(Log *log, Irc *irc, const char *host, const char *service);
bool irc_connect_secure(Log *log, Irc *irc, SSL_CTX *ctx, const char *host,
                        const char *service);
void irc_destroy(Irc *irc);

int irc_read(Irc *irc, void *buf, size_t count);
int irc_write(Irc *irc, const void *buf, size_t count);

void irc_pass(Irc *irc, String_View password);
void irc_join(Irc *irc, String_View channel);
void irc_nick(Irc *irc, String_View nickname);
void irc_privmsg(Irc *irc, String_View channel, String_View message);
void irc_pong(Irc *irc, String_View response);

bool params_next(String_View *params, String_View *output);

#endif // IRC_H_
