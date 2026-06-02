/* keybox.c — load DER key files using dynamically loaded OpenSSL */
#include "keybox.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

static void *crypto_handle;
static void *(*d2i_auto_priv)(void *, const unsigned char **, long);
static void *(*d2i_x509_fn)(void *, const unsigned char **, long);
static void (*evp_pkey_free)(void *);
static void (*x509_free_fn)(void *);
static int (*evp_pkey_id_fn)(const void *);

static int crypto_init(void)
{
    if (crypto_handle) return 0;
    crypto_handle = dlopen("libcrypto.so", RTLD_NOW | RTLD_LOCAL);
    if (!crypto_handle) {
        LOG("dlopen libcrypto.so failed: %s", dlerror());
        return -1;
    }
    d2i_auto_priv  = dlsym(crypto_handle, "d2i_AutoPrivateKey");
    d2i_x509_fn    = dlsym(crypto_handle, "d2i_X509");
    evp_pkey_free  = dlsym(crypto_handle, "EVP_PKEY_free");
    x509_free_fn   = dlsym(crypto_handle, "X509_free");
    evp_pkey_id_fn = dlsym(crypto_handle, "EVP_PKEY_id");
    if (!d2i_auto_priv || !d2i_x509_fn || !evp_pkey_free || !x509_free_fn || !evp_pkey_id_fn) {
        LOG("dlsym crypto failed: %s", dlerror());
        dlclose(crypto_handle);
        crypto_handle = NULL;
        return -1;
    }
    return 0;
}

static uint8_t *file_read(const char *path, size_t *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc(sz);
    if (!buf) { fclose(f); return NULL; }
    *len = fread(buf, 1, sz, f);
    fclose(f);
    return buf;
}

static int load_privkey(struct fm_key *key, const char *dir)
{
    char path[256];
    const char *suffixes[] = {"ec", "rsa", NULL};

    for (int i = 0; suffixes[i]; i++) {
        snprintf(path, sizeof(path), "%s/privkey_%s.der", dir, suffixes[i]);
        size_t len;
        uint8_t *der = file_read(path, &len);
        if (!der) continue;

        const unsigned char *p = der;
        void *pk = d2i_auto_priv(NULL, &p, len);
        if (pk) {
            key->privkey = pk;
            key->algorithm = i; /* 0=EC, 1=RSA */
            LOG("loaded %s private key from %s", suffixes[i], path);
            free(der);
            return 0;
        }
        LOG("failed parsing %s", path);
        free(der);
    }
    LOG("no private key found in %s", dir);
    return -1;
}

static int load_certs(struct fm_key *key, const char *dir)
{
    char path[256];
    snprintf(path, sizeof(path), "%s/cert_chain.der", dir);

    size_t len;
    uint8_t *buf = file_read(path, &len);
    if (!buf) { LOG("cert_chain.der not found"); return -1; }
    if (len < 4) { free(buf); return -1; }

    uint32_t count = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16)
                   | ((uint32_t)buf[2] << 8)  | (uint32_t)buf[3];
    LOG("cert chain: %u certificates", count);

    key->num_certs = 0;

    size_t pos = 4;
    for (uint32_t i = 0; i < count && i < FM_MAX_CERTS && pos + 4 <= len; i++) {
        uint32_t cert_len = ((uint32_t)buf[pos] << 24) | ((uint32_t)buf[pos + 1] << 16)
                          | ((uint32_t)buf[pos + 2] << 8)  | (uint32_t)buf[pos + 3];
        pos += 4;
        if (pos + cert_len > len) break;

        const unsigned char *p = buf + pos;
        void *cert = d2i_x509_fn(NULL, &p, cert_len);
        if (!cert) {
            LOG("failed parsing cert[%u]", i);
        } else {
            struct fm_cert *fc = &key->certs[key->num_certs++];
            fc->x509 = cert;
            fc->der = malloc(cert_len);
            if (fc->der) {
                memcpy(fc->der, buf + pos, cert_len);
                fc->der_len = cert_len;
            }
        }
        pos += cert_len;
    }

    free(buf);
    LOG("loaded %d certificates", key->num_certs);
    return key->num_certs > 0 ? 0 : -1;
}

int fm_keybox_load(struct fm_ctx *ctx, const char *keys_dir)
{
    memset(ctx, 0, sizeof(*ctx));
    if (crypto_init() < 0) return -1;
    if (load_privkey(&ctx->key, keys_dir) < 0) return -1;
    if (load_certs(&ctx->key, keys_dir) < 0) {
        evp_pkey_free(ctx->key.privkey);
        return -1;
    }
    ctx->ready = 1;
    return 0;
}

void fm_keybox_free(struct fm_ctx *ctx)
{
    for (int i = 0; i < ctx->key.num_certs; i++) {
        if (ctx->key.certs[i].x509) x509_free_fn(ctx->key.certs[i].x509);
        free(ctx->key.certs[i].der);
    }
    if (ctx->key.privkey) evp_pkey_free(ctx->key.privkey);
    memset(ctx, 0, sizeof(*ctx));
}
