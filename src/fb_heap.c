/* fb_heap.c  (Student B)
 * Free-block pool kept in a binary min-heap stored in an array.
 * The block at index 0 is always the least used one. Index i has its children
 * at 2i + 1 and 2i + 2 and its parent at (i - 1) / 2.
 * Removing the top and inserting a block both walk one path of the tree, so
 * they cost O(log n).
 *
 * The heap stores only block ids. The key is the write count of the block,
 * read from the memory. A block in the pool is never written, so its key does
 * not change while it sits in the heap and the heap stays valid.
 */
#include "fb.h"
#include "config.h"

static uint16_t heap[MAX_SPARES];
static uint16_t count;
static uint64_t cmp_count;

static void swap_items(uint16_t i, uint16_t j)
{
    uint16_t tmp = heap[i];

    heap[i] = heap[j];
    heap[j] = tmp;
}

/* Move the entry at index i up until its parent is not larger. */
static void sift_up(uint16_t i)
{
    uint16_t parent;

    while (i > 0) {
        parent = (uint16_t)((i - 1) / 2);
        cmp_count++;
        if (fb_less(heap[i], heap[parent])) {
            swap_items(i, parent);
            i = parent;
        } else {
            break;
        }
    }
}

/* Move the entry at index i down until it is not larger than its children. */
static void sift_down(uint16_t i)
{
    uint16_t left;
    uint16_t right;
    uint16_t smallest;

    for (;;) {
        left = (uint16_t)(2 * i + 1);
        right = (uint16_t)(2 * i + 2);
        smallest = i;
        if (left < count) {
            cmp_count++;
            if (fb_less(heap[left], heap[smallest])) {
                smallest = left;
            }
        }
        if (right < count) {
            cmp_count++;
            if (fb_less(heap[right], heap[smallest])) {
                smallest = right;
            }
        }
        if (smallest == i) {
            break;
        }
        swap_items(i, smallest);
        i = smallest;
    }
}

static void h_init(uint16_t first, uint16_t n)
{
    uint16_t i;

    count = (n > MAX_SPARES) ? (uint16_t)MAX_SPARES : n;
    for (i = 0; i < count; i++) {
        heap[i] = (uint16_t)(first + i);
    }
    /* Build the heap bottom-up. Fresh blocks are in id order, so nothing moves. */
    for (i = (uint16_t)(count / 2); i > 0; i--) {
        sift_down((uint16_t)(i - 1));
    }
    cmp_count = 0;
}

static uint16_t h_take_min(void)
{
    uint16_t top;

    if (count == 0) {
        return BLOCK_NONE;
    }
    top = heap[0];
    count--;
    if (count > 0) {
        heap[0] = heap[count];      /* last entry takes the root, then sinks */
        sift_down(0);
    }
    return top;
}

static void h_put(uint16_t id)
{
    if (count < MAX_SPARES) {
        heap[count] = id;
        count++;
        sift_up((uint16_t)(count - 1));
    }
}

static uint16_t h_size(void)
{
    return count;
}

static uint16_t h_item(uint16_t i)
{
    return (i < count) ? heap[i] : BLOCK_NONE;
}

static uint64_t h_comparisons(void)
{
    return cmp_count;
}

static uint32_t h_bytes(void)
{
    return (uint32_t)count * (uint32_t)sizeof(heap[0]);
}

/* Heap rule: no entry is smaller than its parent. */
static int h_check(void)
{
    uint16_t i;

    if (count > MAX_SPARES) {
        return 0;
    }
    for (i = 1; i < count; i++) {
        if (fb_less(heap[i], heap[(i - 1) / 2])) {
            return 0;
        }
    }
    return 1;
}

const fb_ops_t fb_heap_ops = {
    "heap", h_init, h_take_min, h_put, h_size, h_item, h_comparisons, h_bytes, h_check
};
