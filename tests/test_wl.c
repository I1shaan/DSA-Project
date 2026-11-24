/* test_wl.c
 * The worked example from the project brief: 4 physical blocks, logical block
 * 0 is written eight times.
 *   baseline         write counts 8, 0, 0, 0
 *   wear-levelling   write counts 2, 2, 2, 2   (with either free-block pool)
 * PC-only test code.
 */
#include <stdio.h>
#include <stdint.h>

#include "ftl.h"
#include "fb.h"
#include "memory.h"

static int expect(const char *what, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    if (nvm_wear(0) != a || nvm_wear(1) != b || nvm_wear(2) != c || nvm_wear(3) != d) {
        printf("%s: got %lu %lu %lu %lu, expected %lu %lu %lu %lu\n", what,
               (unsigned long)nvm_wear(0), (unsigned long)nvm_wear(1),
               (unsigned long)nvm_wear(2), (unsigned long)nvm_wear(3),
               (unsigned long)a, (unsigned long)b, (unsigned long)c, (unsigned long)d);
        return 1;
    }
    return 0;
}

static int example(const ftl_ops_t *ftl, const fb_ops_t *fb, uint16_t spares,
                   const char *what, uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    uint16_t v;
    int bad = 0;

    if (ftl->init(1, spares, 1000u, fb) != 0) {
        printf("%s: init failed\n", what);
        return 1;
    }
    for (v = 1; v <= 8; v++) {
        ftl->write(0, (uint16_t)(v * 100u));
        if (ftl->read(0) != (uint16_t)(v * 100u)) {
            printf("%s: read after write %u failed\n", what, v);
            bad++;
        }
        if (!ftl->check()) {
            printf("%s: invariant broken after write %u\n", what, v);
            bad++;
        }
    }
    return bad + expect(what, a, b, c, d);
}

int main(void)
{
    int fails = 0;

    fails += example(&ftl_baseline_ops, 0, 0, "baseline", 8, 0, 0, 0);
    fails += example(&ftl_wl_ops, &fb_array_ops, 3, "wl + array", 2, 2, 2, 2);
    fails += example(&ftl_wl_ops, &fb_heap_ops, 3, "wl + heap", 2, 2, 2, 2);

    /* wear-levelling cannot work without a spare block */
    if (ftl_wl_ops.init(4, 0, 1000u, &fb_array_ops) == 0) {
        printf("wl accepted zero spares\n");
        fails++;
    }
    if (fails) {
        printf("test_wl: FAILED (%d)\n", fails);
        return 1;
    }
    printf("test_wl: all passed\n");
    return 0;
}
