"""Client MQTT (paho) : abonnement aux trames des noeuds, relais des alertes vers la supervision."""
from __future__ import annotations
import json
import logging
import os

import paho.mqtt.client as mqtt

from .handler import Handler
from .frame import Frame

log = logging.getLogger("gateway.mqtt")


class GatewayMqtt:
    def __init__(self, handler: Handler, host: str | None = None, port: int | None = None):
        self.handler = handler
        self.host = host or os.environ.get("MQTT_HOST", "localhost")
        self.port = port or int(os.environ.get("MQTT_PORT", "1883"))
        self.client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="gateway")
        user, pw = os.environ.get("MQTT_USER"), os.environ.get("MQTT_PASSWORD")   # jamais en dur (hotspot Sonar)
        if user:
            self.client.username_pw_set(user, pw)
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.handler.on_alert = self.relay_alert

    def _on_connect(self, client, userdata, flags, reason_code, properties=None):
        log.info("connecté au broker %s:%d (%s)", self.host, self.port, reason_code)
        client.subscribe("thermonode/+/data")

    def _on_message(self, client, userdata, msg):
        try:
            self.handler.on_data(msg.payload.decode("utf-8", errors="replace"))
        except Exception:                      # jamais de plantage sur une trame hostile
            log.exception("erreur de traitement")
            self.handler.metrics["errors"] += 1

    def relay_alert(self, f: Frame) -> None:
        self.client.publish("supervision/alerts", json.dumps({"id": f.node_id, "temp": f.temp, "t": f.t}))

    def run_forever(self) -> None:
        self.client.reconnect_delay_set(1, 10)
        self.client.connect(self.host, self.port, keepalive=30)
        self.client.loop_forever(retry_first_connection=True)
