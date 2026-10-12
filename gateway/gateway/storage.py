"""EF-G1 : stockage SQLite des mesures, conservation 90 jours."""
from __future__ import annotations
import sqlite3
import time
from pathlib import Path

RETENTION_S = 90 * 24 * 3600


class Storage:
    def __init__(self, db_path: str | Path):
        self.path = str(db_path)
        self.con = sqlite3.connect(self.path, check_same_thread=False)
        self.con.execute(
            "CREATE TABLE IF NOT EXISTS measures (node_id TEXT NOT NULL, seq INTEGER NOT NULL, t INTEGER, "
            "temp REAL, state TEXT, received_at INTEGER NOT NULL, PRIMARY KEY (node_id, seq))"
        )
        self.con.commit()

    def insert(self, node_id: str, seq: int, t: int, temp: float, state: str, now: int | None = None) -> None:
        now = int(time.time()) if now is None else now
        self.con.execute("INSERT OR IGNORE INTO measures VALUES (?,?,?,?,?,?)", (node_id, seq, t, temp, state, now))
        self.con.commit()

    def count(self, node_id: str | None = None) -> int:
        if node_id is None:
            return self.con.execute("SELECT COUNT(*) FROM measures").fetchone()[0]
        return self.con.execute("SELECT COUNT(*) FROM measures WHERE node_id=?", (node_id,)).fetchone()[0]

    def last_seq(self, node_id: str) -> int | None:
        row = self.con.execute("SELECT MAX(seq) FROM measures WHERE node_id=?", (node_id,)).fetchone()
        return row[0]

    def purge(self, now: int | None = None) -> int:
        now = int(time.time()) if now is None else now
        cur = self.con.execute("DELETE FROM measures WHERE received_at < ?", (now - RETENTION_S,))
        self.con.commit()
        return cur.rowcount

    def close(self) -> None:
        self.con.close()
