#!/usr/bin/env python3
import json
from pathlib import Path
import re
root = Path(__file__).resolve().parent.parent / 'editors' / 'vscode'
manifest = json.loads((root / 'package.json').read_text(encoding='utf-8'))
language = manifest['contributes']['languages'][0]
assert language['id'] == 'serseg' and '.ser' in language['extensions']
config = json.loads((root / language['configuration']).read_text(encoding='utf-8'))
assert config['comments']['lineComment'] == '#'
grammar_info = manifest['contributes']['grammars'][0]
grammar = json.loads((root / grammar_info['path']).read_text(encoding='utf-8'))
assert grammar['scopeName'] == grammar_info['scopeName']
for pattern in grammar['patterns']:
    for key in ('match', 'begin', 'end'):
        if key in pattern:
            re.compile(pattern[key])
    for nested in pattern.get('patterns', []):
        re.compile(nested['match'])
snippets = json.loads((root / manifest['contributes']['snippets'][0]['path']).read_text(encoding='utf-8'))
assert {'Show', 'Task', 'Condition', 'Range loop'} <= set(snippets)
assert (root / manifest['main']).is_file()
assert {'serseg.build', 'serseg.run'} <= {c['command'] for c in manifest['contributes']['commands']}
print('PASS VS Code extension manifest, configuration, grammar and snippets validate')
