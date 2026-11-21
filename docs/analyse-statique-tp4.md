# Analyse statique TP4 — pylint (gateway)

## Etat initial (avant corrections)
- Commande : pylint gateway (depuis ~/tn/gateway, venv actif)
- Note : 9.05/10
- Messages : 18

| Code | Nom | Nb |
|---|---|---|
| W0613 | unused-argument | 5 |
| C0115 | missing-class-docstring | 5 |
| R0917 | too-many-positional-arguments | 2 |
| R0913 | too-many-arguments | 2 |
| W0511 | fixme | 1 |
| C0321 | multiple-statements | 1 |
| C0207 | use-maxsplit-arg | 1 |
| C0103 | invalid-name | 1 |

## Corrections (7 commits)
1. Docstrings de classes (C0115)
2. Sépare instructions multiples (C0321)
3. Maxsplit explicite (C0207)
4. Arguments inutilisés préfixés _ (W0613)
5. Supprime invalid-name justifié (C0103)
6. Retire TODO obsolète (W0511)
7. Supprime too-many-arguments justifié (R0913/R0917)

## Etat final
- Note : 10.00/10
- Messages : 0

## Tableau avant / apres
| Metrique | Avant | Apres |
|---|---|---|
| Note pylint | 9.05/10 | 10.00/10 |
| Messages | 18 | 0 |

## Suppressions justifiees
| Regle | Emplacement | Justification |
|---|---|---|
| C0103 | health.py do_GET | nom impose par BaseHTTPRequestHandler |
| R0913 / R0917 | storage.py insert() | signature imposee par le schema measures |
| R0913 / R0917 | mqtt_client.py _on_connect() | callback impose par paho-mqtt |
