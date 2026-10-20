"""ENF-05 (dédoublonnage), EF-G1 (stockage), alertes, métriques."""
import pytest
from gateway.handler import Handler
from tests.unit.test_frame import signed


def frame(seq, state="NORMAL", node="TN01", temp=25.5):
    return signed(f'{{"id":"{node}","seq":{seq},"t":{1700000000 + seq},"temp":{temp},"state":"{state}"}}')


def test_EFG1_valid_frame_is_stored(tmp_path):
    h = Handler(tmp_path / "gw.sqlite")
    assert h.on_data(frame(0)) is not None
    assert h.storage.count("TN01") == 1


def test_ENF05_duplicate_seq_is_ignored(tmp_path):
    h = Handler(tmp_path / "gw.sqlite")
    h.on_data(frame(5))
    h.on_data(frame(5))
    assert h.storage.count("TN01") == 1
    assert h.metrics["frames_duplicate"] == 1


def test_ENF05_out_of_order_old_frame_is_ignored(tmp_path):
    h = Handler(tmp_path / "gw.sqlite")
    h.on_data(frame(7))
    h.on_data(frame(6))
    assert h.storage.count("TN01") == 1


def test_ENF05_gap_is_counted(tmp_path):
    h = Handler(tmp_path / "gw.sqlite")
    h.on_data(frame(1))
    h.on_data(frame(4))
    assert h.metrics["frames_gap"] == 2


def test_EFG2_rejected_frame_is_counted_not_stored(tmp_path):
    h = Handler(tmp_path / "gw.sqlite")
    assert h.on_data("garbage") is None
    assert h.storage.count() == 0 and h.metrics["frames_rejected"] == 1


def test_alert_callback_called_once_per_alert_frame(tmp_path):
    seen = []
    h = Handler(tmp_path / "gw.sqlite", on_alert=seen.append)
    h.on_data(frame(1))
    h.on_data(frame(2, "ALERT"))
    h.on_data(frame(3, "ALERT"))
    assert [f.seq for f in seen] == [2, 3] and h.metrics["alerts"] == 2


def test_nodes_are_independent(tmp_path):
    h = Handler(tmp_path / "gw.sqlite")
    h.on_data(frame(3, node="TN01"))
    h.on_data(frame(0, node="TN02"))
    assert h.storage.count() == 2


def test_last_seq_survives_restart(tmp_path):
    db = tmp_path / "gw.sqlite"
    Handler(db).on_data(frame(9))
    h2 = Handler(db)
    h2.on_data(frame(9))
    assert h2.storage.count() == 1 and h2.metrics["frames_duplicate"] == 1
