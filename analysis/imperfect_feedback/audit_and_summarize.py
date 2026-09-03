#!/usr/bin/env python3
"""Audit and summarize the final frozen-reference imperfect-feedback grid.

The script intentionally validates the CSV itself rather than trusting only
the accompanying manifest. It writes compact audit and summary artifacts that
can be reviewed before any result is added to the manuscript.
"""

from __future__ import annotations

import json
import os
from pathlib import Path

import numpy as np
import pandas as pd


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "data" / "processed" / "imperfect_feedback"
RAW_ROOT = ROOT / "data" / "raw"
NOISE_CSV = Path(
    os.environ.get(
        "PTA_FEEDBACK_CSV",
        RAW_ROOT / "imperfect_feedback" / "per_run_results.csv",
    )
)
ALLOCATION_CSV = Path(
    os.environ.get(
        "PTA_ALLOCATION_CSV",
        RAW_ROOT / "allocation_grid" / "per_run_results.csv",
    )
)

EXPECTED_ROWS = 1_296_000
EXPECTED_CLEAN = 64_800
EXPECTED_PERTURBED = 1_231_200
EXPECTED_METHODS = 27
EXPECTED_REPS = 100
EXPECTED_ALPHAS = {0.0, 0.05, 0.1, 0.2, 0.4}
EXPECTED_BETAS = {0.0, 0.05, 0.1, 0.2}
EXPECTED_PATTERNS = {"random", "sharp", "scurve", "zigzag"}
EXPECTED_STEPS = {1.5, 2.0, 2.5}
EXPECTED_TASKS = {4, 12}

CONFIG_COLS = ["method_label", "threshold_mode", "threshold_range", "gain_scheme"]

ID_COLS = [
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
    "rep",
]
METRIC_COLS = ["R", "R_abs", "R2", "R2_norm", "R2_max_norm", "avg_switch_noidle"]
SEED_COLS = ["seed", "target_path_seed", "feedback_noise_seed", "feedback_bias_seed"]
PERTURB_COLS = [
    "feedback_noise_alpha",
    "feedback_bias_alpha",
    "feedback_noise_sigma_mode",
    "feedback_noise_clip",
    "feedback_bias_mode",
    "reused_from_allocation",
]
USECOLS = ID_COLS + METRIC_COLS + SEED_COLS + PERTURB_COLS


def require(condition: bool, message: str) -> None:
    if not condition:
        raise RuntimeError(message)


def normalized_keys(frame: pd.DataFrame, columns: list[str]) -> pd.DataFrame:
    result = frame.copy()
    for column in columns:
        if result[column].dtype == object:
            result[column] = result[column].fillna("")
    return result


