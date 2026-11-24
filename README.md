# Wear-levelling: baseline, array scan and min-heap

Project 2 of the data structures course (BITS Pilani Dubai). A simulator for
non-volatile memory (Flash or FRAM) in plain C. Every block can be written only
a limited number of times, so writes that keep hitting the same few blocks wear
those blocks out early. The simulator measures how long the memory lives when
writes are spread over all blocks instead, and compares two ways of finding the
least-used free block: an array scan and a min-heap.

The project covers Weeks 1 to 3: the simulator and baseline, both data
structures, the correctness checks, and the experiment sweeps with results and
plots.

## Repository layout

| Path | What it holds |
|---|---|
| `src/memory.c` | Simulated memory. One value and one write count per physical block. |
| `src/baseline.c` | No wear-levelling. Logical block i always uses physical block i. |
| `src/wl.c` | Wear-levelling. Takes the least-used free block, updates the mapping table, frees the old block. |
| `src/fb.h` | Interface of the free-block pool, shared by both data structures. |
| `src/fb_array.c` | Student A. Free blocks in an array, scanned on every write, O(n). |
| `src/fb_heap.c` | Student B. Free blocks in a binary min-heap, O(log n). |
| `src/sim.c` | Replays the trace until the first block hits the limit and checks every read. |
| `src/platform_pc.c` | All PC-only code: command line and printing. |
| `tests/` | Unit tests for both pools and the worked example from the brief. |
| `tools/model.py` | Independent Python model that must give identical results. |
| `tools/crosscheck.py`, `tools/lint.sh` | Row-by-row comparison with the model, and the coding-rule check. |
| `tools/run_experiments.py`, `tools/make_plots.py` | The Week 3 sweeps and the plots. |
| `results/` | `results.csv`, `summary.md` and four figures from the Week 3 runs. |
| `trace.h`, `more_traces/` | The input data from the course. Not edited. |

Both pools sit behind `fb_ops_t`, so each student commits only their own file
and the rest of the program does not change.

## Build and run

Needs a C11 compiler and make. The Python tools need Python 3 with matplotlib.

    make                      build ./wearlevel with trace.h
    make run                  one CSV row each for baseline, array and heap
    make test                 unit tests and a full checked run
    make sanitize             the same under AddressSanitizer and UBSan
    make lint                 check the coding rules
    make crosscheck           compare with the Python model, row by row

    ./wearlevel --header --method all --spares 16 --limit 10000
    make clean && make TRACE=more_traces/trace_hot95.h

Only one trace can be compiled into a program, so pick it with `TRACE=`.
Output columns: `trace, logical, spares, method, lifetime, max_write,
avg_write, cmp_per_write, pool_bytes, checks`. The last column says whether
every check passed.

## How it works

The memory has `NUM_LOGICAL_BLOCKS + spares` physical blocks. The mapping table
`map[logical] = physical` says where each logical block lives. On a write to
logical block `l` with value `v`:

1. take the least-used free block from the pool (lowest write count, ties to the lowest id)
2. write `v` into it, which adds one to its write count
3. set `map[l]` to that block
4. put the old block of `l` back into the pool

Lifetime is the number of writes done when the first block reaches the write
limit. The trace is replayed again and again until that happens. The same limit
(10,000 by default) is used for every method.

## Checks

Every write is read back and compared with the value just written. Every 4096
writes, and at the end, all logical blocks are read and compared with the last
value written to them, and the invariants are tested: each physical block is
either mapped or free, exactly once, and the pool layout is valid (heap rule
for the heap). Any failure makes the row say `FAIL` and the program exit with 1.

The tests were also run against deliberately broken versions (no mapping
update, block not returned to the pool, heap that does not swap, array that
picks the most-used block, heap without sift-up). All of them were caught.

## Week 3 experiments

    python3 tools/run_experiments.py     writes results/results.csv
    python3 tools/make_plots.py          writes results/fig1 to fig4 and results/summary.md

The sweep covers 16, 64, 256 and 1024 logical blocks, 1, 4 and 16 spares, and
50, 80 and 95 percent of writes going to the hot blocks. It also runs 64 and 256
spares on `trace.h` to show where the heap pulls ahead. All runs pass every
check, and array and heap give the same lifetime in every case.

### Lifetime against memory size (80% hot, 16 spares)

| Logical blocks | Baseline lifetime | With 16 spares | Gain |
|---|---|---|---|
| 16 | 36,686 | 319,941 | 8.7x |
| 64 | 155,728 | 798,979 | 5.1x |
| 256 | 558,233 | 2,701,271 | 4.8x |
| 1024 | 1,904,596 | 10,044,962 | 5.3x |

### Lifetime against hotness (64 blocks)

| Hot share of writes | Baseline lifetime | With 16 spares |
|---|---|---|
| 50% | 245,136 | 799,688 |
| 80% | 155,728 | 798,979 |
| 95% | 130,622 | 796,561 |

The baseline gets worse as writes concentrate on fewer blocks. With spares the
lifetime barely changes, because the writes are spread over all blocks anyway.

### Work per write on trace.h (64 logical blocks)

| Spares | Lifetime (both) | Array cmp/write | Heap cmp/write | Pool bytes |
|---|---|---|---|---|
| 1 | 586,281 | 0.00 | 0.00 | 2 |
| 4 | 676,561 | 3.00 | 3.94 | 8 |
| 16 | 798,979 | 15.00 | 9.29 | 32 |
| 64 | 1,279,732 | 63.00 | 13.04 | 128 |
| 256 | 3,199,664 | 255.00 | 15.77 | 512 |

Both methods pick the same blocks, so the lifetime is identical. The difference
is the work per write. The array grows in a straight line with the number of
spares, the heap grows by about one level each time the pool doubles. At 4
spares the heap needs slightly more comparisons than the array, so it only pays
off from about 16 spares upward. Memory is the same for both.

## Coding rules

Plain C11, `stdint.h` types, no `float`, no `malloc`, fixed-size arrays only
(sized by `MAX_LOGICAL` and `MAX_SPARES` in `src/config.h`). Printing and the
command line live in `src/platform_pc.c` only. `make lint` checks this.

The free-block structure stores one `uint16_t` id per spare, so its size is
`2 * spares` bytes. The write counts live in one shared array in the memory
model. This is the number reported in the `pool_bytes` column.