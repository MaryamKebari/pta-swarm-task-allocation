#!/usr/bin/env python3
"""Generate the empirical diagnostic paired with the population analysis.

The run replays the frozen-reference Agent/HT1/latent PTA configuration at
the reference task count, step ratio, and demand class.  It uses the exact
simulation and target-path seeds saved in the August 7 allocation grid and
records per-step task demand, recruitment counts, and mean stored thresholds.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import shutil
import subprocess
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

import numpy as np
import pandas as pd

EXPECTED_FTRACKER_SHA256 = (
    "86ba010cdf4fe7d097fdfd3328331bddf54841efa4cdc547156cf809c54d35f9"
)
POPS = (50, 100, 500, 1000)
TASKS = 4
STEP_RATIO = 2.0
PATTERN = "scurve"
PATTERN_FULL = "legacy_vector_scurve"
MAX_STEPS = 1000
REPS = 100

OPFILES = """random 0
params 0
initpop 0
finalpop 0
gnu 0
stepsummary 0
stepdemand 0
steptaskdemand 1
steptaskcounts 1
steptaskthresh 1
stepnorthsouth 0
stepeastwest 0
stepagentaction 0
stepagentactionwtime 0
stepagentactionxyz 0
finalstats 1
finalagent 0
finaltask 0
steptargetpath 0
steptrackerpath 0
stepthresh 0
stepthreshnorth 0
stepthreshsouth 0
stepthresheast 0
stepthreshwest 0
threshrange 0
finalintensity 0
stephistnorth 0
stephistsouth 0
stephisteast 0
stephistwest 0
intensityrange 0
stepintensity 0
stepintensitynorth 0
stepintensityeast 0
stepintensitysouth 0
stepintensitywest 0
finalthreshswitch 0
finalthreshact 0
initprob 0
finalprob 0
stepprobnorth 0
stepprobsouth 0
stepprobeast 0
stepprobwest 0
stepagentmintask 0
stepagentmintaskaction 0
"""


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def set_param(text: str, name: str, value: object) -> str:
    pattern = re.compile(rf"^{re.escape(name)}\s*(?:=\s*)?.*$", re.MULTILINE)
    line = f"{name}\t{value}"
    if pattern.search(text):
        return pattern.sub(line, text)
    return text.rstrip() + "\n" + line + "\n"


def read_task_columns(path: Path, prefix: str) -> np.ndarray:
    frame = pd.read_csv(path)
    columns = [f"{prefix}_task{i}" for i in range(1, TASKS + 1)]
    return frame[columns].to_numpy(dtype=np.float64)


def load_design(
    allocation_csv: Path, parameters_csv: Path
) -> tuple[pd.DataFrame, dict]:
    allocation = pd.read_csv(allocation_csv)
    design = allocation[
        (allocation["family"] == "PTA")
        & (allocation["threshold_range"] == "HT1")
        & (allocation["threshold_mode"] == "latent")
        & (allocation["gain_scheme"] == "Agent")
        & (allocation["n"] == TASKS)
        & (allocation["step_ratio"] == STEP_RATIO)
        & (allocation["pattern"] == PATTERN)
        & (allocation["pop"].isin(POPS))
    ].copy()
    design = design.sort_values(["pop", "rep"])
    expected = len(POPS) * REPS
    if len(design) != expected:
        raise ValueError(f"Expected {expected} seed rows, found {len(design)}")
    if not (design.groupby("pop").size() == REPS).all():
        raise ValueError("Each population must contain exactly 100 repetitions")

    params = pd.read_csv(parameters_csv)
    chosen = params[
        (params["family"] == "PTA")
        & (params["threshold_range"] == "HT1")
        & (params["threshold_mode"] == "latent")
        & (params["gain_scheme"] == "Agent")
    ]
    if len(chosen) != 1:
        raise ValueError(
            f"Expected one frozen Agent/HT1/latent row, found {len(chosen)}"
        )
    return design, chosen.iloc[0].to_dict()


def run_one(
    row: dict,
    gains: dict,
    sim_src: Path,
    scratch_root: Path,
) -> tuple[int, int, np.ndarray, np.ndarray, np.ndarray, dict]:
    pop = int(row["pop"])
    rep = int(row["rep"])
    seed = int(row["seed"])
    target_seed = int(row["target_path_seed"])

    with tempfile.TemporaryDirectory(
        prefix=f"popdiag_{pop}_{rep}_", dir=str(scratch_root)
    ) as tmp:
        work = Path(tmp)
        for name in ("sim", "params.default", "opfiles.default"):
            shutil.copy2(sim_src / name, work / name)
        output = work / "out"
        seed_dir = output / f"run.{seed}"
        seed_dir.mkdir(parents=True)
        (seed_dir / f"run.{seed}.random").write_text(f"{seed}\n")
        (work / "run_num").write_text("0\n")
        (work / "opfiles").write_text(OPFILES)

        text = (sim_src / "params").read_text()
        values = {
            "Output_path": output,
            "Run_num_file": work / "run_num",
            "Rerun": seed,
            "Max_steps": MAX_STEPS,
            "Pop_size": pop,
            "Num_tasks": TASKS,
            "Step_ratio": STEP_RATIO,
            "Task_demand_pattern": PATTERN_FULL,
            "Path_amplitude": 120,
            "Path_period": 600,
            "Demand_segment_len": 60,
            "Task_opposition_mode": 1,
            "Task_selection": "random",
            "Thresh_dynamic": 2,
            "Pid": 1,
            "Pid_latent_thresholds": 1,
            "Pid_integral_leak": 0.99,
            "Pid_integral_bound": 500,
            "Agent_pid_gains": 1,
            "Agent_pid_p_spread": gains["Agent_pid_p_spread"],
            "Agent_pid_i_spread": gains["Agent_pid_i_spread"],
            "Agent_pid_d_spread": gains["Agent_pid_d_spread"],
            "P_gain": gains["P_gain"],
            "I_gain": gains["I_gain"],
            "D_gain": gains["D_gain"],
            "FixedTargetPath": 1,
            "TargetPathSeed": target_seed,
            "Demand_switch_step": 0,
            "Kill_number": 0,
            "First_extinction": 0,
            "Extinction_period": 0,
            "Removal_capacity_mode": 1,
            "Feedback_noise_enabled": 0,
            "Feedback_noise_alpha": 0,
            "Feedback_noise_clip": 0,
            "Feedback_bias_alpha": 0,
            "Feedback_bias_mode": 0,
        }
        for name, value in values.items():
            text = set_param(text, name, value)
        (work / "params").write_text(text)

        completed = subprocess.run(
            ["./sim", "params", "opfiles"],
            cwd=work,
            check=False,
            capture_output=True,
            text=True,
        )
        if completed.returncode != 0:
            raise RuntimeError(
                f"Simulator failed for population={pop}, rep={rep}: {completed.stderr}"
            )
        finalstats_files = list(output.glob("run.*/*.finalstats"))
        if not finalstats_files:
            produced_files = sorted(
                str(path.relative_to(work)) for path in work.rglob("*")
            )
            raise RuntimeError(
                f"No finalstats for population={pop}, rep={rep}. "
                f"stdout={completed.stdout!r}; stderr={completed.stderr!r}; "
                f"files={produced_files}"
            )
        produced = finalstats_files[0].parent
        demand_file = next(produced.glob("*.steptaskdemand"))
        counts_file = next(produced.glob("*.steptaskcounts"))
        thresh_file = next(produced.glob("*.steptaskthresh"))
        demand = read_task_columns(demand_file, "arrival")
        counts = read_task_columns(counts_file, "actors")
        thresholds = read_task_columns(thresh_file, "avg_thresh")
        with next(produced.glob("*.finalstats")).open(newline="") as handle:
            stats = next(csv.DictReader(handle))
        stats = {key: float(value) for key, value in stats.items() if key != "run"}

    if demand.shape != (MAX_STEPS, TASKS):
        raise ValueError(
            f"Unexpected trajectory shape {demand.shape} for {pop=}, {rep=}"
        )
    return pop, rep, demand, counts, thresholds, stats


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--sim-src", type=Path, required=True)
    parser.add_argument("--allocation-csv", type=Path, required=True)
    parser.add_argument("--parameters-csv", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--workers", type=int, default=8)
    args = parser.parse_args()

    args.sim_src = args.sim_src.resolve()
    args.out = args.out.resolve()
    args.out.mkdir(parents=True, exist_ok=True)
    scratch = args.out / "scratch"
    scratch.mkdir(exist_ok=True)
    actual_hash = sha256(args.sim_src / "ftracker.c")
    if actual_hash != EXPECTED_FTRACKER_SHA256:
        raise ValueError(
            f"Wrong controller source: {actual_hash}; expected {EXPECTED_FTRACKER_SHA256}"
        )
    design, gains = load_design(args.allocation_csv, args.parameters_csv)

    demand = np.empty((len(POPS), REPS, MAX_STEPS, TASKS), dtype=np.float32)
    counts = np.empty_like(demand)
    thresholds = np.empty_like(demand)
    stats_rows: list[dict] = []
    pop_index = {pop: index for index, pop in enumerate(POPS)}
    jobs = [row._asdict() for row in design.itertuples(index=False)]
    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        futures = [
            pool.submit(run_one, row, gains, args.sim_src, scratch) for row in jobs
        ]
        for index, future in enumerate(as_completed(futures), start=1):
            pop, rep, dem, cnt, thr, stats = future.result()
            pidx = pop_index[pop]
            demand[pidx, rep] = dem
            counts[pidx, rep] = cnt
            thresholds[pidx, rep] = thr
            expected = design[(design["pop"] == pop) & (design["rep"] == rep)].iloc[0]
            stats_rows.append(
                {
                    "pop": pop,
                    "rep": rep,
                    "seed": int(expected["seed"]),
                    "target_path_seed": int(expected["target_path_seed"]),
                    "R_saved": float(expected["R"]),
                    "R_replayed": stats["R"],
                    "R_abs_replayed": stats["R_abs"],
                    "R2_norm_replayed": stats["R2_norm"],
                }
            )
            if index % 25 == 0 or index == len(futures):
                print(f"Completed {index}/{len(futures)} runs", flush=True)

    stats_frame = pd.DataFrame(stats_rows).sort_values(["pop", "rep"])
    stats_frame["R_abs_difference"] = (
        stats_frame["R_replayed"] - stats_frame["R_saved"]
    ).abs()
    stats_frame.to_csv(args.out / "replay_validation.csv", index=False)
    np.savez_compressed(
        args.out / "population_mechanism_trajectories.npz",
        populations=np.asarray(POPS),
        demand=demand,
        counts=counts,
        thresholds=thresholds,
    )
    manifest = {
        "controller_source_sha256": actual_hash,
        "configuration": "PTA-HT1, latent stored-threshold mode, Agent gains",
        "gain_source": str(args.parameters_csv.resolve()),
        "seed_source": str(args.allocation_csv.resolve()),
        "population": list(POPS),
        "tasks": TASKS,
        "step_ratio": STEP_RATIO,
        "demand_class": "Iterative gradual",
        "demand_code": PATTERN,
        "steps": MAX_STEPS,
        "repetitions": REPS,
        "gains": {
            key: float(gains[key])
            for key in (
                "P_gain",
                "I_gain",
                "D_gain",
                "Agent_pid_p_spread",
                "Agent_pid_i_spread",
                "Agent_pid_d_spread",
            )
        },
        "maximum_absolute_R_replay_difference": float(
            stats_frame["R_abs_difference"].max()
        ),
    }
    (args.out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))


if __name__ == "__main__":
    main()