def max_nunique(frame: pd.DataFrame, group: list[str], value: str) -> int:
    return int(frame.groupby(group, observed=True)[value].nunique(dropna=False).max())


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    frame = pd.read_csv(NOISE_CSV, usecols=USECOLS, low_memory=False)

    require(len(frame) == EXPECTED_ROWS, f"rows: expected {EXPECTED_ROWS}, got {len(frame)}")
    require(set(frame["pop"].unique()) == {500}, "unexpected population values")
    require(set(frame["n"].unique()) == EXPECTED_TASKS, "unexpected task counts")
    require(set(frame["step_ratio"].unique()) == EXPECTED_STEPS, "unexpected step ratios")
    require(set(frame["pattern"].unique()) == EXPECTED_PATTERNS, "unexpected demand classes")
    require(set(frame["feedback_noise_alpha"].unique()) == EXPECTED_ALPHAS, "unexpected noise levels")
    require(set(frame["feedback_bias_alpha"].unique()) == EXPECTED_BETAS, "unexpected bias levels")
    configuration_count = len(frame[CONFIG_COLS].fillna("").drop_duplicates())
    require(configuration_count == EXPECTED_METHODS, f"expected 27 configurations, got {configuration_count}")
    require(set(frame["rep"].unique()) == set(range(EXPECTED_REPS)), "repetition set is incomplete")
    require(set(frame["feedback_noise_clip"].unique()) == {0}, "feedback clipping is present")

    clean_mask = (frame["feedback_noise_alpha"] == 0) & (frame["feedback_bias_alpha"] == 0)
    clean = frame.loc[clean_mask].copy()
    perturbed = frame.loc[~clean_mask].copy()
    require(len(clean) == EXPECTED_CLEAN, f"clean rows: expected {EXPECTED_CLEAN}, got {len(clean)}")
    require(
        len(perturbed) == EXPECTED_PERTURBED,
        f"perturbed rows: expected {EXPECTED_PERTURBED}, got {len(perturbed)}",
    )
    require(set(clean["reused_from_allocation"].unique()) == {1}, "clean rows were not all reused")
    require(set(perturbed["reused_from_allocation"].unique()) == {0}, "perturbed rows marked reused")

    group_cols = [
        "n",
        "step_ratio",
        "pattern",
        *CONFIG_COLS,
        "feedback_noise_alpha",
        "feedback_bias_alpha",
    ]
    group_sizes = frame.groupby(group_cols, observed=True, dropna=False).size()
    require(set(group_sizes.unique()) == {EXPECTED_REPS}, "not every configuration cell has 100 repetitions")
    require(len(group_sizes) == 12_960, f"expected 12,960 cells, got {len(group_sizes)}")

    combo_count = frame.groupby(
        ["n", "step_ratio", "pattern", *CONFIG_COLS, "rep"], observed=True, dropna=False
    ).size()
    require(set(combo_count.unique()) == {20}, "not every matched run has all 20 perturbation pairs")

    metric_values = frame[METRIC_COLS].to_numpy(dtype=float)
    require(np.isfinite(metric_values).all(), "non-finite metric values found")
    require((metric_values >= 0).all(), "negative metric values found")

    base_rep = ["n", "step_ratio", "pattern", "rep"]
    seed_audit = {
        "target_path_seed_max_nunique": max_nunique(frame, base_rep, "target_path_seed"),
        "feedback_noise_seed_max_nunique": max_nunique(frame, base_rep, "feedback_noise_seed"),
        "feedback_bias_seed_max_nunique": max_nunique(frame, base_rep, "feedback_bias_seed"),
        "simulation_seed_max_nunique": max_nunique(frame, base_rep, "seed"),
    }
    require(seed_audit["target_path_seed_max_nunique"] == 1, "target paths are not matched")
    require(seed_audit["feedback_noise_seed_max_nunique"] == 1, "noise draws are not matched")
    require(seed_audit["feedback_bias_seed_max_nunique"] == 1, "bias signs are not matched")

    # Verify that reused clean rows are identical to the allocation grid.
    match_keys = [
        "pop",
        "n",
        "step_ratio",
        "pattern",
        "method_label",
        "threshold_mode",
        "threshold_range",
        "gain_scheme",
        "rep",
    ]
    allocation = pd.read_csv(
        ALLOCATION_CSV,
        usecols=match_keys + METRIC_COLS + ["seed", "target_path_seed"],
        low_memory=False,
    )
    allocation = allocation[
        allocation["pop"].eq(500)
        & allocation["n"].isin(EXPECTED_TASKS)
        & allocation["step_ratio"].isin(EXPECTED_STEPS)
        & allocation["pattern"].isin(EXPECTED_PATTERNS)
    ].copy()
    clean_for_merge = normalized_keys(clean[match_keys + METRIC_COLS + ["seed", "target_path_seed"]], match_keys)
    allocation_for_merge = normalized_keys(allocation, match_keys)
    merged = clean_for_merge.merge(
        allocation_for_merge,
        on=match_keys,
        how="outer",
        suffixes=("_noise", "_allocation"),
        indicator=True,
        validate="one_to_one",
    )
    require(len(merged) == EXPECTED_CLEAN, "clean/allocation merge size mismatch")
    require(set(merged["_merge"].unique()) == {"both"}, "clean/allocation keys do not match")
    clean_metric_max_abs_diff = {
        metric: float(np.max(np.abs(merged[f"{metric}_noise"] - merged[f"{metric}_allocation"])))
        for metric in METRIC_COLS
    }
    require(max(clean_metric_max_abs_diff.values()) < 1e-12, "clean metrics differ from allocation grid")
    require((merged["seed_noise"] == merged["seed_allocation"]).all(), "clean simulation seeds differ")
    require(
        (merged["target_path_seed_noise"] == merged["target_path_seed_allocation"]).all(),
        "clean target-path seeds differ",
    )

    method_counts = (
        frame.groupby(["family", "method_label", "threshold_mode", "threshold_range", "gain_scheme"], dropna=False)
        .size()
        .rename("rows")
        .reset_index()
        .sort_values(["family", "method_label"])
    )
    method_counts.to_csv(OUT / "configuration_counts.csv", index=False)

    severity_counts = (
        frame.groupby(["feedback_noise_alpha", "feedback_bias_alpha"], observed=True)
        .size()
        .rename("rows")
        .reset_index()
    )
    severity_counts.to_csv(OUT / "severity_counts.csv", index=False)

    # Paired degradation relative to the reused clean control for each exact
    # configuration, operating condition, and repetition.
    clean_keys = [
        "n",
        "step_ratio",
        "pattern",
        "method_label",
        "threshold_mode",
        "threshold_range",
        "gain_scheme",
        "rep",
    ]
    work = normalized_keys(frame[clean_keys + ["family"] + METRIC_COLS + PERTURB_COLS], clean_keys)
    clean_values = work.loc[
        work["feedback_noise_alpha"].eq(0) & work["feedback_bias_alpha"].eq(0),
        clean_keys + METRIC_COLS,
    ].rename(columns={metric: f"clean_{metric}" for metric in METRIC_COLS})
    work = work.merge(clean_values, on=clean_keys, how="left", validate="many_to_one")
    for metric in ["R", "R_abs", "R2_norm", "R2_max_norm"]:
        work[f"{metric}_change_percent"] = 100.0 * (
            work[metric] / work[f"clean_{metric}"] - 1.0
        )

    family_degradation = (
        work.groupby(["family", "feedback_noise_alpha", "feedback_bias_alpha"], observed=True)
        .agg(
            comparisons=("R_change_percent", "size"),
            median_R_change_percent=("R_change_percent", "median"),
            q25_R_change_percent=("R_change_percent", lambda x: x.quantile(0.25)),
            q75_R_change_percent=("R_change_percent", lambda x: x.quantile(0.75)),
            median_R_abs_change_percent=("R_abs_change_percent", "median"),
            median_R2_norm_change_percent=("R2_norm_change_percent", "median"),
            median_R2_max_norm_change_percent=("R2_max_norm_change_percent", "median"),
        )
        .reset_index()
    )
    family_degradation.to_csv(OUT / "family_degradation_vs_clean.csv", index=False)

    # Condition-level means are compact enough for the final inferential and
    # plotting script, while preserving exact configuration identities.
    condition_means = (
        frame.groupby(
            [
                "n",
                "step_ratio",
                "pattern",
                "family",
                "method_label",
                "threshold_mode",
                "threshold_range",
                "gain_scheme",
                "feedback_noise_alpha",
                "feedback_bias_alpha",
            ],
            dropna=False,
            observed=True,
        )[METRIC_COLS]
        .mean()
        .reset_index()
    )
    condition_means.to_csv(OUT / "condition_means.csv", index=False)

    report = {
        "status": "PASS",
        "rows": len(frame),
        "clean_rows": len(clean),
        "perturbed_rows": len(perturbed),
        "configurations": configuration_count,
        "cells": len(group_sizes),
        "repetitions_per_cell": sorted(int(value) for value in group_sizes.unique()),
        "noise_levels": sorted(float(value) for value in frame["feedback_noise_alpha"].unique()),
        "bias_levels": sorted(float(value) for value in frame["feedback_bias_alpha"].unique()),
        "feedback_noise_clip_values": sorted(int(value) for value in frame["feedback_noise_clip"].unique()),
        "noise_sigma_modes": sorted(str(value) for value in frame["feedback_noise_sigma_mode"].unique()),
        "bias_modes": sorted(str(value) for value in frame["feedback_bias_mode"].unique()),
        "seed_audit": seed_audit,
        "clean_metric_max_abs_diff": clean_metric_max_abs_diff,
        "metric_ranges": {
            metric: [float(frame[metric].min()), float(frame[metric].max())] for metric in METRIC_COLS
        },
    }
    (OUT / "audit_report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
