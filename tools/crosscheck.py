"""Run ./wearlevel and the Python reference model on the same configurations
and require identical output, character for character.

Usage: python3 tools/crosscheck.py [TRACE.h] [SPARES such as 1,4,16] [BINARY]
"""
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
import model

trace = sys.argv[1] if len(sys.argv) > 1 else 'trace.h'
spare_list = [int(x) for x in sys.argv[2].split(',')] if len(sys.argv) > 2 else [1, 4, 16, 64, 256]
binary = sys.argv[3] if len(sys.argv) > 3 else './wearlevel'
bad = 0
count = 0
for method in ['baseline', 'array', 'heap']:
    for spares in ([0] if method == 'baseline' else spare_list):
        out = subprocess.run([binary, '--method', method, '--spares', str(spares)],
                             capture_output=True, text=True, check=True).stdout.strip()
        want = model.model_row(trace, method, spares)
        count += 1
        if out != want:
            bad += 1
            print('MISMATCH')
            print('  C    :', out)
            print('  model:', want)
print(f'crosscheck {trace}: {count - bad}/{count} rows identical')
sys.exit(1 if bad else 0)
