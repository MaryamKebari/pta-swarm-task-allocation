#!/usr/bin/env python3
"""Run the manuscript validation campaigns with frozen reference parameters.

The default grids match the paper.  Large campaigns are resumable: each
completed row is appended to a CSV, and an existing row with the same complete
experimental key is skipped.  Use ``--smoke`` before launching a full run.
"""

from __future__ import annotations

import argparse
import csv
import json
import os
import sys
from collections.abc import Iterable, Iterator
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from datetime import datetime, timezone
from pathlib import Path

if __package__ in {None, ""}:
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from experiments.definitions import (
    PATTERN_CODES,
    MethodConfiguration,
    filter_configurations,
    load_method_configurations,
)
from experiments.simulator import run_simulation

ROOT = Path(__file__).resolve().parents[1]
SEED_MAP = ROOT / "data" / "manifests" / "allocation_seed_map.csv"
FEEDBACK_SEED_MAP = ROOT / "data" / "manifests" / "imperfect_feedback_seed_map.csv"
DEFAULT_OUTPUTS = {
    "clean": ROOT / "data" / "raw" / "allocation_grid" / "per_run_results.csv",
    "ablation": ROOT / "data" / "raw" / "controller_ablation" / "per_run_results.csv",
    "removal": ROOT / "data" / "raw" / "agent_removal" / "per_run_results.csv",
    "feedback": ROOT / "data" / "raw" / "imperfect_feedback" / "per_run_results.csv",
}

COMMON_COLUMNS = [
    "pop",
    "n",
    "step_ratio",
    "pattern",
    "pattern_full",
    "family",
    "method",
    "method_label",
    "threshold_mode",
    "threshold_range",
    "gain_scheme",
    "run_method_id",
    "td",
    "pid",
    "agent_pid_gains",
    "pid_latent_thresholds",
    "P_gain",
    "I_gain",
    "D_gain",
    "Agent_pid_p_spread",
    "Agent_pid_i_spread",
    "Agent_pid_d_spread",
    "Thresh_increase",
    "Thresh_decrease",
    "rep",
    "seed",
    "target_path_seed",
    "max_steps",
]
METRIC_COLUMNS = [
    "R",
    "R_abs",
    "R2",
    "R2_norm",
    "R2_max_norm",
    "avg_switch_noidle",
    "avg_post",
    "avg_pre",
    "avg_switch",
    "target_norm",
    "tracker_norm",
]
EXTRA_COLUMNS = {
    "clean": [],
    "ablation": ["ablation_variant"],
    "removal": [
        "kill_percent",
        "kill_number",
        "first_extinction",
        "removal_capacity_mode",
        "post_removal_R",
        "post_removal_R_abs",
        "post_removal_R2_norm",
        "post_removal_R2_max_norm",
        "post_removal_steps",
    ],
    "feedback": [
        "feedback_noise_alpha",
        "feedback_bias_alpha",
        "feedback_noise_sigma_mode",
        "feedback_noise_clip",
        "feedback_noise_seed",
        "feedback_bias_seed",
        "feedback_bias_mode",
    ],
}
KEY_COLUMNS = {
    "clean": ["run_method_id", "pop", "n", "step_ratio", "pattern", "rep"],
    "ablation": [
        "run_method_id",
        "pop",
        "n",
        "step_ratio",
        "pattern",
        "rep",
        "ablation_variant",
    ],
    "removal": [
        "run_method_id",
        "pop",
        "n",
        "step_ratio",
        "pattern",
        "rep",
        "kill_percent",
    ],
    "feedback": [
        "run_method_id",
        "pop",
        "n",
        "step_ratio",
        "pattern",
        "rep",
        "feedback_noise_alpha",
        "feedback_bias_alpha",
    ],
}


