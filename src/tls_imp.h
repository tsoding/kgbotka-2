
#include <sys/types.h>

#ifdef B_SSL_OPENSSL

#include <openssl/err.h>
#include <openssl/ssl.h>

struct tls_imp_global {
    SSL_CTX* ctx;
};

struct tls_imp_client {
    SSL* ssl;
};

#endif

#ifdef B_SSL_LIBRESSL

#include <tls.h>

struct tls_imp_global {
    struct tls_config* config;
};

struct tls_imp_client {
    struct tls* tls;
};

#endif

int tls_imp_init(struct tls_imp_global* global);
int tls_imp_client_init(struct tls_imp_global* global,
                        struct tls_imp_client* client, int fd,
                        const char* domainName);
size_t tls_imp_read(struct tls_imp_client* client, char* data, size_t size);
size_t tls_imp_write(struct tls_imp_client* client, const char* data,
                     size_t size);
int tls_imp_client_close(struct tls_imp_client* client);
void tls_imp_free(struct tls_imp_global* global);