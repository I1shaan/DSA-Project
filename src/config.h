/* config.h
 * Compile-time limits for the fixed-size arrays. No malloc is used anywhere,
 * so every array is sized for the largest experiment in the project brief.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define MAX_LOGICAL   1024u   /* largest trace: 1024 logical blocks */
#define MAX_SPARES    256u    /* largest spare count we sweep */
#define MAX_PHYSICAL  (MAX_LOGICAL + MAX_SPARES)

#define DEFAULT_WRITE_LIMIT 10000u  /* same value is used for every method */

#define BLOCK_NONE 0xFFFFu          /* returned when no block is available */

#endif /* CONFIG_H */
