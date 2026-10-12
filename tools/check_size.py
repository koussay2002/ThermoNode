#!/usr/bin/env python3
"""Porte qualité ENF-03 : vérifie flash (text+data) et RAM (data+bss) à partir de la sortie de `size` (Berkeley).
Usage : check_size.py reports/size.txt --flash-max 49152 --ram-max 8192 [--margin 0.10]
Code de retour 1 si un budget est dépassé (le pipeline échoue)."""
import argparse, sys, re
ap = argparse.ArgumentParser()
ap.add_argument('size_file'); ap.add_argument('--flash-max', type=int, required=True)
ap.add_argument('--ram-max', type=int, required=True); ap.add_argument('--margin', type=float, default=0.0)
a = ap.parse_args()
rows = [l.split() for l in open(a.size_file) if re.match(r'^\s*\d+\s+\d+\s+\d+', l)]
if not rows:
    print('check_size: sortie de size illisible'); sys.exit(2)
text, data, bss = (int(x) for x in rows[-1][:3])
flash, ram = text + data, data + bss
ok = True
for name, val, mx in (('flash', flash, a.flash_max), ('RAM', ram, a.ram_max)):
    pct = 100.0 * val / mx
    status = 'OK'
    if val > mx: status, ok = 'DEPASSEMENT', False
    elif a.margin and val > mx * (1 - a.margin): status = 'ALERTE marge'
    print(f'{name:5s}: {val:6d} / {mx:6d} octets ({pct:5.1f} %) {status}')
sys.exit(0 if ok else 1)
