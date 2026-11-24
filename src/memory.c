/* memory.c
 * Simulated non-volatile memory. See memory.h.
 */
#include "memory.h"
#include "config.h"

static uint16_t data[MAX_PHYSICAL];
static uint32_t wear[MAX_PHYSICAL];
static uint16_t num_phys;
static uint32_t write_limit;
static uint16_t first_dead;

void nvm_init(uint16_t n_phys, uint32_t limit)
{
    uint16_t i;

    num_phys = (n_phys > MAX_PHYSICAL) ? (uint16_t)MAX_PHYSICAL : n_phys;
    write_limit = limit;
    first_dead = BLOCK_NONE;
    for (i = 0; i < MAX_PHYSICAL; i++) {
        data[i] = 0;
        wear[i] = 0;
    }
}

int nvm_write(uint16_t p, uint16_t value)
{
    if (p >= num_phys) {
        return 0;               /* ignore an out-of-range write; callers check ranges */
    }
    data[p] = value;
    wear[p]++;
    if (wear[p] >= write_limit) {
        if (first_dead == BLOCK_NONE) {
            first_dead = p;
        }
        return 1;
    }
    return 0;
}

uint16_t nvm_read(uint16_t p)
{
    return (p < num_phys) ? data[p] : 0;
}

uint32_t nvm_wear(uint16_t p)
{
    return (p < num_phys) ? wear[p] : 0;
}

uint16_t nvm_num_phys(void)
{
    return num_phys;
}

uint16_t nvm_first_dead(void)
{
    return first_dead;
}
