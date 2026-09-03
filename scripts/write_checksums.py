#!/usr/bin/env python3
"""Write SHA-256 checksums for versioned research artifacts."""

from __future__ import annotations

import hashlib
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
INCLUDE = ("analysis", "configs", "data/manifests", "data/parameters", "data/processed", "docs", "simulator/src")
EXCLUDE_NAMES = {"CHECKSUMS.sha256", "sim"}


def digest(path: Path) -> str:
    value = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            value.update(block)
    return value.hexdigest()


def main() -> None:
    files: list[Path] = []
    for name in INCLUDE:
        files.extend(path for path in (ROOT / name).rglob("*") if path.is_file())
    files = sorted(path for path in files if path.name not in EXCLUDE_NAMES and path.suffix != ".o")
    lines = [f"{digest(path)}  {path.relative_to(ROOT)}" for path in files]
    (ROOT / "CHECKSUMS.sha256").write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Wrote checksums for {len(lines)} artifacts")


if __name__ == "__main__":
    main()
