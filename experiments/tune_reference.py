#!/usr/bin/env python3
"""Tune every adaptive configuration at the manuscript reference setting.

The implementation follows the paper protocol: Optuna's multivariate TPE
sampler minimizes mean ``R`` over 20 common simulation seeds.  Studies use a
SQLite database and can be resumed safely.  CT is absent because it has no
adaptive parameter.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import sys
import zlib
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timezone
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import numpy as np
import optuna

from experiments.definitions import (
    REFERENCE_SETTING,
    MethodConfiguration,
    filter_configurations,
    load_method_configurations,
)
from experiments.run_campaign import Job
from experiments.run_campaign import (
    simulator_overrides as campaign_overrides,
)
from experiments.simulator import run_simulation

ROOT = Path(__file__).resolve().parents[1]
SEED_TABLE = ROOT / "data" / "manifests" / "tuning_seed_map.csv"
DEFAULT_OUTPUT = ROOT / "data" / "raw" / "reference_tuning"

TRIALS = {
    "LFTA": 100,
    "SBTA": 100,
    "SETA": 100,
    "PTA_Global": 100,
    "PTA_Agent": 240,
}
TUNING_SEED_FIELDS = [
    "run_method_id",
    "trial_number",
    "P_gain",
    "I_gain",
    "D_gain",
    "Agent_pid_p_spread",
    "Agent_pid_i_spread",
    "Agent_pid_d_spread",
    "Thresh_increase",
    "Thresh_decrease",
    "eta",
    "seed",
    "R",
    "R_abs",
]


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument(
        "--families", nargs="+", choices=["LFTA", "SBTA", "SETA", "PTA"]
    )
    parser.add_argument(
        "--configurations", nargs="+", help="Exact four-field run_method_id values"
    )
    parser.add_argument(
        "--workers", type=int, default=max(1, min(8, os.cpu_count() or 1))
    )
    parser.add_argument("--scratch-root", type=Path)
    parser.add_argument("--sampler-seed", type=int, default=20260807)
    parser.add_argument(
        "--smoke",
        action="store_true",
        help="Use two seeds and one trial per selected family",
    )
    args = parser.parse_args()
    if args.workers < 1:
        parser.error("workers must be positive")
    return args


def tuning_seeds(smoke: bool) -> list[int]:
    with SEED_TABLE.open(newline="", encoding="utf-8") as handle:
        seeds = [int(row["seed"]) for row in csv.DictReader(handle)]
    if len(seeds) != 20 or len(set(seeds)) != 20:
        raise ValueError("The tuning seed map must contain 20 unique seeds")
    return seeds[:2] if smoke else seeds


def study_key(configuration: MethodConfiguration) -> str:
    return configuration.run_method_id.replace("|", "__").rstrip("_")


def trial_budget(configuration: MethodConfiguration) -> int:
    if configuration.family != "PTA":
        return TRIALS[configuration.family]
    return TRIALS[f"PTA_{configuration.gain_scheme}"]


def sample_parameters(
    trial: optuna.Trial,
    configuration: MethodConfiguration,
) -> dict[str, float]:
    """Sample exactly the search space declared in the manuscript."""
    if configuration.family == "LFTA":
        return {
            "Thresh_increase": trial.suggest_float(
                "Thresh_increase", 1e-6, 1.0, log=True
            ),
            "Thresh_decrease": trial.suggest_float(
                "Thresh_decrease", 1e-6, 1.0, log=True
            ),
        }
    if configuration.family in {"SBTA", "SETA"}:
        return {"P_gain": trial.suggest_float("eta", 1e-8, 2e-2, log=True)}
    if configuration.gain_scheme == "Global":
        return {
            "P_gain": trial.suggest_float("P_gain", 1e-8, 2e-2, log=True),
            "I_gain": trial.suggest_float("I_gain", 1e-10, 3e-5, log=True),
            "D_gain": trial.suggest_float("D_gain", 1e-8, 1.2e-1, log=True),
        }
    return {
        "P_gain": trial.suggest_float("P_gain", 1e-8, 3e-2, log=True),
        "I_gain": trial.suggest_float("I_gain", 1e-10, 5e-5, log=True),
        "D_gain": trial.suggest_float("D_gain", 1e-8, 1.8e-1, log=True),
        "Agent_pid_p_spread": trial.suggest_float("Agent_pid_p_spread", 0.0, 0.8),
        "Agent_pid_i_spread": trial.suggest_float("Agent_pid_i_spread", 0.0, 0.8),
        "Agent_pid_d_spread": trial.suggest_float("Agent_pid_d_spread", 0.0, 0.8),
    }


def reference_seed_row(seed: int) -> dict[str, str]:
    return {
        "pop": str(REFERENCE_SETTING["pop"]),
        "n": str(REFERENCE_SETTING["n"]),
        "step_ratio": str(REFERENCE_SETTING["step_ratio"]),
        "pattern": str(REFERENCE_SETTING["pattern"]),
        "pattern_full": "legacy_vector_scurve",
        "rep": "0",
        "seed": str(seed),
        "target_path_seed": str(seed + 12_345),
    }


def evaluate_candidate(
    configuration: MethodConfiguration,
    candidate: dict[str, float],
    seeds: list[int],
    workers: int,
    scratch_root: Path | None,
) -> tuple[float, list[dict[str, float | int]]]:
    def one(seed: int) -> dict[str, float | int]:
        job = Job("clean", configuration, reference_seed_row(seed), {})
        overrides = campaign_overrides(job)
        overrides.update(candidate)
        stats = run_simulation(overrides, seed=seed, scratch_root=scratch_root)
        return {"seed": seed, "R": float(stats["R"]), "R_abs": float(stats["R_abs"])}

    with ThreadPoolExecutor(max_workers=workers) as pool:
        rows = list(pool.map(one, seeds))
    return float(np.mean([float(row["R"]) for row in rows])), rows


def append_seed_rows(path: Path, rows: list[dict[str, object]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    write_header = not path.exists() or path.stat().st_size == 0
    with path.open("a", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=TUNING_SEED_FIELDS)
        if write_header:
            writer.writeheader()
        writer.writerows(rows)


def tune_configuration(
    configuration: MethodConfiguration,
    args: argparse.Namespace,
    seeds: list[int],
) -> dict[str, object]:
    output = args.output.resolve()
    database = output / "optuna.sqlite3"
    per_seed = output / "tuning_per_seed.csv"
    identifier = study_key(configuration)
    sampler_seed = (
        args.sampler_seed + zlib.crc32(configuration.run_method_id.encode("utf-8"))
    ) % (2**32)
    sampler = optuna.samplers.TPESampler(seed=sampler_seed, multivariate=True)
    study = optuna.create_study(
        study_name=identifier,
        storage=f"sqlite:///{database}",
        direction="minimize",
        sampler=sampler,
        load_if_exists=True,
    )
    budget = 1 if args.smoke else trial_budget(configuration)

    def objective(trial: optuna.Trial) -> float:
        candidate = sample_parameters(trial, configuration)
        mean_r, outcomes = evaluate_candidate(
            configuration,
            candidate,
            seeds,
            args.workers,
            args.scratch_root,
        )
        append_seed_rows(
            per_seed,
            [
                {
                    "run_method_id": configuration.run_method_id,
                    "trial_number": trial.number,
                    **candidate,
                    **outcome,
                }
                for outcome in outcomes
            ],
        )
        return mean_r

    remaining = max(0, budget - len(study.trials))
    if remaining:
        study.optimize(objective, n_trials=remaining, show_progress_bar=True)
    selected = {
        **configuration.metadata(),
        "trials": len(study.trials),
        "tuning_repetitions": len(seeds),
        "selected_mean_R": study.best_value,
        **study.best_params,
    }
    if "eta" in study.best_params:
        selected["P_gain"] = study.best_params["eta"]
    return selected


def main() -> None:
    args = parse_arguments()
    args.output = args.output.expanduser().resolve()
    args.output.mkdir(parents=True, exist_ok=True)
    configurations = filter_configurations(
        [config for config in load_method_configurations() if config.family != "CT"],
        set(args.families or []),
        set(args.configurations or []),
    )
    if args.smoke:
        configurations = [
            next(config for config in configurations if config.family == family)
            for family in dict.fromkeys(config.family for config in configurations)
        ]
    seeds = tuning_seeds(args.smoke)
    selected = [tune_configuration(config, args, seeds) for config in configurations]

    fields = sorted({key for row in selected for key in row})
    with (args.output / "selected_parameters.csv").open(
        "w", newline="", encoding="utf-8"
    ) as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(selected)
    manifest = {
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "reference_setting": REFERENCE_SETTING,
        "objective": "mean R",
        "tuning_repetitions_per_trial": len(seeds),
        "sampler": "Optuna TPESampler(multivariate=True)",
        "sampler_seed_base": args.sampler_seed,
        "configurations": [config.run_method_id for config in configurations],
    }
    (args.output / "manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    print(f"PASS: wrote reference tuning outputs to {args.output}")


if __name__ == "__main__":
    main()
