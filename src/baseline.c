/* baseline.c
 * No wear-levelling. Logical block i is always stored in physical block i,
 * so a hot logical block wears out the same physical block again and again.
 */
#include "ftl.h"
#include "config.h"
#include "memory.h"

static uint16_t num_logical;

static int b_init(uint16_t n_logical, uint16_t n_spares, uint32_t limit, const fb_ops_t *fb)
{
    (void)n_spares;     /* the baseline has no spare blocks */
    (void)fb;
    if (n_logical == 0 || n_logical > MAX_LOGICAL) {
        return -1;
    }
    num_logical = n_logical;
    nvm_init(n_logical, limit);
    return 0;
}

static int b_write(uint16_t logical, uint16_t value)
{
    if (logical >= num_logical) {
        return 0;
    }
    return nvm_write(logical, value);
}

static uint16_t b_read(uint16_t logical)
{
    return (logical < num_logical) ? nvm_read(logical) : 0;
}

static int b_check(void)
{
    return nvm_num_phys() == num_logical;
}

const ftl_ops_t ftl_baseline_ops = { "baseline", b_init, b_write, b_read, b_check };
