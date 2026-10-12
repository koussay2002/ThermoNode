"""Tests de fumée après déploiement sur staging : GATEWAY_URL et MQTT_HOST fournis par l'environnement."""
import os, time, urllib.request
import pytest
import paho.mqtt.publish as publish
from gateway.frame import crc16_ccitt_false

URL = os.environ.get("GATEWAY_URL", "http://localhost:8000")
HOST = os.environ.get("MQTT_HOST", "localhost")


def get(path):
    return urllib.request.urlopen(URL + path, timeout=5).read().decode()


def metric(name):
    for line in get("/metrics").splitlines():
        if line.startswith("gateway_" + name + " "):
            return int(line.split()[1])
    return 0


@pytest.mark.timeout(30)
def test_smoke_health():
    assert get("/health") == "ok"


@pytest.mark.timeout(30)
def test_smoke_frame_is_accepted():
    before = metric("frames_stored")
    payload = '{"id":"SMOKE","seq":%d,"t":%d,"temp":21.5,"state":"NORMAL"}' % (int(time.time()), int(time.time()))
    payload = payload[:-1] + ',"crc":"%04X"}' % crc16_ccitt_false(payload.encode())
    publish.single("thermonode/SMOKE/data", payload, hostname=HOST)
    for _ in range(50):
        if metric("frames_stored") > before:
            return
        time.sleep(0.1)
    pytest.fail("trame non stockée en 5 s")
