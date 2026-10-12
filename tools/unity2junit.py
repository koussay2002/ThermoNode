#!/usr/bin/env python3
"""Convertit les journaux Unity (fichier:ligne:test:PASS|FAIL:msg) en un rapport JUnit XML."""
import sys, re, os
from xml.sax.saxutils import escape
suites = []
for path in sys.argv[1:]:
    name = os.path.basename(path).replace('.log', '')
    cases = []
    for line in open(path, encoding='utf-8', errors='replace'):
        line = re.sub(r'\x1b\[[0-9;]*m', '', line)
        m = re.match(r'^(.*?):(\d+):(\w+):(PASS|FAIL|IGNORE)(?::(.*))?$', line.strip())
        if m:
            cases.append((m.group(3), m.group(4).strip(), (m.group(5) or '').strip()))
    suites.append((name, cases))
out = ['<?xml version="1.0" encoding="UTF-8"?>', '<testsuites>']
for name, cases in suites:
    fails = sum(1 for c in cases if c[1] == 'FAIL')
    skipped = sum(1 for c in cases if c[1] == 'IGNORE')
    out.append(f'  <testsuite name="{name}" tests="{len(cases)}" failures="{fails}" errors="0" skipped="{skipped}">')
    for tname, status, msg in cases:
        out.append(f'    <testcase classname="{name}" name="{tname}">')
        if status == 'FAIL':
            out.append(f'      <failure message="{escape(msg, {chr(34): "&quot;"})}"/>')
        elif status == 'IGNORE':
            out.append('      <skipped/>')
        out.append('    </testcase>')
    out.append('  </testsuite>')
out.append('</testsuites>')
print('\n'.join(out))
