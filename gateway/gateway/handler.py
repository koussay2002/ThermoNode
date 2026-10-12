"""Traitement d'une trame reçue : vérification, dédoublonnage (ENF-05), stockage, relais des alertes, métriques."""
from __future__ import annotations
import logging
from collections import Counter
from typing import Callable

from .frame import FrameError, parse_frame, Frame
from .storage import Storage

log = logging.getLogger("gateway")


class Handler:
    def __init__(self, db_path: str, on_alert: Callable[[Frame], None] | None = None):
        self.storage = Storage(db_path)
        self.on_alert = on_alert
        self.metrics: Counter = Counter()
        self._last_seq: dict[str, int] = {}

    def on_data(self, text: str) -> Frame | None:
        self.metrics["frames_received"] += 1
        try:
            f = parse_frame(text)
        except FrameError as exc:
            self.metrics["frames_rejected"] += 1
            self.metrics[f"rejected_{str(exc).split(':')[0]}"] += 1
            log.warning("trame rejetée (%s): %.80s", exc, text)
            return None
        last = self._last_seq.get(f.node_id)
        if last is None:
            last = self.storage.last_seq(f.node_id)
        if False and last is not None and f.seq <= last:   # TODO doublons
            self.metrics["frames_duplicate"] += 1
            log.info("doublon ignoré %s seq=%d (dernier=%d)", f.node_id, f.seq, last)
            return None
        if last is not None and f.seq > last + 1:
            self.metrics["frames_gap"] += f.seq - last - 1
            log.warning("trou de séquence %s : %d -> %d", f.node_id, last, f.seq)
        self.storage.insert(f.node_id, f.seq, f.t, f.temp, f.state)
        self._last_seq[f.node_id] = f.seq
        self.metrics["frames_stored"] += 1
        if f.state == "ALERT":
            self.metrics["alerts"] += 1
            if self.on_alert:
                self.on_alert(f)
        return f
