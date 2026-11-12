#!/usr/bin/env python3
"""Real Windows integration tests. Not a substitute for interactive console testing."""
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys
import tempfile

if os.name != 'nt':
    raise SystemExit('Windows x64 required for this test')
compiler = Path(sys.argv[1]).resolve()

def run(args, *, cwd=None, env=None, expected=0):
    result = subprocess.run([str(x) for x in args], cwd=cwd, env=env, capture_output=True)
    if (result.returncode == 0) != (expected == 0):
        raise AssertionError(f'{args}: unexpected exit {result.returncode}\n{result.stderr!r}')
    return result

with tempfile.TemporaryDirectory(prefix='serseg win ') as temp:
    root = Path(temp) / 'مجلد تجربة with spaces'
    root.mkdir()
    work = root / 'project dir'
    work.mkdir()
    tempdir = root / 'temporary files'
    tempdir.mkdir()
    env = os.environ.copy()
    env['TEMP'] = env['TMP'] = str(tempdir)
    copied = root / 'compiler with spaces.exe'
    shutil.copy2(compiler, copied)
    source = work / 'ملف main.ser'
    helper = work / 'math.ser'
    source.write_bytes(b'\xef\xbb\xbfuse math\r\nshow "\xd9\x85\xd8\xb1\xd8\xad\xd8\xa8\xd8\xa7"\r\nshow square(7)\r\n')
    helper.write_bytes(b'\xef\xbb\xbftask square(n:int) -> int {\r\n give n*n\r\n}\r\n')
    run([copied, 'check', source], cwd=work, env=env)
    out = run([copied, 'run', source], cwd=work, env=env).stdout.replace(b'\r\n', b'\n')
    assert out == 'مرحبا\n49\n'.encode('utf-8'), out
    print('PASS Windows Unicode/spaced compiler, source, temp, BOM/CRLF and redirected UTF-8')
    builtins = work / 'text builtins.ser'
    builtins.write_text('let name = input("Name? ")\nshow "Hello " + name\nshow len(name)\nshow str(-42)\nshow str(1.25)\nshow str(true)\nshow to_int("-9223372036854775808")\n', encoding='utf-8')
    result = subprocess.run([str(copied), 'run', str(builtins)], cwd=work, env=env, input=b'Aya\n', capture_output=True)
    assert result.returncode == 0, result.stderr
    assert result.stdout.replace(b'\r\n', b'\n') == b'Name? Hello Aya\n3\n-42\n1.25\ntrue\n-9223372036854775808\n', result.stdout
    invalid = work / 'invalid number.ser'
    invalid.write_text('show to_int("9223372036854775808")\n', encoding='utf-8')
    bad = subprocess.run([str(copied), 'run', str(invalid)], cwd=work, env=env, capture_output=True)
    assert bad.returncode != 0 and b'to_int: integer out of range' in bad.stderr, bad.stderr
    print('PASS Windows text builtins, prompt, signed integer boundary and runtime error')
    advanced = work / 'advanced text.ser'
    advanced.write_text('show utf8_len("مرحبا 🌍")\nshow utf8_slice("مرحبا", 0, 2)\nshow contains(trim("  world  "), "or")\nshow to_float("1.25") + 1.0\nwhen is_int("12") { show "valid" } otherwise when is_float("x") { show "float" } otherwise { show "invalid" }\n', encoding='utf-8')
    out = run([copied, 'run', advanced], cwd=work, env=env).stdout.replace(b'\r\n', b'\n')
    assert out == '7\nمر\ntrue\n2.25\nvalid\n'.encode('utf-8'), out
    print('PASS Windows UTF-8, numeric validation, condition chains and text operations')
    output = work / 'برنامج hello'
    run([copied, 'build', source, '-o', output], cwd=work, env=env)
    binary = work / 'برنامج hello.exe'
    assert binary.is_file() and not output.exists()
    pe = binary.read_bytes()
    pe_offset = struct.unpack_from('<I', pe, 0x3c)[0]
    assert pe[pe_offset:pe_offset+4] == b'PE\0\0'
    opt = pe_offset + 24
    assert struct.unpack_from('<H', pe, opt)[0] == 0x20b
    assert struct.unpack_from('<Q', pe, opt + 72)[0] == 8 * 1024 * 1024
    print('PASS Windows automatic .exe extension and 8 MiB PE stack reserve')
    run([copied, 'build', source, '-o', output], cwd=work, env=env, expected=1)
    standalone_dir = root / 'different directory'
    standalone_dir.mkdir()
    moved = standalone_dir / 'run me.exe'
    shutil.copy2(binary, moved)
    limited_env = env.copy()
    limited_env['PATH'] = str(Path(os.environ['SystemRoot']) / 'System32')
    out = run([moved], cwd=standalone_dir, env=limited_env).stdout.replace(b'\r\n', b'\n')
    assert out == 'مرحبا\n49\n'.encode('utf-8'), out
    print('PASS generated Windows executable outside compiler directory with system-only PATH')
    tests = work / 'tests'
    tests.mkdir()
    (tests / 'passing.ser').write_text('assert 1 + 1 == 2\n', encoding='utf-8')
    result = run([copied, 'test'], cwd=work, env=env)
    assert b'1 passed; 0 failed' in result.stdout
    print('PASS Windows test runner starts a compiler in a Unicode path')
    recursion = work / 'recursion.ser'
    recursion.write_text('task deep(n:int) -> int {\n when n == 0 { give 0 }\n give deep(n - 1) + 1\n}\nshow deep(2500)\n', encoding='utf-8')
    assert run([copied, 'run', recursion], cwd=work, env=env).stdout.strip() == b'2500'
    print('PASS Windows recursion at depth 2500')

    if os.environ.get('GITHUB_ACTIONS') == 'true':
        repo = Path(__file__).resolve().parent.parent
        gcc = shutil.which('gcc.exe')
        assert gcc, 'gcc.exe not on PATH'
        ps = Path(os.environ['SystemRoot']) / 'System32' / 'WindowsPowerShell' / 'v1.0' / 'powershell.exe'
        run([ps, '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', repo / 'tests' / 'windows-install.ps1', '-InstallDir', root / 'install', '-GccBin', Path(gcc).parent], env=env)
        assert (root / 'install' / 'personal.txt').is_file()
        print('PASS Windows installer and safe uninstall (isolated CI user PATH)')
        vscode_env = env.copy()
        vscode_env['USERPROFILE'] = str(root / 'test-profile')
        script = repo / 'scripts' / 'install-vscode.ps1'
        run([ps, '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', script], env=vscode_env)
        ext = Path(vscode_env['USERPROFILE']) / '.vscode' / 'extensions' / 'serseg.serseg-language-0.1.0'
        assert (ext / 'package.json').is_file()
        run([ps, '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', script], env=vscode_env, expected=1)
        print('PASS VS Code files copied to isolated profile and existing directory protected')
