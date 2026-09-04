#!/usr/bin/env python3
"""Check repository completeness, portability, provenance, and common leaks."""

from __future__ import annotations

import csv
import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
EXPECTED_FTRACKER = "86ba010cdf4fe7d097fdfd3328331bddf54841efa4cdc547156cf809c54d35f9"
EXPECTED_PARAMS = "01d394af9229c4494247f429e5714f6088de4b17d80aa3591a4bcad3088d1156"
REQUIRED = [
    "README.md",
    "LICENSE",
    "CITATION.cff",
    "configs/experiment_design.json",
    "configs/methods.json",
    "data/parameters/reference_tuned_parameters.csv",
    "data/provenance/source_hashes.json",
    "data/provenance/simulator_source_sha256.csv",
    "docs/DATA.md",
    "docs/EXPERIMENTS.md",
    "docs/FIGURES.md",
    "docs/METHODS.md",
    "docs/PAPER_TRACEABILITY.md",
    "docs/PROVENANCE.md",
    "docs/REPRODUCIBILITY.md",
    "analysis/figures.py",
    "experiments/run.py",
    "experiments/tune.py",
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
    grid = design["allocation"]
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
    params_hash = sha256(ROOT / "configs/params.default")
    if params_hash != EXPECTED_PARAMS:
        raise SystemExit(f"params.default provenance mismatch: {params_hash}")

    with (ROOT / "data/provenance/simulator_source_sha256.csv").open(
        newline="", encoding="utf-8"
    ) as handle:
        source_manifest = list(csv.DictReader(handle))
    source_mismatches = []
    for row in source_manifest:
        path = ROOT / row["path"]
        if not path.is_file():
            source_mismatches.append(f"missing {row['path']}")
        elif sha256(path) != row["sha256"]:
            source_mismatches.append(f"changed {row['path']}")
    if source_mismatches:
        raise SystemExit(
            "Simulator source provenance mismatch:\n" + "\n".join(source_mismatches)
        )

    skip_parts = {
        ".git",
        ".venv",
        "venv",
        "build",
        "__pycache__",
        ".ruff_cache",
        ".pytest_cache",
        "site-packages",
    }
    findings: list[str] = []
    for path in ROOT.rglob("*"):
        if (
            not path.is_file()
            or skip_parts.intersection(path.parts)
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
        "PASS: repository contract, 144 condition grid, "
        f"and {len(source_manifest)} verified simulator source files"
    )


if __name__ == "__main__":
    main()
