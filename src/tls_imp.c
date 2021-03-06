#include "./tls_imp.h"

#ifdef B_SSL_OPENSSL

int tls_imp_init(struct tls_imp_global* global) {
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
    global->ctx = SSL_CTX_new(TLS_client_method());

    if (global->ctx == NULL) {
        return 0;
    }
    return 1;
}

int tls_imp_client_init(struct tls_imp_global* global,
                        struct tls_imp_client* client, int fd) {
    client->ssl = SSL_new(global->ctx);
    SSL_set_fd(client->ssl, fd);

    if (SSL_connect(client->ssl) < 0) {
        return 0;
    }
    return 1;
}

size_t tls_imp_read(struct tls_imp_client* client, char* data, size_t size) {
    return SSL_read(client->ssl, data, size);
}

size_t tls_imp_write(struct tls_imp_client* client, const char* data,
                     size_t size) {
    return SSL_write(client->ssl, data, size);
}

int tls_imp_client_close(struct tls_imp_client* client) {
    // TODO: implement tls_imp_client_close for B_SSL_OPENSSL
    return 1;
}

void tls_imp_free(struct tls_imp_global* global) {
    // TODO: implement tls_imp_free for B_SSL_OPENSSL
}

#endif

#ifdef B_SSL_LIBRESSL

int tls_imp_init(struct tls_imp_global* global) {
    if (tls_init() == -1) return 0;
    if ((global->config = tls_config_new()) == NULL) return 0;
    return 1;
}

int tls_imp_client_init(struct tls_imp_global* global,
                        struct tls_imp_client* client, int fd,
                        const char* domainName) {
    if ((client->tls = tls_client()) == NULL) return 0;
    if (tls_configure(client->tls, global->config) == -1) return 0;
    if (tls_connect_socket(client->tls, fd, domainName) == -1) return 0;
    return 1;
}

size_t tls_imp_read(struct tls_imp_client* client, char* data, size_t size) {
    return tls_read(client->tls, data, size);
}

size_t tls_imp_write(struct tls_imp_client* client, const char* data,
                     size_t size) {
    return tls_write(client->tls, data, size);
}

int tls_imp_client_close(struct tls_imp_client* client) {
    if (tls_close(client->tls) < 0) return 0;
    tls_free(client->tls);
    return 1;
}

void tls_imp_free(struct tls_imp_global* global) {
    tls_config_free(global->config);
}

#endif