"""python -m gateway : démarre la passerelle (variables : MQTT_HOST, MQTT_PORT, DB_PATH, HTTP_PORT)."""
import logging
import os

from .handler import Handler
from .health import start_health_server
from .mqtt_client import GatewayMqtt

logging.basicConfig(level=os.environ.get("LOG_LEVEL", "INFO"), format="%(asctime)s %(levelname)s %(name)s: %(message)s")
handler = Handler(os.environ.get("DB_PATH", "gw.sqlite"))
start_health_server(handler, int(os.environ.get("HTTP_PORT", "8000")))
GatewayMqtt(handler).run_forever()
