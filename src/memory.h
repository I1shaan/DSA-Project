/* memory.h
 * Simulated non-volatile memory. Every physical block stores one 16-bit value
 * and keeps a write count. A block is worn out once its count reaches the
 * write limit.
 */
#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>

/* Reset the memory to n_phys blocks, all zero, with the given write limit. */
void nvm_init(uint16_t n_phys, uint32_t limit);

/* Store a value and add one to the write count of block p.
 * Returns 1 when this write brought the block to the write limit, else 0. */
int nvm_write(uint16_t p, uint16_t value);

uint16_t nvm_read(uint16_t p);
uint32_t nvm_wear(uint16_t p);      /* write count of block p */
uint16_t nvm_num_phys(void);
uint16_t nvm_first_dead(void);      /* first block to reach the limit, or BLOCK_NONE */

#endif /* MEMORY_H */