@dataclass(frozen=True)
class Job:
    experiment: str
    configuration: MethodConfiguration
    seed_row: dict[str, str]
    extra: dict[str, object]

    def key(self) -> tuple[str, ...]:
        values = self.base_metadata() | self.extra
        if self.experiment == "ablation":
            values["run_method_id"] = (
                f"{values['run_method_id']}|{self.extra['ablation_variant']}"
            )
        return tuple(str(values[column]) for column in KEY_COLUMNS[self.experiment])

    def base_metadata(self) -> dict[str, object]:
        row = self.seed_row
        return {
            "pop": int(row["pop"]),
            "n": int(row["n"]),
            "step_ratio": float(row["step_ratio"]),
            "pattern": row["pattern"],
            "pattern_full": row["pattern_full"],
            **self.configuration.metadata(),
            "rep": int(row["rep"]),
            "seed": int(row["seed"]),
            "target_path_seed": int(row["target_path_seed"]),
            "max_steps": 1000,
        }


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("experiment", choices=tuple(DEFAULT_OUTPUTS))
    parser.add_argument(
        "--output", type=Path, help="Result CSV; defaults under data/raw"
    )
    parser.add_argument(
        "--workers", type=int, default=max(1, min(8, os.cpu_count() or 1))
    )
    parser.add_argument("--scratch-root", type=Path, default=None)
    parser.add_argument(
        "--families", nargs="+", choices=["CT", "LFTA", "SBTA", "SETA", "PTA"]
    )
    parser.add_argument(
        "--configurations", nargs="+", help="Exact run_method_id values"
    )
    parser.add_argument("--populations", nargs="+", type=int)
    parser.add_argument("--tasks", nargs="+", type=int)
    parser.add_argument("--step-ratios", nargs="+", type=float)
    parser.add_argument("--patterns", nargs="+", choices=sorted(PATTERN_CODES))
    parser.add_argument("--repetitions", type=int, default=100)
    parser.add_argument(
        "--smoke", action="store_true", help="Run a small cross-family contract check"
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Write only the manifest and planned count",
    )
    parser.add_argument(
        "--overwrite", action="store_true", help="Replace an existing result CSV"
    )
    parser.add_argument("--chunk-size", type=int, default=256)
    args = parser.parse_args()
    if args.workers < 1 or args.repetitions < 1 or args.chunk_size < 1:
        parser.error("workers, repetitions, and chunk size must be positive")
    return args


def load_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def filtered_seed_rows(args: argparse.Namespace) -> list[dict[str, str]]:
    rows = load_csv(SEED_MAP)
    filters = {
        "pop": set(args.populations or []),
        "n": set(args.tasks or []),
        "step_ratio": set(args.step_ratios or []),
        "pattern": set(args.patterns or []),
    }
    if args.experiment in {"ablation", "feedback"}:
        filters["pop"] = {500}
    if args.experiment == "ablation":
        filters["n"] = {4, 12}
        filters["step_ratio"] = {1.5, 2.5}
    if args.experiment == "feedback":
        filters["n"] = {4, 12}
    rows = [
        row
        for row in rows
        if int(row["rep"]) < args.repetitions
        and all(
            not levels
            or (
                float(row[name])
                if name == "step_ratio"
                else int(row[name])
                if name in {"pop", "n"}
                else row[name]
            )
            in levels
            for name, levels in filters.items()
        )
    ]
    if not rows:
        raise ValueError("No seed rows match the requested campaign filters")
    return rows


def smoke_configurations(
    configurations: list[MethodConfiguration],
) -> list[MethodConfiguration]:
    selected = []
    for family in ["CT", "LFTA", "SBTA", "SETA", "PTA"]:
        candidates = [config for config in configurations if config.family == family]
        selected.append(
            next(
                (
                    config
                    for config in candidates
                    if config.threshold_range == "HT1"
                    and config.threshold_mode == "clamped"
                    and (family != "PTA" or config.gain_scheme == "Agent")
                ),
                candidates[0],
            )
        )
    return selected


