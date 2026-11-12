#!/usr/bin/env python3
import os
from pathlib import Path
import random
import subprocess
import sys
import tempfile

compiler = Path(sys.argv[1]).resolve()
with tempfile.TemporaryDirectory(prefix='serseg-utf8-') as directory:
    root = Path(directory)
    source = root / 'main.ser'
    exe = root / ('program.exe' if os.name == 'nt' else 'program')
    source.write_text('let value = input()\nshow utf8_len(value)\n', encoding='utf-8')
    subprocess.run([str(compiler), 'build', str(source), '-o', str(exe)], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    def invoke(data):
        return subprocess.run([str(exe)], input=data + b'\n', stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    rng = random.Random(8192)
    alphabet = ['a', 'é', 'م', '中', '🌍', '\u0301', ' ']
    for _ in range(100):
        text = ''.join(rng.choice(alphabet) for _ in range(rng.randrange(32)))
        result = invoke(text.encode('utf-8'))
        assert result.returncode == 0 and int(result.stdout.strip()) == len(text), (text, result.stderr)
    for invalid in (b'\x80', b'\xc0\x80', b'\xc2', b'\xe0\x80\x80', b'\xed\xa0\x80', b'\xf0\x80\x80\x80', b'\xf4\x90\x80\x80', b'\xf5\x80\x80\x80', b'A\xe2\x82'):
        result = invoke(invalid)
        assert result.returncode != 0 and b'utf8_len: invalid UTF-8' in result.stderr, (invalid, result.stderr)
    print('PASS 100 UTF-8 codepoint differential cases and 9 invalid encodings')
    source.write_text('let value = input()\nshow utf8_slice(value, 1, 3)\n', encoding='utf-8')
    exe.unlink()
    subprocess.run([str(compiler), 'build', str(source), '-o', str(exe)], check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    result = invoke('Aم🌍z'.encode('utf-8'))
    assert result.returncode == 0 and result.stdout.replace(b'\r\n', b'\n') == 'م🌍\n'.encode('utf-8'), result.stderr
    for _ in range(100):
        text = ''.join(rng.choice(alphabet) for _ in range(rng.randrange(3, 24)))
        result = invoke(text.encode('utf-8'))
        assert result.returncode == 0 and result.stdout.replace(b'\r\n', b'\n') == (text[1:3] + '\n').encode('utf-8'), (text, result.stderr)
    result = invoke(b'A')
    assert result.returncode != 0 and b'utf8_slice: range out of bounds' in result.stderr
    result = invoke(b'Abc\xff')
    assert result.returncode != 0 and b'utf8_slice: invalid UTF-8' in result.stderr
    print('PASS 100 UTF-8 slice differential cases, bounds and full-text validation')
