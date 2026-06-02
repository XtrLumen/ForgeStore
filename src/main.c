#include "keybox.h"
#include "hook.h"
#include "log.h"
#include <stdbool.h>

__attribute__((visibility("default"))) bool fm_entry(void *handle)
{
    (void)handle;
    static struct fm_ctx ctx;

    if (fm_hook_init(&ctx) < 0) {
        LOG("hook init failed");
        return false;
    }

    LOG("hooks installed");
    return true;
}
