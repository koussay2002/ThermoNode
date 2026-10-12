#!/usr/bin/env python3
"""Convertit reports/cppcheck.xml (format XML v2) en rapport 'generic issue' SonarQube (JSON)."""
import sys, json
import xml.etree.ElementTree as ET
sev = {'error': 'CRITICAL', 'warning': 'MAJOR', 'style': 'MINOR', 'performance': 'MINOR', 'portability': 'MINOR', 'information': 'INFO'}
issues = []
for e in ET.parse(sys.argv[1]).getroot().iter('error'):
    loc = e.find('location')
    if loc is None: continue
    issues.append({'engineId': 'cppcheck', 'ruleId': e.get('id'), 'severity': sev.get(e.get('severity'), 'MINOR'),
                   'type': 'BUG' if e.get('severity') == 'error' else 'CODE_SMELL',
                   'primaryLocation': {'message': e.get('msg'), 'filePath': 'firmware/' + loc.get('file'),
                                       'textRange': {'startLine': int(loc.get('line') or 1)}}})
json.dump({'issues': issues}, sys.stdout, indent=1)
