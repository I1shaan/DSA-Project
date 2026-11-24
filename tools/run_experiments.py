"""Week 3 experiments. Builds one program per trace, runs every configuration
and writes results/results.csv.

Required sweeps from the brief:
  number of blocks   16, 64, 256, 1024      (80% hot)
  spare blocks       1, 4, 16
  hotness            50%, 80%, 95%          (64 blocks)
Extra, only on trace.h: 64 and 256 spares, to show how the two methods scale.

Measured per run: lifetime, largest write count, average write count,
comparisons per write, memory used by the free-block structure.

Usage: python3 tools/run_experiments.py
"""
import csv
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

TRACES = [
    'more_traces/trace_blocks16.h',
    'trace.h',
    'more_traces/trace_blocks256.h',
    'more_traces/trace_blocks1024.h',
    'more_traces/trace_hot50.h',
    'more_traces/trace_hot95.h',
]
SPARES = [1, 4, 16]
EXTRA_SPARES = [64, 256]
EXTRA_ON = 'trace.h'
FIELDS = ['trace', 'logical', 'hot_pct', 'spares', 'method', 'lifetime', 'max_write',
          'avg_write', 'cmp_per_write', 'pool_bytes', 'checks']


def hot_percent(path):
    head = open(path).read(400)
    m = re.search(r'(\d+)% of writes', head)
    return int(m.group(1)) if m else 0


def build(trace):
    name = os.path.splitext(os.path.basename(trace))[0]
    out = f'bin/{name}'
    os.makedirs('bin', exist_ok=True)
    subprocess.run(['make', '-s', f'TRACE={trace}', f'OUT={out}'], check=True)
    return out


def run(binary, method, spares):
    res = subprocess.run([binary, '--method', method, '--spares', str(spares)],
                         capture_output=True, text=True)
    if res.returncode != 0:
        sys.exit(f'run failed: {binary} {method} {spares}\n{res.stdout}{res.stderr}')
    return res.stdout.strip().split(',')


def main():
    rows = []
    for trace in TRACES:
        binary = build(trace)
        hot = hot_percent(trace)
        configs = [('baseline', 0)]
        spares = SPARES + (EXTRA_SPARES if trace == EXTRA_ON else [])
        for s in spares:
            configs += [('array', s), ('heap', s)]
        for method, s in configs:
            f = run(binary, method, s)
            row = {'trace': os.path.basename(trace), 'logical': f[1], 'hot_pct': hot,
                   'spares': f[2], 'method': f[3], 'lifetime': f[4], 'max_write': f[5],
                   'avg_write': f[6], 'cmp_per_write': f[7], 'pool_bytes': f[8], 'checks': f[9]}
            if row['checks'] != 'ok':
                sys.exit(f'check failed: {row}')
            rows.append(row)
            print(row['trace'], row['method'], row['spares'], row['lifetime'], flush=True)
    # both methods must choose the same blocks, so the lifetimes must agree
    by = {}
    for r in rows:
        by[(r['trace'], r['spares'], r['method'])] = r
    for (t, s, m), r in by.items():
        if m == 'array':
            h = by[(t, s, 'heap')]
            assert r['lifetime'] == h['lifetime'] and r['avg_write'] == h['avg_write'], (t, s)
    os.makedirs('results', exist_ok=True)
    with open('results/results.csv', 'w', newline='') as fh:
        w = csv.DictWriter(fh, fieldnames=FIELDS)
        w.writeheader()
        w.writerows(rows)
    print(f'wrote results/results.csv with {len(rows)} rows')


if __name__ == '__main__':
    main()
