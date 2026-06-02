#ifndef FM_KEYBOX_H
#define FM_KEYBOX_H

#include <stdint.h>
#include <stdlib.h>

#define FM_ALG_EC  0
#define FM_ALG_RSA 1
#define FM_MAX_CERTS 8

struct fm_cert {
    void *x509;
    uint8_t *der;
    int der_len;
};

struct fm_key {
    int algorithm;
    void *privkey;
    int num_certs;
    struct fm_cert certs[FM_MAX_CERTS];
};

struct fm_ctx {
    int ready;
    struct fm_key key;
};

int fm_keybox_load(struct fm_ctx *ctx, const char *keys_dir);
void fm_keybox_free(struct fm_ctx *ctx);

#endif
