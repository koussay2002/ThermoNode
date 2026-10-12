"""EI-02 : analyse et vérification des trames JSON + CRC-16/CCITT-FALSE (EF-G2 : rejet des trames invalides)."""
from __future__ import annotations
import json
import re
from dataclasses import dataclass

STATES = ("INIT", "NORMAL", "ALERT", "FAULT")
_CRC_RE = re.compile(r',"crc":"([0-9A-Fa-f]{4})"')


class FrameError(ValueError):
    """Trame rejetée (format, type, CRC)."""


@dataclass(frozen=True)
class Frame:
    node_id: str
    seq: int
    t: int
    temp: float
    state: str


def crc16_ccitt_false(data: bytes) -> int:
    crc = 0xFFFF
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if crc & 0x8000 else (crc << 1) & 0xFFFF
    return crc


def check_crc(text: str) -> bool:
    """Le CRC est calculé sur le texte JSON privé de son membre ,"crc":"XXXX"."""
    m = _CRC_RE.search(text)
    if not m:
        return False
    stripped = text[: m.start()] + text[m.end():]
    return crc16_ccitt_false(stripped.encode("utf-8")) == int(m.group(1), 16)


def parse_frame(text: str) -> Frame:
    if not isinstance(text, str) or len(text) > 256:
        raise FrameError("taille")
    if not check_crc(text):
        raise FrameError("crc")
    try:
        obj = json.loads(text)
    except json.JSONDecodeError as exc:
        raise FrameError(f"json: {exc.msg}") from exc
    if not isinstance(obj, dict):
        raise FrameError("format")
    try:
        node_id, seq, t, temp, state = obj["id"], obj["seq"], obj["t"], obj["temp"], obj["state"]
    except KeyError as exc:
        raise FrameError(f"champ manquant: {exc.args[0]}") from exc
    if not isinstance(node_id, str) or not re.fullmatch(r"[A-Za-z0-9_-]{1,7}", node_id):
        raise FrameError("id")
    if isinstance(seq, bool) or not isinstance(seq, int):
        raise FrameError("seq")
    if isinstance(t, bool) or not isinstance(t, int) or t < 0:
        raise FrameError("t")
    try:
        temp = float(temp)
    except (TypeError, ValueError) as exc:
        raise FrameError("temp") from exc
    if state not in STATES:
        raise FrameError("state")
    return Frame(node_id, seq, t, float(temp), state)
