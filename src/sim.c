/* sim.c
 * Trace replay and correctness checking. This is the only file that includes
 * the trace, so only one trace is compiled into a program. Choose it with
 * -DTRACE_FILE=\"more_traces/trace_hot95.h\" (default is trace.h).
 */
#include "sim.h"
#include "config.h"
#include "memory.h"

#ifndef TRACE_FILE
#define TRACE_FILE "trace.h"
#endif
#include TRACE_FILE

_Static_assert(NUM_LOGICAL_BLOCKS <= MAX_LOGICAL, "trace has more logical blocks than MAX_LOGICAL");

#define CHECK_PERIOD 4096u      /* writes between two full sweeps */

uint16_t sim_num_logical(void)
{
    return NUM_LOGICAL_BLOCKS;
}

uint32_t sim_num_writes(void)
{
    return NUM_WRITES;
}

static uint16_t expected[MAX_LOGICAL];  /* last value written to each logical block */

/* Read every logical block and compare with the last value written to it. */
static uint32_t full_sweep(const ftl_ops_t *ftl)
{
    uint32_t bad = 0;
    uint16_t l;

    for (l = 0; l < NUM_LOGICAL_BLOCKS; l++) {
        if (ftl->read(l) != expected[l]) {
            bad++;
        }
    }
    if (!ftl->check()) {
        bad++;
    }
    return bad;
}

int sim_run(const ftl_ops_t *ftl, const fb_ops_t *fb, uint16_t n_spares,
            uint32_t limit, sim_result_t *out)
{
    uint32_t i;
    uint16_t p;
    uint16_t l;
    uint64_t total = 0;
    uint64_t guard;
    int worn = 0;

    out->lifetime = 0;
    out->max_wear = 0;
    out->sum_wear = 0;
    out->num_phys = 0;
    out->first_dead = BLOCK_NONE;
    out->comparisons = 0;
    out->pool_bytes = 0;
    out->errors = 0;

    for (l = 0; l < NUM_LOGICAL_BLOCKS; l++) {
        expected[l] = 0;
    }
    if (ftl->init(NUM_LOGICAL_BLOCKS, n_spares, limit, fb) != 0) {
        out->errors = 1;
        return -1;
    }
    out->num_phys = nvm_num_phys();
    out->pool_bytes = (fb != 0) ? fb->bytes() : 0;

    /* Each write adds one to one block, so after limit * num_phys writes some
     * block must have reached the limit. Reaching the guard means a bug. */
    guard = (uint64_t)limit * (uint64_t)out->num_phys + 1u;

    while (!worn && total <= guard) {
        for (i = 0; i < NUM_WRITES; i++) {
            l = writes[i].block;
            expected[l] = writes[i].value;
            worn = ftl->write(l, writes[i].value);
            total++;
            if (ftl->read(l) != writes[i].value) {
                out->errors++;
            }
            if (total % CHECK_PERIOD == 0u) {
                out->errors += full_sweep(ftl);
            }
            if (worn) {
                break;
            }
        }
    }
    if (!worn) {
        out->errors++;
    }
    out->errors += full_sweep(ftl);

    out->lifetime = (uint32_t)total;
    for (p = 0; p < out->num_phys; p++) {
        uint32_t w = nvm_wear(p);

        out->sum_wear += w;
        if (w > out->max_wear) {
            out->max_wear = w;
        }
    }
    out->first_dead = nvm_first_dead();
    out->comparisons = (fb != 0) ? fb->comparisons() : 0;
    return (out->errors == 0u) ? 0 : -1;
}
