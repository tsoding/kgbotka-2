#ifndef NET_H_
#define NET_H_

#include <stdbool.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "./log.h"

int net_connect_plain(Log *log,
                      const char *host,
                      const char *service,
                      bool nonblocking);
SSL *net_upgrade_to_secure(Log *log,
                           int sd,
                           SSL_CTX *ctx,
                           bool nonblocking);

#endif // NET_H_
