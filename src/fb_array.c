/* fb_array.c  (Student A)
 * Free-block pool kept in a plain array. Finding the least-used block scans the
 * whole array, so it costs n - 1 comparisons for n free blocks, which is O(n).
 */
#include "fb.h"
#include "config.h"

static uint16_t pool[MAX_SPARES];
static uint16_t count;
static uint64_t cmp_count;

static void a_init(uint16_t first, uint16_t n)
{
    uint16_t i;

    count = (n > MAX_SPARES) ? (uint16_t)MAX_SPARES : n;
    for (i = 0; i < count; i++) {
        pool[i] = (uint16_t)(first + i);
    }
    cmp_count = 0;
}

static uint16_t a_take_min(void)
{
    uint16_t best = 0;
    uint16_t i;
    uint16_t id;

    if (count == 0) {
        return BLOCK_NONE;
    }
    for (i = 1; i < count; i++) {
        cmp_count++;
        if (fb_less(pool[i], pool[best])) {
            best = i;
        }
    }
    id = pool[best];
    pool[best] = pool[count - 1];   /* fill the hole with the last entry */
    count--;
    return id;
}

static void a_put(uint16_t id)
{
    if (count < MAX_SPARES) {
        pool[count] = id;
        count++;
    }
}

static uint16_t a_size(void)
{
    return count;
}

static uint16_t a_item(uint16_t i)
{
    return (i < count) ? pool[i] : BLOCK_NONE;
}

static uint64_t a_comparisons(void)
{
    return cmp_count;
}

static uint32_t a_bytes(void)
{
    return (uint32_t)count * (uint32_t)sizeof(pool[0]);
}

static int a_check(void)
{
    return count <= MAX_SPARES;     /* an unordered array has no other rule */
}

const fb_ops_t fb_array_ops = {
    "array", a_init, a_take_min, a_put, a_size, a_item, a_comparisons, a_bytes, a_check
};
