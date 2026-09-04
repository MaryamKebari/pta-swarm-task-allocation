#!/usr/bin/env python3
"""Regenerate the principal public figures from committed processed data."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(*arguments: str) -> None:
    subprocess.run([sys.executable, *arguments], cwd=ROOT, check=True)


def main() -> None:
    (ROOT / "figures").mkdir(exist_ok=True)
    run("analysis/generate_paper_figures.py")
    run("analysis/clean_transfer/make_frozen_winner_atlas.py")
    run("analysis/imperfect_feedback/analyze_imperfect_feedback_results.py")
    run(
        "analysis/population/plot_population_mechanism_diagnostic.py",
        "--trajectories",
        "data/processed/population/population_mechanism_trajectories.npz",
        "--validation",
        "data/processed/population/replay_validation.csv",
        "--out",
        "figures/population_mechanism_diagnostic.pdf",
        "--summary",
        "data/processed/population/population_mechanism_summary.csv",
        "--representative",
        "data/processed/population/representative_timeseries.csv",
    )
    print("PASS: regenerated figures in figures/")


if __name__ == "__main__":
    main()
