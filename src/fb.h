/* fb.h
 * Interface of the free-block pool. The wear-levelling code only talks to this
 * interface, so the array scan (Student A) and the min-heap (Student B) can be
 * swapped without touching anything else.
 *
 * The pool holds ids of free physical blocks. take_min removes and returns the
 * free block with the lowest write count. Ties go to the lowest block id, so
 * every implementation picks exactly the same block.
 */
#ifndef FB_H
#define FB_H

#include <stdint.h>
#include "memory.h"

typedef struct {
    const char *name;
    /* The pool starts with ids first, first+1, ..., first+count-1. */
    void     (*init)(uint16_t first, uint16_t count);
    uint16_t (*take_min)(void);          /* BLOCK_NONE when the pool is empty */
    void     (*put)(uint16_t id);
    uint16_t (*size)(void);
    uint16_t (*item)(uint16_t i);        /* i-th stored id, for invariant checks */
    uint64_t (*comparisons)(void);       /* key comparisons made since init */
    uint32_t (*bytes)(void);             /* storage used by the structure */
    int      (*check)(void);             /* 1 when the internal layout is valid */
} fb_ops_t;

extern const fb_ops_t fb_array_ops;      /* fb_array.c, Student A */
extern const fb_ops_t fb_heap_ops;       /* fb_heap.c,  Student B */

/* Ordering used by both structures: lower write count first, then lower id. */
static inline int fb_less(uint16_t a, uint16_t b)
{
    uint32_t wa = nvm_wear(a);
    uint32_t wb = nvm_wear(b);

    return (wa < wb) || (wa == wb && a < b);
}

#endif /* FB_H */
