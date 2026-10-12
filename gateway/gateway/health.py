"""Serveur HTTP minimal : /health (200 si vivant) et /metrics (format Prometheus) — utilisé par les tests de fumée."""
from __future__ import annotations
import threading
from http.server import BaseHTTPRequestHandler, HTTPServer

from .handler import Handler


def start_health_server(handler: Handler, port: int = 8000) -> HTTPServer:
    class H(BaseHTTPRequestHandler):
        def do_GET(self):
            if self.path == "/health":
                body = b"ok"
            elif self.path == "/metrics":
                body = "".join(f"gateway_{k} {v}\n" for k, v in sorted(handler.metrics.items())).encode()
            else:
                self.send_response(404); self.end_headers(); return
            self.send_response(200)
            self.send_header("Content-Type", "text/plain; charset=utf-8")
            self.end_headers()
            self.wfile.write(body)

        def log_message(self, *args):  # silence
            pass

    srv = HTTPServer(("0.0.0.0", port), H)
    threading.Thread(target=srv.serve_forever, daemon=True).start()
    return srv
