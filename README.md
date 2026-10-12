# ThermoNode — projet fil rouge du module EET3S1.3-TVD

Nœud capteur de température IoT (firmware C99 pour STM32G031K8 + passerelle Python) servant de support
aux ateliers du module *Test, validation et DevOps pour les systèmes cyberphysiques*.

## Démarrage rapide (poste Linux, 5 minutes)

```bash
sudo apt install build-essential gcc-arm-none-eabi cppcheck python3-pip docker-compose-v2
pip install gcovr && pip install -r gateway/requirements.txt
make -C firmware test            # 75 tests unitaires Unity
make -C firmware coverage        # rapport dans reports/coverage-fw.html
make -C firmware static          # cppcheck -> reports/cppcheck.xml
make -C firmware size-check      # compilation croisée + porte mémoire (ENF-03)
cd gateway && pytest tests/unit  # 24 tests pytest
docker compose -f ci/compose.test.yaml up -d --build --wait   # broker + passerelle + simulateur
cd gateway && MQTT_HOST=localhost pytest tests/integration    # tests système SIL
```

## Arborescence
- `firmware/src` modules applicatifs (convert, filter, threshold, fault, ringbuf, frame, node) — `hal/` implémentations (cible, simulateur, stub) — `test/` doublures + tests Unity
- `gateway/` passerelle Python (frame, storage, handler, mqtt_client, health) + tests
- `tools/` traçabilité, porte mémoire, conversion cppcheck→Sonar, simulateur OTA
- `ci/` image de build, environnement d'intégration Compose, plateforme Jenkins/Sonar/Nexus
- `docs/` exigences, ICD, dérogations, plan de test, dossier de validation
- `Jenkinsfile`, `sonar-project.properties`, `deploy/staging.yaml`

Voir `docs/guide-construction.md` pour le matériel, le câblage et la construction pas à pas.
