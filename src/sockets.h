#ifndef SOCKETS_H_
#define SOCKETS_H_

#include <stdbool.h>
#include "./log.h"
#include "./sv.h"

typedef struct Socket Socket;

Socket *socket_plain_connect(Log *log,
                             const char *host,
                             const char *service,
                             bool nonblocking);

Socket *socket_secure_connect(Log *log,
                              SSL_CTX *ctx,
                              const char *host,
                              const char *service,
                              bool nonblocking);

void socket_log_last_error(Log *log, const Socket *socket, int ret);

bool socket_read_again(Socket *socket, int ret);

void socket_destroy(Socket *socket);
int socket_write(Socket *socket, const void *buffer, size_t buffer_size);
int socket_write_cstr(Socket *socket, const char *cstr);
int socket_write_sv(Socket *socket, String_View sv);
int socket_read(Socket *socket, void *buffer, size_t buffer_size);

#endif // SOCKETS_H_
