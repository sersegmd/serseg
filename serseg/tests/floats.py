#!/usr/bin/env python3
"""Differential check of bounded IEEE-754 binary64 expression results."""
import os
import random
import struct
import subprocess
import sys
import tempfile

rng = random.Random(4815)
compiler = os.path.abspath(sys.argv[1])

def expr(depth):
    if depth == 0 or rng.random() < 0.3:
        value = rng.randint(-80, 80) / 8.0
        return f'({value})', value
    a, av = expr(depth - 1)
    b, bv = expr(depth - 1)
    op = rng.choice(['+', '-', '*', '/'])
    if op == '/' and bv == 0.0:
        op = '+'
    value = {'+': lambda: av + bv, '-': lambda: av - bv,
             '*': lambda: av * bv, '/': lambda: av / bv}[op]()
    return f'({a} {op} {b})', value

items = [expr(4) for _ in range(200)]
with tempfile.TemporaryDirectory(prefix='serseg-floats-') as directory:
    source = os.path.join(directory, 'floats.ser')
    with open(source, 'w', encoding='utf-8') as stream:
        for text, _ in items:
            stream.write('show ' + text + '\n')
    run = subprocess.run([compiler, 'run', source], capture_output=True, text=True)
    if run.returncode:
        print(run.stderr, file=sys.stderr)
        sys.exit(1)
    output = run.stdout.splitlines()
    if len(output) != len(items):
        sys.exit('incorrect number of outputs')
    for index, (line, (text, expected)) in enumerate(zip(output, items)):
        actual = float(line)
        if struct.pack('>d', actual) != struct.pack('>d', expected):
            sys.exit(f'mismatch {index}: {text}: {actual!r} != {expected!r}')
print('PASS 200 deterministic differential float expressions')
