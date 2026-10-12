"""EI-02 : analyse des trames — À COMPLÉTER (partie guidée G4)."""
import pytest
from gateway.frame import parse_frame, FrameError, crc16_ccitt_false


def signed(payload: str) -> str:
    """Ajoute le membre crc à un objet JSON (sans crc)."""
    crc = crc16_ccitt_false(payload.encode())
    return payload[:-1] + f',"crc":"{crc:04X}"}}'


VALID = signed('{"id":"TN01","seq":12,"t":1700000000,"temp":25.50,"state":"NORMAL"}')


def test_EI02_valid_frame_is_parsed():
    f = parse_frame(VALID)
    assert (f.node_id, f.seq, f.state) == ("TN01", 12, "NORMAL")
    assert f.temp == pytest.approx(25.5, abs=0.01)
