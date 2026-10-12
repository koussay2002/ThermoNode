"""Fixtures d'intégration : pilotent l'environnement ci/compose.test.yaml (broker, gateway, node-sim).
Exécution : docker compose -f ci/compose.test.yaml up -d --build --wait ; puis pytest tests/integration."""
import json, os, subprocess, threading, time
from pathlib import Path
import pytest
import paho.mqtt.client as mqtt

ROOT = Path(__file__).resolve().parents[3]
COMPOSE = ["docker", "compose", "-f", str(ROOT / "ci" / "compose.test.yaml")]
SCENARIO = ROOT / "tests" / "scenarios" / "current.csv"
MQTT_HOST = os.environ.get("MQTT_HOST", "localhost")
GATEWAY_URL = os.environ.get("GATEWAY_URL", "http://localhost:8000")


class Broker:
    def stop(self):  subprocess.run(COMPOSE + ["stop", "broker"], check=True)
    def start(self):
        subprocess.run(COMPOSE + ["start", "broker"], check=True)
        time.sleep(1)


class Node:
    """Écrit dans le scénario lu en continu par node-sim (une ligne = une mesure)."""
    def __init__(self, listener):
        self.listener = listener
    def inject_measures(self, temps):
        with SCENARIO.open("a") as f:
            f.write("".join(f"{t}\n" for t in temps))
    def inject_command(self, obj):
        with SCENARIO.open("a") as f:
            f.write("CMD:" + json.dumps(obj, separators=(",", ":")) + "\n")
    def wait_until_buffered(self, n, timeout_s=60):
        t0 = time.time()
        while time.time() - t0 < timeout_s:
            if self.listener.buffered >= n:
                return
            time.sleep(0.2)
        raise AssertionError(f"tampon {self.listener.buffered} < {n} après {timeout_s} s")


class Listener:
    """Abonné MQTT : collecte les trames data et le diagnostic du simulateur."""
    def __init__(self):
        self.frames, self.acks, self.buffered = [], [], 0
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="it-listener")
        self.client.on_message = self._on
        self.client.connect(MQTT_HOST, 1883)
        self.client.subscribe([("thermonode/+/data", 0), ("thermonode/+/diag", 0), ("thermonode/+/ack", 0)])
        self.client.loop_start()
    def _on(self, c, u, m):
        p = m.payload.decode(errors="replace")
        if m.topic.endswith("/data"):
            try: self.frames.append(json.loads(p))
            except ValueError: pass
        elif m.topic.endswith("/diag"):
            self.buffered = json.loads(p).get("buffered", 0)
        else:
            self.acks.append(p)
    def wait_for_frames(self, min_count, timeout_s):
        t0 = time.time()
        while time.time() - t0 < timeout_s:
            if len(self.frames) >= min_count:
                return list(self.frames)
            time.sleep(0.2)
        raise AssertionError(f"{len(self.frames)} trames < {min_count} après {timeout_s} s")
    def close(self):
        self.client.loop_stop(); self.client.disconnect()


@pytest.fixture(scope="session")
def env():
    SCENARIO.write_text("# scénario courant (écrit par les tests)\n25.0\n")
    subprocess.run(COMPOSE + ["up", "-d", "--build", "--wait"], check=True)
    yield
    subprocess.run(COMPOSE + ["down", "-v"], check=False)


@pytest.fixture
def gateway(env):
    l = Listener(); yield l; l.close()

@pytest.fixture
def broker(env):
    return Broker()

@pytest.fixture
def node(env, gateway):
    return Node(gateway)
