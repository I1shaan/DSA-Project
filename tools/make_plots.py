"""Make the Week 3 plots from results/results.csv.

Usage: python3 tools/make_plots.py
Writes four PNG files into results/.
"""
import csv
import os

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
os.chdir(ROOT)

INK = '#0b0b0b'
INK2 = '#52514e'
GRID = '#e4e3df'
SURFACE = '#fcfcfb'
GRAY = '#8a8985'
BLUE = '#2a78d6'       # categorical slot 1: min-heap
ORANGE = '#eb6834'     # categorical slot 2: array scan
SEQ = {1: '#9cc3ee', 4: '#4f93df', 16: '#1b4f95'}   # one hue, light to dark: more spares

plt.rcParams.update({
    'font.family': 'DejaVu Sans', 'font.size': 10, 'text.color': INK,
    'axes.edgecolor': GRID, 'axes.labelcolor': INK2, 'xtick.color': INK2, 'ytick.color': INK2,
    'axes.facecolor': SURFACE, 'figure.facecolor': SURFACE, 'savefig.facecolor': SURFACE,
    'axes.spines.top': False, 'axes.spines.right': False,
})

rows = list(csv.DictReader(open('results/results.csv')))
for r in rows:
    r['logical'] = int(r['logical']); r['hot_pct'] = int(r['hot_pct']); r['spares'] = int(r['spares'])
    r['lifetime'] = int(r['lifetime']); r['max_write'] = int(r['max_write'])
    r['avg_write'] = float(r['avg_write']); r['cmp_per_write'] = float(r['cmp_per_write'])
    r['pool_bytes'] = int(r['pool_bytes'])


def pick(**kw):
    return [r for r in rows if all(r[k] == v for k, v in kw.items())]


def base(trace):
    return pick(trace=trace, method='baseline')[0]


def style(ax):
    ax.grid(axis='y', color=GRID, lw=0.8)
    ax.set_axisbelow(True)


def kfmt(x, _):
    return f'{x / 1e6:g}M' if x >= 1e6 else (f'{x / 1e3:g}k' if x >= 1e3 else f'{x:g}')


# traces by sweep
BLOCK_TRACES = [('trace_blocks16.h', 16), ('trace.h', 64), ('trace_blocks256.h', 256), ('trace_blocks1024.h', 1024)]
HOT_TRACES = [('trace_hot50.h', 50), ('trace.h', 80), ('trace_hot95.h', 95)]

# ---- figure 1: lifetime against number of blocks
fig, ax = plt.subplots(figsize=(7.2, 4.3), dpi=200)
xs = [n for _, n in BLOCK_TRACES]
ax.plot(xs, [base(t)['lifetime'] for t, _ in BLOCK_TRACES], color=GRAY, lw=2, marker='o', ms=6, label='no wear-levelling')
for s in (1, 4, 16):
    ys = [pick(trace=t, method='heap', spares=s)[0]['lifetime'] for t, _ in BLOCK_TRACES]
    ax.plot(xs, ys, color=SEQ[s], lw=2, marker='o', ms=6, label=f'wear-levelling, {s} spare{"s" if s > 1 else ""}')
ax.set_xscale('log', base=2); ax.set_yscale('log')
ax.set_xticks(xs); ax.set_xticklabels([str(x) for x in xs])
ax.yaxis.set_major_formatter(FuncFormatter(kfmt))
ax.set_xlabel('logical blocks'); ax.set_ylabel('lifetime (writes until first block wears out)')
ax.set_title('Wear-levelling multiplies the lifetime at every memory size', loc='left', fontsize=11, color=INK)
style(ax); ax.legend(frameon=False, loc='upper left', fontsize=9)
fig.text(0.01, 0.01, 'Array scan and min-heap give identical lifetimes, so one line per spare count. 80% hot writes, limit 10,000.',
         fontsize=7.5, color=INK2)
fig.tight_layout(rect=(0, 0.03, 1, 1)); fig.savefig('results/fig1_blocks.png'); plt.close(fig)

# ---- figure 2: lifetime against hotness
fig, ax = plt.subplots(figsize=(7.2, 4.3), dpi=200)
xs = [h for _, h in HOT_TRACES]
ax.plot(xs, [base(t)['lifetime'] for t, _ in HOT_TRACES], color=GRAY, lw=2, marker='o', ms=6, label='no wear-levelling')
for s in (1, 4, 16):
    ys = [pick(trace=t, method='heap', spares=s)[0]['lifetime'] for t, _ in HOT_TRACES]
    ax.plot(xs, ys, color=SEQ[s], lw=2, marker='o', ms=6, label=f'wear-levelling, {s} spare{"s" if s > 1 else ""}')
ax.set_xticks(xs); ax.set_xticklabels([f'{x}%' for x in xs])
ax.set_ylim(0, None)
ax.yaxis.set_major_formatter(FuncFormatter(kfmt))
ax.set_xlabel('share of writes that go to the hot 20% of blocks'); ax.set_ylabel('lifetime (writes)')
ax.set_title('Hotter data nearly halves the baseline life; spares absorb most of it', loc='left', fontsize=11, color=INK)
style(ax); ax.legend(frameon=False, loc='center right', fontsize=9)
fig.text(0.01, 0.01, '64 logical blocks, limit 10,000. Array scan and min-heap give identical lifetimes.', fontsize=7.5, color=INK2)
fig.tight_layout(rect=(0, 0.03, 1, 1)); fig.savefig('results/fig2_hotness.png'); plt.close(fig)