def build_jobs(args: argparse.Namespace) -> Iterator[Job]:
    configurations = filter_configurations(
        load_method_configurations(),
        set(args.families or []),
        set(args.configurations or []),
    )
    seeds = filtered_seed_rows(args)
    if args.smoke:
        configurations = smoke_configurations(configurations)
        seeds = seeds[:1]

    if args.experiment == "clean":
        for seed_row in seeds:
            for configuration in configurations:
                yield Job("clean", configuration, seed_row, {})
        return

    if args.experiment == "removal":
        levels = [0, 20] if args.smoke else [0, 5, 10, 20, 30, 40, 50]
        for seed_row in seeds:
            population = int(seed_row["pop"])
            for kill_percent in levels:
                for configuration in configurations:
                    yield Job(
                        "removal",
                        configuration,
                        seed_row,
                        {
                            "kill_percent": kill_percent,
                            "kill_number": int(population * kill_percent / 100),
                            "first_extinction": 500,
                            "removal_capacity_mode": "capacity_loss",
                        },
                    )
        return

    if args.experiment == "feedback":
        feedback_seeds = {
            (
                int(row["n"]),
                float(row["step_ratio"]),
                row["pattern"],
                int(row["rep"]),
            ): row
            for row in load_csv(FEEDBACK_SEED_MAP)
        }
        severities = (
            [(0.0, 0.0), (0.4, 0.2)]
            if args.smoke
            else [
                (alpha, beta)
                for alpha in [0.0, 0.05, 0.10, 0.20, 0.40]
                for beta in [0.0, 0.05, 0.10, 0.20]
            ]
        )
        for seed_row in seeds:
            key = (
                int(seed_row["n"]),
                float(seed_row["step_ratio"]),
                seed_row["pattern"],
                int(seed_row["rep"]),
            )
            perturbation_seed = feedback_seeds[key]
            for alpha, beta in severities:
                for configuration in configurations:
                    yield Job(
                        "feedback",
                        configuration,
                        seed_row,
                        {
                            "feedback_noise_alpha": alpha,
                            "feedback_bias_alpha": beta,
                            "feedback_noise_sigma_mode": "mean_active_task_demand",
                            "feedback_noise_clip": 0,
                            "feedback_noise_seed": int(
                                perturbation_seed["feedback_noise_seed"]
                            ),
                            "feedback_bias_seed": int(
                                perturbation_seed["feedback_bias_seed"]
                            ),
                            "feedback_bias_mode": "task_fixed_random",
                        },
                    )
        return

    # The repeated reversal term ablation uses only latent Agent PTA under HT1/HT2.
    configurations = [
        config
        for config in configurations
        if config.family == "PTA"
        and config.threshold_range in {"HT1", "HT2"}
        and config.threshold_mode == "latent"
        and config.gain_scheme == "Agent"
    ]
    if not configurations:
        raise ValueError("The ablation requires latent Agent PTA under HT1 and HT2")
    variants = {
        "P": (True, False, False),
        "PI": (True, True, False),
        "PD": (True, False, True),
        "full_PTA": (True, True, True),
    }
    for seed_row in seeds:
        for configuration in configurations:
            for variant in variants:
                yield Job(
                    "ablation", configuration, seed_row, {"ablation_variant": variant}
                )


def simulator_overrides(job: Job) -> dict[str, object]:
    row = job.seed_row
    overrides = {
        "Max_steps": 1000,
        "Pop_size": int(row["pop"]),
        "Num_tasks": int(row["n"]),
        "Step_ratio": float(row["step_ratio"]),
        "Task_demand_pattern": row["pattern_full"],
        "Target_path": row["pattern"],
        "Demand_segment_len": 60,
        "Path_amplitude": 20,
        "Path_period": 40,
        "Task_opposition_mode": 1,
        "Task_selection": "random",
        "FixedTargetPath": 1,
        "TargetPathSeed": int(row["target_path_seed"]),
        "Kill_number": 0,
        "First_extinction": -1,
        "Extinction_period": 0,
        "Removal_capacity_mode": 1,
        "Feedback_noise_enabled": 0,
        "Feedback_noise_alpha": 0,
        "Feedback_noise_clip": 0,
        "Feedback_bias_alpha": 0,
        "Feedback_bias_mode": "none",
        **job.configuration.simulator_overrides(),
    }
    if job.experiment == "removal":
        overrides.update(
            {
                "Kill_number": job.extra["kill_number"],
                "First_extinction": 500,
                "Removal_capacity_mode": 1,
            }
        )
    elif job.experiment == "feedback":
        overrides.update(
            {
                "Feedback_noise_enabled": 1,
                "Feedback_noise_alpha": job.extra["feedback_noise_alpha"],
                "Feedback_noise_seed": job.extra["feedback_noise_seed"],
                "Feedback_noise_sigma_mode": job.extra["feedback_noise_sigma_mode"],
                "Feedback_noise_clip": 0,
                "Feedback_bias_alpha": job.extra["feedback_bias_alpha"],
                "Feedback_bias_seed": job.extra["feedback_bias_seed"],
                "Feedback_bias_mode": job.extra["feedback_bias_mode"],
            }
        )
    elif job.experiment == "ablation":
        variant = str(job.extra["ablation_variant"])
        if "I" not in variant and variant != "full_PTA":
            overrides["I_gain"] = 0
            overrides["Agent_pid_i_spread"] = 0
        if "D" not in variant and variant != "full_PTA":
            overrides["D_gain"] = 0
            overrides["Agent_pid_d_spread"] = 0
    return overrides


