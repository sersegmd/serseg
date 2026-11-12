#!/usr/bin/env python3
"""Differential test: generated, bounded integer expressions versus Python integers."""
import os, random, subprocess, sys, tempfile
rng = random.Random(1729)
compiler = os.path.abspath(sys.argv[1])
def expr(depth):
    if depth == 0 or rng.random() < .3:
        n = rng.randint(0, 40)
        return str(n), n
    a, av = expr(depth - 1)
    b, bv = expr(depth - 1)
    op = rng.choice(['+', '-', '*', '<', '>=', '=='])
    if op == '+': return f'({a} + {b})', av + bv
    if op == '-': return f'({a} - {b})', av - bv
    if op == '*': return f'({a} * {b})', av * bv
    # Comparison results cannot be used as integer operands. Convert via branch statements instead.
    return f'({a} - {b})', av - bv
items = [expr(4) for _ in range(200)]
with tempfile.TemporaryDirectory(prefix='serseg-differential-') as d:
    source = os.path.join(d, 'expressions.ser')
    with open(source, 'w', encoding='utf-8') as f:
        for text, _ in items: f.write('show ' + text + '\n')
    run = subprocess.run([compiler, 'run', source] + sys.argv[2:], capture_output=True, text=True)
    if run.returncode:
        print(run.stderr, file=sys.stderr); sys.exit(1)
    actual = run.stdout.splitlines()
    expected = [str(value) for _, value in items]
    if actual != expected:
        for i, (a, b) in enumerate(zip(actual, expected)):
            if a != b: print(f'mismatch {i}: {items[i][0]}: {a} != {b}', file=sys.stderr); break
        sys.exit(1)
print('PASS 200 deterministic differential integer expressions ' + (' '.join(sys.argv[2:]) or '-O0'))
