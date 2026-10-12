#!/usr/bin/env python3
"""Génère la matrice de traçabilité exigences <-> tests à partir de docs/requirements.md et des rapports JUnit.
Les tests sont rattachés aux exigences par leur nom : test_EF04_..., test_ENF05_..., test_EI02_...
Usage : traceability.py docs/requirements.md reports/junit-*.xml [--fail-on-uncovered P1] > reports/traceability.md"""
import sys, re, glob, argparse
import xml.etree.ElementTree as ET
ap = argparse.ArgumentParser()
ap.add_argument('requirements'); ap.add_argument('junit', nargs='+')
ap.add_argument('--fail-on-uncovered', metavar='PRIO')
ap.add_argument('--analysis', default='docs/analysis-coverage.txt', help='exigences couvertes par analyse/revue/mesure')
a = ap.parse_args()
reqs = {}
for line in open(a.requirements, encoding='utf-8'):
    m = re.match(r'^\|\s*((?:EF|ENF|EI|C)-\d+)\s*\|\s*(P[123])\s*\|\s*(.*?)\s*\|', line)
    if m: reqs[m.group(1)] = (m.group(2), m.group(3))
tests = {}
analysis = {}
try:
    for line in open(a.analysis, encoding='utf-8'):
        if line.strip() and not line.startswith('#'):
            rid, mean, proof = (x.strip() for x in line.split('|', 2))
            analysis[rid] = (mean, proof)
except FileNotFoundError:
    pass
files = [f for pat in a.junit for f in glob.glob(pat)]
for f in files:
    for tc in ET.parse(f).getroot().iter('testcase'):
        name = tc.get('name', '')
        status = 'échec' if tc.find('failure') is not None or tc.find('error') is not None else ('ignoré' if tc.find('skipped') is not None else 'réussi')
        for m in re.finditer(r'(EF|ENF|EI|C)(\d+)', name):
            rid = f'{m.group(1)}-{int(m.group(2)):02d}'
            tests.setdefault(rid, []).append((name, status))
print('| Exigence | Prio | Titre | Tests | Réussis | Échoués | Statut |\n|---|---|---|---|---|---|---|')
uncovered = []
for rid, (prio, title) in sorted(reqs.items()):
    t = tests.get(rid, [])
    ok = sum(1 for _, s in t if s == 'réussi'); ko = sum(1 for _, s in t if s == 'échec')
    if not t and rid in analysis: status = f'couvert par {analysis[rid][0]}'
    elif not t: status = 'ORPHELIN'; uncovered.append((rid, prio))
    elif ko: status = 'ÉCHEC'
    else: status = 'validé'
    print(f'| {rid} | {prio} | {title} | {len(t)} | {ok} | {ko} | {status} |')
    if not t and rid in analysis: pass
orphan_tests = sorted(set(n for rid, lst in tests.items() if rid not in reqs for n, _ in lst))
n = len(reqs); cov = n - len(uncovered)
print(f'\nExigences : {n} ; couvertes : {cov} ({100*cov/n:.0f} %) ; orphelines : {", ".join(r for r, _ in uncovered) or "aucune"}')
if orphan_tests: print(f'Tests sans exigence connue : {", ".join(orphan_tests)}')
if a.fail_on_uncovered and any(p == a.fail_on_uncovered for _, p in uncovered):
    print(f'ERREUR : exigences {a.fail_on_uncovered} sans test', file=sys.stderr); sys.exit(1)
