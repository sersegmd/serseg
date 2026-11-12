#!/usr/bin/env python3
"""Compare SERSEG optimization levels on the same checked iteration workload."""
import argparse
import hashlib
import json
import os
import platform
import statistics
import subprocess
import tempfile
import time
from datetime import datetime, timezone
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('compiler', help='path to SERSEG compiler')
parser.add_argument('--samples', type=int, default=7)
parser.add_argument('--output', help='new JSON results file (will not overwrite)')
args = parser.parse_args()
if args.samples < 1 or args.samples > 100:
    parser.error('--samples must be between 1 and 100')
compiler = str(Path(args.compiler).resolve())
source = Path(__file__).with_name('iteration.ser').resolve()
expected = '14985000000'
cpu = platform.processor() or 'unknown'
try:
    for line in Path('/proc/cpuinfo').read_text().splitlines():
        if line.startswith('model name'):
            cpu = line.split(':', 1)[1].strip()
            break
except OSError:
    pass
result = {
    'timestamp_utc': datetime.now(timezone.utc).isoformat(),
    'workload': '30,000,000 int_array visits with checked each loop',
    'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
    'compiler_sha256': hashlib.sha256(Path(compiler).read_bytes()).hexdigest(),
    'compiler_version': subprocess.check_output([compiler, '--version'], text=True).strip(),
    'gcc_version': subprocess.check_output(['gcc', '--version'], text=True).splitlines()[0],
    'system': platform.platform(), 'machine': platform.machine(), 'cpu': cpu,
    'samples_per_level': args.samples,
    'method': 'one build per level; one untimed warmup; alternate first level per repetition; elapsed wall time includes process startup',
    'results': {},
}
with tempfile.TemporaryDirectory(prefix='serseg-benchmark-') as d:
    binaries = {}
    for flag in ('-O0', '-O1'):
        exe = str(Path(d) / ('program' + flag + ('.exe' if os.name == 'nt' else '')))
        start = time.perf_counter()
        subprocess.run([compiler, 'build', str(source), flag, '-o', exe], check=True, capture_output=True)
        build_seconds = time.perf_counter() - start
        binaries[flag] = exe
        result['results'][flag] = {'build_seconds_single_run': build_seconds, 'binary_size_bytes': Path(exe).stat().st_size, 'execution_seconds': []}
    def run(exe):
        start = time.perf_counter()
        p = subprocess.run([exe], check=True, capture_output=True, text=True)
        elapsed = time.perf_counter() - start
        if p.stdout.strip() != expected:
            raise RuntimeError(f'incorrect workload output: {p.stdout!r}')
        return elapsed
    for flag in ('-O0', '-O1'):
        run(binaries[flag])
    for i in range(args.samples):
        order = ('-O0', '-O1') if i % 2 == 0 else ('-O1', '-O0')
        for flag in order:
            result['results'][flag]['execution_seconds'].append(run(binaries[flag]))
    for value in result['results'].values():
        value['execution_median_seconds'] = statistics.median(value['execution_seconds'])
text = json.dumps(result, indent=2) + '\n'
if args.output:
    with open(args.output, 'x', encoding='utf8') as f:
        f.write(text)
else:
    print(text, end='')
