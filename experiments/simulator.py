"""Isolated interface to the verified C simulator.

Every invocation runs in a temporary directory.  The repository, default
parameter files, and previous results are never modified by a simulation.
"""

from __future__ import annotations

import csv
import shutil
import subprocess
import tempfile
from collections.abc import Mapping
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SIMULATOR = ROOT / "simulator" / "src" / "sim"
CONFIG_DIR = ROOT / "configs"

NUMERIC_FINALSTATS = {
    "R",
    "R_abs",
    "R2",
    "R2_norm",
    "R2_max_norm",
    "avg_switch_noidle",
    "avg_residual_norm",
    "avg_pre_service_residual_norm",
    "max_residual_norm",
    "max_pre_service_residual_norm",
    "avg_switch",
    "target_vector_norm",
    "tracker_vector_norm",
    "post_removal_R",
    "post_removal_R_abs",
    "post_removal_R2_norm",
    "post_removal_R2_max_norm",
}


def format_parameter(value: object) -> str:
    if isinstance(value, bool):
        return "1" if value else "0"
    if isinstance(value, float):
        return format(value, ".17g")
    return str(value)


def write_overrides(path: Path, values: Mapping[str, object]) -> None:
    """Write a simulator override file with deterministic key ordering."""
    lines = [
        f"{name}\t{format_parameter(value)}" for name, value in sorted(values.items())
    ]
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def parse_finalstats(path: Path) -> dict[str, object]:
    with path.open(newline="", encoding="utf-8") as handle:
        row = next(csv.DictReader(handle))
    result: dict[str, object] = dict(row)
    for name in NUMERIC_FINALSTATS.intersection(row):
        result[name] = float(row[name])
    if "post_removal_steps" in row:
        result["post_removal_steps"] = int(float(row["post_removal_steps"]))
    return result


def run_simulation(
    overrides: Mapping[str, object],
    *,
    seed: int,
    scratch_root: Path | None = None,
) -> dict[str, object]:
    """Run one simulation and return the single final-statistics row."""
    if not SIMULATOR.is_file():
        raise FileNotFoundError("Simulator is not built. Run `make build` first.")

    parent = None if scratch_root is None else str(scratch_root.expanduser().resolve())
    if scratch_root is not None:
        scratch_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="pta_run_", dir=parent) as temporary:
        work = Path(temporary)
        output = work / "output"
        seed_directory = output / f"run.{seed}"
        seed_directory.mkdir(parents=True)

        shutil.copy2(SIMULATOR, work / "sim")
        shutil.copy2(CONFIG_DIR / "params.default", work / "params.default")
        shutil.copy2(CONFIG_DIR / "opfiles.full", work / "opfiles.default")
        shutil.copy2(CONFIG_DIR / "opfiles.smoke", work / "opfiles")
        (seed_directory / f"run.{seed}.random").write_text(
            f"{seed}\n", encoding="utf-8"
        )
        (work / "run.num").write_text("0\n", encoding="utf-8")

        complete = {
            "Rerun": seed,
            "Run_num_file": "run.num",
            "Output_path": "output",
            "Print_params": 0,
            "Print_step": 0,
            "Gnuplot_plots": 0,
            "Animate_thresh": 0,
            **overrides,
        }
        write_overrides(work / "params", complete)
        completed = subprocess.run(
            ["./sim", "params", "opfiles"],
            cwd=work,
            check=False,
            capture_output=True,
            text=True,
        )
        if completed.returncode != 0 or "ends ok" not in completed.stdout:
            raise RuntimeError(
                f"Simulation failed for seed {seed}.\n"
                f"stdout:\n{completed.stdout}\nstderr:\n{completed.stderr}"
            )
        summaries = sorted(output.glob("run.*/*.finalstats"))
        if len(summaries) != 1:
            raise RuntimeError(f"Expected one finalstats file; found {len(summaries)}")
        return parse_finalstats(summaries[0])
