#!/usr/bin/env python3
"""Run a small, deterministic end to end simulator check.

The simulator reads ``params.default`` and ``opfiles.default`` from its current
directory before applying the supplied override files. This wrapper constructs
that directory in a temporary location, so the test never writes into the
repository. The fixed agent seed and target path seed make failures repeatable.
"""

from __future__ import annotations

import csv
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SIMULATOR = ROOT / "simulator" / "src" / "sim"
CONFIG = ROOT / "configs"
REQUIRED_METRICS = {
    "R",
    "R_abs",
    "R2",
    "R2_norm",
    "avg_switch_noidle",
    "post_removal_R",
    "post_removal_R_abs",
    "post_removal_R2_norm",
    "post_removal_steps",
}


def main() -> None:
    if not SIMULATOR.exists():
        raise SystemExit("Simulator is not built. Run `make build` first.")

    with tempfile.TemporaryDirectory(prefix="pta_smoke_") as tmp:
        work = Path(tmp)
        output = work / "output"
        seed_dir = output / "run.4242"
        seed_dir.mkdir(parents=True)

        # Rerun mode reads this seed, while the new result is written to run.1.
        (seed_dir / "run.4242.random").write_text("4242\n", encoding="utf-8")
        (work / "run.num").write_text("0\n", encoding="utf-8")
        shutil.copy2(CONFIG / "params.default", work / "params.default")
        shutil.copy2(CONFIG / "opfiles.full", work / "opfiles.default")
        shutil.copy2(CONFIG / "smoke.params", work / "smoke.params")
        shutil.copy2(CONFIG / "opfiles.smoke", work / "opfiles.smoke")

        completed = subprocess.run(
            [str(SIMULATOR), "smoke.params", "opfiles.smoke"],
            cwd=work,
            check=False,
            capture_output=True,
            text=True,
        )
        if completed.returncode != 0 or "ends ok" not in completed.stdout:
            raise RuntimeError(
                "Smoke simulation failed.\n"
                f"stdout:\n{completed.stdout}\n"
                f"stderr:\n{completed.stderr}"
            )

        finalstats = output / "run.1" / "run.1.finalstats"
        if not finalstats.exists():
            raise RuntimeError(f"Missing simulator summary: {finalstats}")
        with finalstats.open(newline="", encoding="utf-8") as handle:
            row = next(csv.DictReader(handle))

        missing = REQUIRED_METRICS.difference(row)
        if missing:
            raise RuntimeError(f"Missing required metrics: {sorted(missing)}")
        for metric in REQUIRED_METRICS - {"post_removal_steps"}:
            float(row[metric])
        if int(row["post_removal_steps"]) != 0:
            raise RuntimeError("Clean smoke test unexpectedly used a removal window")

    print("PASS: deterministic PTA smoke simulation and metric contract")


if __name__ == "__main__":
    main()
