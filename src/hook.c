#include "hook.h"
#include "log.h"
#include "raplt.h"
#include <string.h>
#include <stddef.h>
#include <sys/mman.h>

typedef struct AIBinder AIBinder;
typedef struct AParcel AParcel;
typedef int32_t transaction_code_t;
typedef int32_t binder_status_t;

typedef AIBinder *(*get_service_t)(const char *instance);
typedef binder_status_t (*transact_t)(AIBinder *, transaction_code_t,
                                       AParcel **, AParcel **, uint32_t);
typedef binder_status_t (*read_ba_t)(const AParcel *, int32_t *, const uint8_t **);
typedef binder_status_t (*read_buf_t)(const AParcel *, const uint8_t **, int32_t *);

__attribute__((weak)) binder_status_t AParcel_getData(const AParcel *, const uint8_t **, int32_t *);
__attribute__((weak)) binder_status_t AParcel_readByteArray(const AParcel *, int32_t *, const uint8_t **);
__attribute__((weak)) binder_status_t AParcel_readBuffer(const AParcel *, const uint8_t **, int32_t *);

static struct fm_ctx *g_ctx;
static get_service_t g_orig_get_service;
static transact_t g_orig_transact;
static read_ba_t g_orig_read_ba;
static read_buf_t g_orig_read_buf;
static AIBinder *g_keymint_binder;

#define KEYMINT_INST "android.hardware.security.keymint.IKeyMintDevice"
#define TXN_GENERATE_KEY 3

static _Thread_local int g_rp; /* 0=idle, 1=reading genkey reply */
static _Thread_local int g_rc; /* read count within this reply */

static AIBinder *hooked_get_service(const char *instance)
{
    AIBinder *binder = g_orig_get_service(instance);
    if (binder && strstr(instance, KEYMINT_INST))
        g_keymint_binder = binder;
    return binder;
}

static void replace_with_certs(const uint8_t **data, int32_t *len)
{
    if (!g_ctx || !g_ctx->ready || g_ctx->key.num_certs == 0) return;
    int idx = g_rc - 2; /* read #0=keyBlob, #1=char, #2+=certs */
    if (idx < 0 || idx >= g_ctx->key.num_certs) return;
    struct fm_cert *fc = &g_ctx->key.certs[idx];
    *data = fc->der;
    *len = fc->der_len;
    LOG("replaced cert[%d] with %d bytes", idx, fc->der_len);
}

static binder_status_t hooked_read_ba(const AParcel *parcel,
                                       int32_t *numElements,
                                       const uint8_t **arrayData)
{
    binder_status_t s = g_orig_read_ba(parcel, numElements, arrayData);
    if (s == 0 && g_rp == 1) {
        g_rc++;
        if (g_rc >= 2) replace_with_certs(arrayData, numElements);
    }
    return s;
}

static binder_status_t hooked_read_buf(const AParcel *parcel,
                                        const uint8_t **data,
                                        int32_t *numElements)
{
    binder_status_t s = g_orig_read_buf(parcel, data, numElements);
    if (s == 0 && g_rp == 1) {
        g_rc++;
        if (g_rc >= 2) replace_with_certs(data, numElements);
    }
    return s;
}

static binder_status_t hooked_transact(AIBinder *binder, transaction_code_t code,
                                        AParcel **in, AParcel **out, uint32_t flags)
{
    int want = (binder == g_keymint_binder && code == TXN_GENERATE_KEY);
    if (want) LOGD(">> generateKey");

    binder_status_t status = g_orig_transact(binder, code, in, out, flags);

    if (want && status == 0) {
        g_rp = 1;
        g_rc = 0;
        if (AParcel_getData) {
            const uint8_t *raw;
            int32_t sz;
            AParcel_getData(*out, &raw, &sz);
            LOG("<< generateKey reply=%d bytes", sz);
        }
    }

    return status;
}

static void try_hook_sym(const char *name, void *hook, void **orig)
{
    raplt_hook_t *h = raplt_register(NULL, name, hook, orig, RAPLT_FLAG_NONE);
    if (!h) LOG("warn: %s not found", name);
}

int fm_hook_init(struct fm_ctx *ctx)
{
    g_ctx = ctx;

    try_hook_sym("AServiceManager_getService",
                 (void *)hooked_get_service, (void **)&g_orig_get_service);
    try_hook_sym("AIBinder_transact",
                 (void *)hooked_transact, (void **)&g_orig_transact);

    if (AParcel_readByteArray)
        try_hook_sym("AParcel_readByteArray",
                     (void *)hooked_read_ba, (void **)&g_orig_read_ba);

    if (AParcel_readBuffer)
        try_hook_sym("AParcel_readBuffer",
                     (void *)hooked_read_buf, (void **)&g_orig_read_buf);

    if (raplt_commit() != 0) {
        LOG("raplt_commit failed");
        return -1;
    }

    LOG("hooks installed: getService + transact"
        "%s%s",
        g_orig_read_ba ? " + readByteArray" : "",
        g_orig_read_buf ? " + readBuffer" : "");
    return 0;
}
