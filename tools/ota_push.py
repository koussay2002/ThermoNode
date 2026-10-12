#!/usr/bin/env python3
"""Simulateur de mise à jour OTA d'une flotte pilote : télécharge l'artefact depuis Nexus, vérifie l'empreinte
SHA-256, 'installe' sur N noeuds simulés (retour arrière si le firmware dépasse 48 Ko) et journalise dans deploy/log/.
Usage : ota_push.py --fleet pilot --version v1.0.0 --from http://nexus:8081/repository/thermonode-raw [--nodes 5]"""
import argparse, hashlib, json, os, sys, time, urllib.request, getpass, base64
ap = argparse.ArgumentParser(); ap.add_argument('--fleet', required=True); ap.add_argument('--version', required=True)
ap.add_argument('--from', dest='base', required=True); ap.add_argument('--nodes', type=int, default=5)
ap.add_argument('--user'); ap.add_argument('--password')
a = ap.parse_args()
name = f'thermonode-fw-{a.version}.bin'
def fetch(path):
    req = urllib.request.Request(f'{a.base}/{a.version}/{path}')
    if a.user: req.add_header('Authorization', 'Basic ' + base64.b64encode(f'{a.user}:{a.password}'.encode()).decode())
    return urllib.request.urlopen(req, timeout=30).read()
fw = fetch(name); sums = fetch('SHA256SUMS').decode()
expected = next((l.split()[0] for l in sums.splitlines() if l.strip().endswith(name)), None)
actual = hashlib.sha256(fw).hexdigest()
if expected != actual:
    print(f'EMPREINTE INVALIDE : attendu {expected}, obtenu {actual}'); sys.exit(1)
log = {'fleet': a.fleet, 'version': a.version, 'sha256': actual, 'date': time.strftime('%Y-%m-%dT%H:%M:%SZ', time.gmtime()),
       'approver': os.environ.get('BUILD_USER', getpass.getuser()), 'nodes': []}
for i in range(1, a.nodes + 1):
    ok = (len(fw) < 49152)
    log['nodes'].append({'id': f'TN{i:02d}', 'result': 'ok' if ok else 'rollback'})
os.makedirs('deploy/log', exist_ok=True)
path = f'deploy/log/{a.fleet}-{a.version}.json'
json.dump(log, open(path, 'w'), indent=1)
print(f'{sum(1 for n in log["nodes"] if n["result"]=="ok")}/{a.nodes} noeuds mis à jour ; journal : {path}')
sys.exit(0 if all(n['result'] == 'ok' for n in log['nodes']) else 1)
