#!/usr/bin/env python3
"""Check repository completeness, portability, provenance, and common leaks."""

from __future__ import annotations

import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXPECTED_FTRACKER = "86ba010cdf4fe7d097fdfd3328331bddf54841efa4cdc547156cf809c54d35f9"
REQUIRED = [
    "README.md",
    "LICENSE",
    "CITATION.cff",
    "configs/experiment_design.json",
    "configs/methods.json",
    "data/parameters/reference_tuned_parameters.csv",
    "data/manifests/source_hashes.json",
    "docs/DATA.md",
    "docs/EXPERIMENTS.md",
    "docs/METHODS.md",
    "docs/PROVENANCE.md",
    "docs/REPRODUCIBILITY.md",
    "simulator/src/ftracker.c",
]
TEXT_SUFFIXES = {".c", ".h", ".py", ".md", ".json", ".toml", ".yml", ".yaml", ".txt"}
FORBIDDEN_PATHS = [
    re.compile(r"/Users/[^/]+/"),
    re.compile(r"/Volumes/[^/]+/"),
    re.compile(r"/home/[^/]+/"),
]
SECRET_PATTERNS = [
    re.compile(r"\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}\b", re.IGNORECASE),
    re.compile(r"ghp_[A-Za-z0-9]{20,}"),
    re.compile(r"github_pat_[A-Za-z0-9_]{20,}"),
    re.compile(r"AKIA[0-9A-Z]{16}"),
    re.compile(r"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----"),
]


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def main() -> None:
    missing = [name for name in REQUIRED if not (ROOT / name).exists()]
    if missing:
        raise SystemExit(f"Missing required files: {missing}")

    with (ROOT / "configs/experiment_design.json").open(encoding="utf-8") as handle:
        design = json.load(handle)
    grid = design["clean_feedback_grid"]
    computed = (
        len(grid["population"])
        * len(grid["tasks"])
        * len(grid["step_ratio"])
        * len(grid["demand_class"])
    )
    if computed != grid["operating_conditions"]:
        raise SystemExit("Experiment grid count does not match its factor levels")

    source_hash = sha256(ROOT / "simulator/src/ftracker.c")
    if source_hash != EXPECTED_FTRACKER:
        raise SystemExit(f"ftracker.c provenance mismatch: {source_hash}")

    findings: list[str] = []
    for path in ROOT.rglob("*"):
        if (
            not path.is_file()
            or ".git" in path.parts
            or path.suffix.lower() not in TEXT_SUFFIXES
        ):
            continue
        if path.resolve() == Path(__file__).resolve():
            continue
        text = path.read_text(encoding="utf-8", errors="ignore")
        for pattern in FORBIDDEN_PATHS + SECRET_PATTERNS:
            if pattern.search(text):
                findings.append(f"{path.relative_to(ROOT)} matches {pattern.pattern}")
    if findings:
        raise SystemExit("Portability or secret scan failed:\n" + "\n".join(findings))

    print(
        f"PASS: repository contract, 144 condition grid, and source hash {source_hash[:12]}…"
    )


if __name__ == "__main__":
    main()
