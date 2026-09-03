#!/usr/bin/env python3
"""Verify completeness and pairing properties of the reported removal grid."""

from __future__ import annotations

import argparse
from collections import defaultdict
from pathlib import Path

import numpy as np
import pandas as pd


STEP_RATIOS = {1.5, 2.0, 2.5}
POST_METRICS = [
    "post_removal_R",
    "post_removal_R_abs",
    "post_removal_R2_norm",
    "post_removal_R2_max_norm",
]


def config_id(frame: pd.DataFrame) -> pd.Series:
    family = frame["family"].astype(str)
    result = family + "-" + frame["threshold_range"].astype(str)
    has_mode = family.isin(["SETA", "PTA"])
    result = result.where(~has_mode, result + "-" + frame["threshold_mode"].astype(str))
    is_pta = family.eq("PTA")
    return result.where(~is_pta, result + "-" + frame["gain_scheme"].astype(str))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    usecols = [
        "pop",
        "n",
        "step_ratio",
        "pattern",
        "family",
        "threshold_range",
        "threshold_mode",
        "gain_scheme",
        "kill_percent",
        "kill_number",
        "rep",
        "seed",
        "target_path_seed",
        "post_removal_steps",
        "reused_from_allocation",
    ] + POST_METRICS

    cell_counts: defaultdict[tuple, int] = defaultdict(int)
    base_seed_pairs: defaultdict[tuple, set[tuple[int, int]]] = defaultdict(set)
    base_configs: defaultdict[tuple, set[str]] = defaultdict(set)
    severity_pairs: defaultdict[tuple, set[tuple[int, int]]] = defaultdict(set)
    nonfinite = {metric: 0 for metric in POST_METRICS}
    invalid_window_rows = 0
    invalid_removal_counts = 0
    rows = 0

    for chunk in pd.read_csv(args.input, usecols=usecols, chunksize=200_000):
        chunk = chunk[chunk["step_ratio"].isin(STEP_RATIOS)].copy()
        chunk["gain_scheme"] = chunk["gain_scheme"].fillna("")
        chunk["config"] = config_id(chunk)
        rows += len(chunk)

        nonzero = chunk["kill_percent"].gt(0)
        invalid_window_rows += int(
            (
                (nonzero & chunk["post_removal_steps"].ne(500))
                | (nonzero & chunk["reused_from_allocation"].ne(0))
            ).sum()
        )
        expected_kill = np.rint(
            chunk["pop"].to_numpy(float)
            * chunk["kill_percent"].to_numpy(float)
            / 100.0
        ).astype(int)
        invalid_removal_counts += int(
            (chunk["kill_number"].to_numpy(int) != expected_kill).sum()
        )
        for metric in POST_METRICS:
            nonfinite[metric] += int(
                (~np.isfinite(chunk.loc[nonzero, metric].to_numpy(float))).sum()
            )

        cell_cols = [
            "pop",
            "n",
            "step_ratio",
            "pattern",
            "kill_percent",
            "config",
        ]
        for key, count in chunk.groupby(cell_cols).size().items():
            cell_counts[key] += int(count)

        base_cols = ["pop", "n", "step_ratio", "pattern", "kill_percent", "rep"]
        for key, group in chunk.groupby(base_cols):
            pairs = set(zip(group["seed"].astype(int), group["target_path_seed"].astype(int)))
            base_seed_pairs[key].update(pairs)
            base_configs[key].update(group["config"].astype(str))

        severity_cols = ["pop", "n", "step_ratio", "pattern", "rep"]
        for key, group in chunk.groupby(severity_cols):
            severity_pairs[key].update(
                zip(group["seed"].astype(int), group["target_path_seed"].astype(int))
            )

    cell_values = np.fromiter(cell_counts.values(), dtype=int)
    bad_seed_bases = sum(len(values) != 1 for values in base_seed_pairs.values())
    bad_config_bases = sum(len(values) != 27 for values in base_configs.values())
    bad_severity_bases = sum(len(values) != 1 for values in severity_pairs.values())
    expected_rows = 4 * 3 * 3 * 4 * 7 * 27 * 100
    expected_cells = 4 * 3 * 3 * 4 * 7 * 27

    checks = {
        "reported_rows": rows,
        "expected_rows": expected_rows,
        "configuration_cells": len(cell_counts),
        "expected_configuration_cells": expected_cells,
        "min_repetitions_per_cell": int(cell_values.min()),
        "max_repetitions_per_cell": int(cell_values.max()),
        "base_repetition_keys_with_multiple_seed_pairs": bad_seed_bases,
        "base_repetition_keys_without_27_configurations": bad_config_bases,
        "repetition_keys_with_seed_changes_across_severity": bad_severity_bases,
        "invalid_nonzero_post_windows": invalid_window_rows,
        "invalid_removal_counts": invalid_removal_counts,
        **{f"nonfinite_{metric}": count for metric, count in nonfinite.items()},
    }
    failures = {
        key: value
        for key, value in checks.items()
        if (
            (key == "reported_rows" and value != expected_rows)
            or (key == "configuration_cells" and value != expected_cells)
            or (key in {"min_repetitions_per_cell", "max_repetitions_per_cell"} and value != 100)
            or (key not in {
                "reported_rows",
                "expected_rows",
                "configuration_cells",
                "expected_configuration_cells",
                "min_repetitions_per_cell",
                "max_repetitions_per_cell",
            } and value != 0)
        )
    }
    lines = ["Agent-removal grid-integrity audit"]
    lines.extend(f"{key}: {value:,}" for key, value in checks.items())
    lines.append(f"status: {'FAIL' if failures else 'PASS'}")
    if failures:
        lines.append(f"failures: {failures}")
    args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print("\n".join(lines))
    if failures:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
