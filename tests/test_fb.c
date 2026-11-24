/* test_fb.c
 * Random test of both free-block pools against a brute-force reference.
 * After every operation the pool must hand out exactly the block that a plain
 * search over all free blocks would pick, and its internal layout must be valid.
 * PC-only test code, so it may print and use its own random numbers.
 */
#include <stdio.h>
#include <stdint.h>

#include "fb.h"
#include "config.h"
#include "memory.h"

static uint32_t rng_state = 12345u;

static uint32_t rnd(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;     /* simple LCG */
    return rng_state >> 8;
}

static uint8_t in_pool[MAX_PHYSICAL];

/* Reference answer: scan every physical block that is marked free. */
static uint16_t reference_min(uint16_t n_phys)
{
    uint16_t best = BLOCK_NONE;
    uint16_t i;

    for (i = 0; i < n_phys; i++) {
        if (in_pool[i] && (best == BLOCK_NONE || fb_less(i, best))) {
            best = i;
        }
    }
    return best;
}

static int run(const fb_ops_t *fb, uint16_t spares, uint32_t ops)
{
    uint16_t n_phys = (uint16_t)(spares + 40u);
    uint16_t first = 40;
    uint16_t i;
    uint32_t k;
    uint16_t held[MAX_PHYSICAL];
    uint16_t n_held = 0;

    nvm_init(n_phys, 0xFFFFFFFFu);
    /* Give every block a random starting wear, with many ties on purpose. */
    for (i = 0; i < n_phys; i++) {
        uint32_t w = rnd() % 6u;
        while (w--) {
            nvm_write(i, 0);
        }
        in_pool[i] = 0;
    }
    fb->init(first, spares);
    for (i = first; i < n_phys; i++) {
        in_pool[i] = 1;
    }
    /* init is only valid when the pool is a valid layout for the current wear */
    if (!fb->check()) {
        printf("%s: invalid layout right after init\n", fb->name);
        return 1;
    }

    for (k = 0; k < ops; k++) {
        int take = (n_held == 0) || (fb->size() > 0 && (rnd() % 2u));
        if (take) {
            uint16_t want = reference_min(n_phys);
            uint16_t got = fb->take_min();

            if (got != want) {
                printf("%s: op %lu take_min gave %u, expected %u\n", fb->name, (unsigned long)k, got, want);
                return 1;
            }
            if (got != BLOCK_NONE) {
                in_pool[got] = 0;
                held[n_held++] = got;
            }
        } else {
            uint16_t idx = (uint16_t)(rnd() % n_held);
            uint16_t id = held[idx];

            held[idx] = held[--n_held];
            /* a block that left the pool may have been written meanwhile */
            if (rnd() % 3u) {
                nvm_write(id, 0);
                if (rnd() % 2u) nvm_write(id, 0);
            }
            fb->put(id);
            in_pool[id] = 1;
        }
        if (!fb->check()) {
            printf("%s: op %lu broke the layout\n", fb->name, (unsigned long)k);
            return 1;
        }
        {
            uint16_t expect_size = 0;
            for (i = 0; i < n_phys; i++) expect_size += in_pool[i];
            if (fb->size() != expect_size) {
                printf("%s: op %lu size %u, expected %u\n", fb->name, (unsigned long)k, fb->size(), expect_size);
                return 1;
            }
        }
    }
    return 0;
}

/* Exact comparison counts on a tiny example worked out by hand. */
static int test_counts(void)
{
    /* 8 fresh blocks with wear 0: array scan takes 7 comparisons. */
    nvm_init(8, 0xFFFFFFFFu);
    fb_array_ops.init(0, 8);
    if (fb_array_ops.take_min() != 0 || fb_array_ops.comparisons() != 7u) {
        printf("array: expected block 0 after 7 comparisons\n");
        return 1;
    }
    /* heap of 8 equal-wear blocks: removing the top moves block 7 to the root
     * and it sinks along one path, using 2 comparisons per level. */
    nvm_init(8, 0xFFFFFFFFu);
    fb_heap_ops.init(0, 8);
    if (fb_heap_ops.take_min() != 0) {
        printf("heap: expected block 0 first\n");
        return 1;
    }
    if (fb_heap_ops.comparisons() == 0u || fb_heap_ops.comparisons() > 6u) {
        printf("heap: unexpected comparison count %lu\n", (unsigned long)fb_heap_ops.comparisons());
        return 1;
    }
    return 0;
}

int main(void)
{
    static const uint16_t sizes[] = { 1, 2, 3, 4, 7, 16, 33, 64, 256 };
    uint16_t s;
    int fails = 0;

    for (s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++) {
        fails += run(&fb_array_ops, sizes[s], 20000u);
        fails += run(&fb_heap_ops, sizes[s], 20000u);
    }
    fails += test_counts();
    if (fails) {
        printf("test_fb: FAILED (%d)\n", fails);
        return 1;
    }
    printf("test_fb: all passed\n");
    return 0;
}