# ---- figure 3: how even the wear is (average write count as a share of the busiest block)
fig, ax = plt.subplots(figsize=(7.2, 4.3), dpi=200)
labels = [f'{n} blocks' for _, n in BLOCK_TRACES]
series = [('no wear-levelling', GRAY, None)] + [(f'{s} spare{"s" if s > 1 else ""}', SEQ[s], s) for s in (1, 4, 16)]
w = 0.2
for k, (name, col, s) in enumerate(series):
    ys = []
    for t, _ in BLOCK_TRACES:
        r = base(t) if s is None else pick(trace=t, method='heap', spares=s)[0]
        ys.append(100.0 * r['avg_write'] / r['max_write'])
    xs = [i + (k - 1.5) * w for i in range(len(BLOCK_TRACES))]
    ax.bar(xs, ys, width=w * 0.88, color=col, label=name)
ax.set_xticks(range(len(BLOCK_TRACES))); ax.set_xticklabels(labels)
ax.set_ylim(0, 122); ax.set_yticks([0, 20, 40, 60, 80, 100]); ax.set_ylabel('average write count as % of the busiest block')
ax.set_title('Wear-levelling uses most of the memory\'s life', loc='left', fontsize=11, color=INK)
style(ax); ax.legend(frameon=False, loc='upper center', fontsize=9, ncol=4)
fig.text(0.01, 0.01, '100% means every block wore out together. The busiest block always ends at the 10,000 limit.', fontsize=7.5, color=INK2)
fig.tight_layout(rect=(0, 0.03, 1, 1)); fig.savefig('results/fig3_evenness.png'); plt.close(fig)

# ---- figure 4: work per write and memory against spare blocks (trace.h, with the extra 64 and 256)
fig, (a1, a2) = plt.subplots(1, 2, figsize=(9.2, 4.0), dpi=200, gridspec_kw={'width_ratios': [1.35, 1]})
sp = [1, 4, 16, 64, 256]
arr = [pick(trace='trace.h', method='array', spares=s)[0]['cmp_per_write'] for s in sp]
hp = [pick(trace='trace.h', method='heap', spares=s)[0]['cmp_per_write'] for s in sp]
a1.plot(sp, arr, color=ORANGE, lw=2, marker='o', ms=6, label='array scan')
a1.plot(sp, hp, color=BLUE, lw=2, marker='o', ms=6, label='min-heap')
a1.set_xscale('log', base=2); a1.set_xticks(sp); a1.set_xticklabels([str(s) for s in sp])
a1.set_xlabel('spare blocks'); a1.set_ylabel('comparisons per write')
a1.set_title('Work per write', loc='left', fontsize=11, color=INK)
a1.annotate(f'{arr[-1]:.0f}', (256, arr[-1]), textcoords='offset points', xytext=(-6, 6), ha='right', color=ORANGE, fontsize=9)
a1.annotate(f'{hp[-1]:.1f}', (256, hp[-1]), textcoords='offset points', xytext=(-6, 8), ha='right', color=BLUE, fontsize=9)
a1.annotate(f'at 4 spares the heap is slightly worse:\n{hp[1]:.2f} against {arr[1]:.2f}', (4, hp[1]), textcoords='offset points', xytext=(8, 34), ha='left', color=INK2, fontsize=8)
style(a1); a1.legend(frameon=False, loc='upper left', fontsize=9)
mem = [pick(trace='trace.h', method='heap', spares=s)[0]['pool_bytes'] for s in sp]
a2.plot(sp, mem, color=INK2, lw=2, marker='o', ms=6)
a2.set_xscale('log', base=2); a2.set_xticks(sp); a2.set_xticklabels([str(s) for s in sp])
a2.set_xlabel('spare blocks'); a2.set_ylabel('bytes in the free-block structure')
a2.set_title('Memory (same for both)', loc='left', fontsize=11, color=INK)
style(a2)
fig.text(0.01, 0.01, 'trace.h, 64 blocks. 64 and 256 spares are an extra experiment beyond the brief. Memory is 2 bytes per spare (one uint16 id).',
         fontsize=7.5, color=INK2)
fig.tight_layout(rect=(0, 0.03, 1, 1)); fig.savefig('results/fig4_work_memory.png'); plt.close(fig)

# ---- summary table in markdown
with open('results/summary.md', 'w') as fh:
    fh.write('| trace | blocks | hot % | spares | method | lifetime | gain vs baseline | avg write | cmp / write | bytes |\n')
    fh.write('|---|---|---|---|---|---|---|---|---|---|\n')
    for r in rows:
        b = base(r['trace'])['lifetime']
        gain = '1.0x' if r['method'] == 'baseline' else f"{r['lifetime'] / b:.1f}x"
        fh.write(f"| {r['trace']} | {r['logical']} | {r['hot_pct']} | {r['spares']} | {r['method']} | {r['lifetime']:,} | {gain} | "
                 f"{r['avg_write']:.2f} | {r['cmp_per_write']:.2f} | {r['pool_bytes']} |\n")
print('plots and summary written')
