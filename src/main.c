#include "keybox.h"
#include "hook.h"
#include "log.h"
#include "raplt.h"
#include <stdbool.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

static int (*g_test_orig_ioctl)(int, unsigned long, ...);

static int test_hooked_ioctl(int fd, unsigned long request, void *arg)
{
    return g_test_orig_ioctl(fd, request, arg);
}

static void run_diagnostics(void)
{
    LOG("=== diagnostics start ===");

    void *orig = NULL;
    raplt_hook_t *h = raplt_register(".*libbinder\\.so$", "ioctl",
                                       (void *)test_hooked_ioctl,
                                       &orig, RAPLT_FLAG_BATCH);
    if (!h) {
        LOG("cross-lib register: NO GOT ioctl in libbinder.so");
    } else {
        int rc = raplt_commit();
        LOG("cross-lib commit: %s", rc == 0 ? "SUCCESS" : "FAILED");
        if (rc == 0) {
            g_test_orig_ioctl = (__typeof(g_test_orig_ioctl))orig;
            LOG("test hook OK, unregistering...");
            raplt_unregister(h);
        }
    }

    int mem_fd = open("/proc/self/mem", O_RDWR);
    if (mem_fd < 0) {
        LOG("procselfmem open: FAIL errno=%d", errno);
    } else {
        volatile int tv = 0x1234;
        ssize_t w = pwrite(mem_fd, &(int){0x5678}, sizeof(int), (off_t)&tv);
        LOG("procselfmem write: %s tv=%x",
            (w == sizeof(int) && tv == 0x5678) ? "PASS" : "FAIL", tv);
        close(mem_fd);
    }

    LOG("=== diagnostics end ===");
}

__attribute__((visibility("default"))) bool fm_entry(void *handle)
{
    (void)handle;
    static struct fm_ctx ctx;

    run_diagnostics();

    if (fm_hook_init(&ctx) < 0) {
        LOG("hook init failed");
        return false;
    }

    LOG("hooks installed");
    return true;
}
