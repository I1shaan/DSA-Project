/* sim.h
 * Replays the trace again and again until the first block reaches the write
 * limit. Total writes done by then is the lifetime. Every write is checked:
 * reading the block back must give the value just written, and a full sweep
 * of all logical blocks runs at regular intervals and at the end.
 */
#ifndef SIM_H
#define SIM_H

#include <stdint.h>
#include "ftl.h"

typedef struct {
    uint32_t lifetime;        /* writes done until the first block hit the limit */
    uint32_t max_wear;        /* write count of the busiest block */
    uint64_t sum_wear;        /* total of all write counts */
    uint16_t num_phys;        /* physical blocks in use */
    uint16_t first_dead;      /* first block to reach the limit */
    uint64_t comparisons;     /* key comparisons made by the free-block pool */
    uint32_t pool_bytes;      /* storage of the free-block pool at the start */
    uint32_t errors;          /* failed reads or failed invariant checks */
} sim_result_t;

uint16_t sim_num_logical(void);
uint32_t sim_num_writes(void);

/* Run one experiment. fb may be 0 for the baseline. Returns 0 when the run
 * finished and every check passed, otherwise -1. */
int sim_run(const ftl_ops_t *ftl, const fb_ops_t *fb, uint16_t n_spares,
            uint32_t limit, sim_result_t *out);

#endif /* SIM_H */
