/* ftl.h
 * The layer between the program (logical blocks) and the memory (physical
 * blocks). Two implementations: baseline.c with a fixed mapping, and wl.c with
 * wear-levelling. The simulator drives both through this interface.
 */
#ifndef FTL_H
#define FTL_H

#include <stdint.h>
#include "fb.h"

typedef struct {
    const char *name;
    /* Set up the memory and the mapping. Returns 0 on success, -1 on a bad size.
     * The baseline ignores n_spares and fb. */
    int      (*init)(uint16_t n_logical, uint16_t n_spares, uint32_t limit, const fb_ops_t *fb);
    /* Returns 1 when this write brought some block to the write limit. */
    int      (*write)(uint16_t logical, uint16_t value);
    uint16_t (*read)(uint16_t logical);
    /* Internal invariants. Returns 1 when all hold. */
    int      (*check)(void);
} ftl_ops_t;

extern const ftl_ops_t ftl_baseline_ops;   /* baseline.c */
extern const ftl_ops_t ftl_wl_ops;         /* wl.c */

#endif /* FTL_H */
