"""Reference model of the wear-levelling simulator, written independently of the C code.

It prints the same CSV rows as ./wearlevel so that the two can be compared exactly.
Usage: python3 tools/model.py TRACE.h METHOD SPARES [LIMIT]
"""
import heapq
import re
import sys


def load(path):
    text = open(path).read()
    n = int(re.search(r'NUM_LOGICAL_BLOCKS\s+(\d+)', text).group(1))
    writes = [(int(b), int(v)) for b, v in re.findall(r'\{(\d+),\s*(\d+)\}', text)]
    return n, writes


def run_baseline(n, writes, limit):
    wear = [0] * n
    total = 0
    while True:
        for block, _ in writes:
            wear[block] += 1
            total += 1
            if wear[block] >= limit:
                return total, wear, 0


def run_wl(n, writes, spares, limit, kind):
    phys = n + spares
    wear = [0] * phys
    mapping = list(range(n))
    store = {}
    shadow = {}
    total = 0
    cmp_total = 0
    if kind == 'array':
        pool = list(range(n, phys))
    else:
        pool = list(range(n, phys))     # ascending ids with equal wear is a valid heap
    while True:
        for block, value in writes:
            if kind == 'array':
                best = 0
                for i in range(1, len(pool)):
                    cmp_total += 1
                    if (wear[pool[i]], pool[i]) < (wear[pool[best]], pool[best]):
                        best = i
                fresh = pool[best]
                pool[best] = pool[-1]
                pool.pop()
                old = mapping[block]
                wear[fresh] += 1
                mapping[block] = fresh
                store[fresh] = value
                pool.append(old)
            else:
                fresh = pool[0]
                last = pool.pop()
                if pool:
                    pool[0] = last
                    i = 0
                    size = len(pool)
                    while True:
                        left, right, small = 2 * i + 1, 2 * i + 2, i
                        if left < size:
                            cmp_total += 1
                            if (wear[pool[left]], pool[left]) < (wear[pool[small]], pool[small]):
                                small = left
                        if right < size:
                            cmp_total += 1
                            if (wear[pool[right]], pool[right]) < (wear[pool[small]], pool[small]):
                                small = right
                        if small == i:
                            break
                        pool[i], pool[small] = pool[small], pool[i]
                        i = small
                old = mapping[block]
                wear[fresh] += 1
                mapping[block] = fresh
                store[fresh] = value
                pool.append(old)
                i = len(pool) - 1
                while i > 0:
                    parent = (i - 1) // 2
                    cmp_total += 1
                    if (wear[pool[i]], pool[i]) < (wear[pool[parent]], pool[parent]):
                        pool[i], pool[parent] = pool[parent], pool[i]
                        i = parent
                    else:
                        break
            shadow[block] = value
            total += 1
            assert store[mapping[block]] == value, 'read-back failed'
            if wear[fresh] >= limit:
                for b, v in shadow.items():
                    assert store[mapping[b]] == v, 'final sweep failed'
                assert len(set(mapping) | set(pool)) == phys and len(mapping) + len(pool) == phys
                return total, wear, cmp_total


def row(trace, n, spares, method, total, wear, cmp_total, pool_bytes):
    avg100 = sum(wear) * 100 // len(wear)
    cmp100 = cmp_total * 100 // total
    return (f'{trace},{n},{spares},{method},{total},{max(wear)},{avg100 // 100}.{avg100 % 100:02d},'
            f'{cmp100 // 100}.{cmp100 % 100:02d},{pool_bytes},ok')


def model_row(trace, method, spares, limit=10000):
    n, writes = load(trace)
    if method == 'baseline':
        total, wear, cmp_total = run_baseline(n, writes, limit)
        return row(trace, n, 0, 'baseline', total, wear, 0, 0)
    total, wear, cmp_total = run_wl(n, writes, spares, limit, method)
    return row(trace, n, spares, method, total, wear, cmp_total, 2 * spares)


if __name__ == '__main__':
    trace, method, spares = sys.argv[1], sys.argv[2], int(sys.argv[3])
    limit = int(sys.argv[4]) if len(sys.argv) > 4 else 10000
    print(model_row(trace, method, spares, limit))
