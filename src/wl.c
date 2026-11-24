/* wl.c
 * Wear-levelling. On every write the data goes to the least-used free physical
 * block, the mapping table is updated, and the old block returns to the free
 * pool. The free pool is a fb_ops_t, so the same code runs with the array scan
 * and with the min-heap.
 *
 * Physical blocks 0 .. n_logical-1 start as the homes of logical blocks
 * 0 .. n_logical-1. Physical blocks n_logical .. n_logical+n_spares-1 start
 * in the free pool.
 */
#include "ftl.h"
#include "config.h"
#include "memory.h"

static uint16_t map[MAX_LOGICAL];       /* map[logical] = physical */
static uint16_t num_logical;
static uint16_t num_phys;
static const fb_ops_t *pool;

static int w_init(uint16_t n_logical, uint16_t n_spares, uint32_t limit, const fb_ops_t *fb)
{
    uint16_t i;

    if (n_logical == 0 || n_logical > MAX_LOGICAL || n_spares == 0 || n_spares > MAX_SPARES || fb == 0) {
        return -1;      /* wear-levelling needs at least one spare block */
    }
    num_logical = n_logical;
    num_phys = (uint16_t)(n_logical + n_spares);
    pool = fb;
    nvm_init(num_phys, limit);
    for (i = 0; i < n_logical; i++) {
        map[i] = i;
    }
    pool->init(n_logical, n_spares);
    return 0;
}

static int w_write(uint16_t logical, uint16_t value)
{
    uint16_t fresh;
    uint16_t old;
    int worn;

    if (logical >= num_logical) {
        return 0;
    }
    fresh = pool->take_min();           /* 1. least-used free block */
    if (fresh == BLOCK_NONE) {
        return 0;                       /* cannot happen with at least one spare */
    }
    old = map[logical];
    worn = nvm_write(fresh, value);     /* 2. write the data there */
    map[logical] = fresh;               /* 3. point the logical block at it */
    pool->put(old);                     /* 4. the old block becomes free */
    return worn;
}

static uint16_t w_read(uint16_t logical)
{
    return (logical < num_logical) ? nvm_read(map[logical]) : 0;
}

/* Every physical block must be either mapped or in the pool, exactly once. */
static int w_check(void)
{
    static uint8_t seen[MAX_PHYSICAL];
    uint16_t i;
    uint16_t id;
    uint16_t total = 0;

    for (i = 0; i < num_phys; i++) {
        seen[i] = 0;
    }
    for (i = 0; i < num_logical; i++) {
        id = map[i];
        if (id >= num_phys || seen[id]) {
            return 0;
        }
        seen[id] = 1;
        total++;
    }
    for (i = 0; i < pool->size(); i++) {
        id = pool->item(i);
        if (id >= num_phys || seen[id]) {
            return 0;
        }
        seen[id] = 1;
        total++;
    }
    return total == num_phys && pool->check();
}

const ftl_ops_t ftl_wl_ops = { "wear-levelling", w_init, w_write, w_read, w_check };