def execute(job: Job, scratch_root: Path | None) -> dict[str, object]:
    stats = run_simulation(
        simulator_overrides(job),
        seed=int(job.seed_row["seed"]),
        scratch_root=scratch_root,
    )
    row = job.base_metadata() | job.extra
    if job.experiment == "ablation":
        variant = str(job.extra["ablation_variant"])
        row["method_label"] = (
            f"{row['method_label']} {variant.replace('full_PTA', 'full PTA')}"
        )
        row["run_method_id"] = f"{row['run_method_id']}|{variant}"
    row.update(
        {
            "R": stats["R"],
            "R_abs": stats["R_abs"],
            "R2": stats["R2"],
            "R2_norm": stats["R2_norm"],
            "R2_max_norm": stats["R2_max_norm"],
            "avg_switch_noidle": stats["avg_switch_noidle"],
            "avg_post": stats.get("avg_residual_norm", stats["R"]),
            "avg_pre": stats.get("avg_pre_service_residual_norm", ""),
            "avg_switch": stats.get("avg_switch", ""),
            "target_norm": stats.get("target_vector_norm", ""),
            "tracker_norm": stats.get("tracker_vector_norm", ""),
        }
    )
    if job.experiment == "removal":
        for name in [
            "post_removal_R",
            "post_removal_R_abs",
            "post_removal_R2_norm",
            "post_removal_R2_max_norm",
            "post_removal_steps",
        ]:
            row[name] = stats[name]
    return row


def chunks(values: Iterable[Job], size: int) -> Iterator[list[Job]]:
    block: list[Job] = []
    for value in values:
        block.append(value)
        if len(block) == size:
            yield block
            block = []
    if block:
        yield block


def existing_keys(path: Path, experiment: str) -> set[tuple[str, ...]]:
    if not path.is_file() or path.stat().st_size == 0:
        return set()
    with path.open(newline="", encoding="utf-8") as handle:
        return {
            tuple(str(row[column]) for column in KEY_COLUMNS[experiment])
            for row in csv.DictReader(handle)
        }


def write_manifest(
    args: argparse.Namespace, output: Path, planned: int, remaining: int
) -> None:
    manifest = {
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "experiment": args.experiment,
        "result_csv": output.name,
        "planned_rows_after_filters": planned,
        "rows_remaining_at_launch": remaining,
        "repetitions": args.repetitions,
        "smoke": args.smoke,
        "workers": args.workers,
        "selected_parameter_table": "data/parameters/reference_tuned_parameters.csv",
        "seed_map": "data/manifests/allocation_seed_map.csv",
        "feedback_seed_map": (
            "data/manifests/imperfect_feedback_seed_map.csv"
            if args.experiment == "feedback"
            else None
        ),
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    manifest_path = output.with_name("manifest.json")
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    args = parse_arguments()
    output = (args.output or DEFAULT_OUTPUTS[args.experiment]).expanduser().resolve()
    if args.overwrite and output.exists():
        output.unlink()
    done = existing_keys(output, args.experiment)
    planned = 0
    remaining = 0
    for job in build_jobs(args):
        planned += 1
        if job.key() not in done:
            remaining += 1
    write_manifest(args, output, planned, remaining)
    print(
        f"{args.experiment}: {planned:,} planned, {len(done):,} already present, "
        f"{remaining:,} remaining",
        flush=True,
    )
    if args.dry_run or remaining == 0:
        return

    output.parent.mkdir(parents=True, exist_ok=True)
    columns = COMMON_COLUMNS + METRIC_COLUMNS + EXTRA_COLUMNS[args.experiment]
    needs_header = not output.exists() or output.stat().st_size == 0
    completed_count = 0
    with output.open("a", newline="", encoding="utf-8", buffering=1) as handle:
        writer = csv.DictWriter(handle, fieldnames=columns, extrasaction="ignore")
        if needs_header:
            writer.writeheader()
        pending = (job for job in build_jobs(args) if job.key() not in done)
        for block in chunks(pending, args.chunk_size):
            with ThreadPoolExecutor(max_workers=args.workers) as pool:
                futures = {
                    pool.submit(execute, job, args.scratch_root): job for job in block
                }
                for future in as_completed(futures):
                    writer.writerow(future.result())
                    completed_count += 1
            handle.flush()
            print(f"completed {completed_count:,}/{remaining:,} new rows", flush=True)
    print(f"PASS: wrote {output}")


if __name__ == "__main__":
    main()
