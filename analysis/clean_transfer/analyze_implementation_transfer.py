#!/usr/bin/env python3
"""Analyze transferred gain schemes and stored-threshold modes.

The input is the completed clean frozen-reference allocation grid. Bootstrap
resampling uses the 144 operating conditions as clusters and retains all
associated modes or gain schemes.
"""

from __future__ import annotations

import os
from pathlib import Path

import numpy as np
import pandas as pd


REPO_ROOT = Path(__file__).resolve().parents[2]
HERE = REPO_ROOT / "data" / "processed" / "clean_transfer"
INPUT = Path(
    os.environ.get(
        "PTA_ALLOCATION_CSV",
        REPO_ROOT / "data" / "raw" / "allocation_grid" / "per_run_results.csv",
    )
)
CONDITION = ["pop", "n", "step_ratio", "pattern"]
METRICS = {
    "R": "R",
    "R_abs": "R_abs",
}
RANGES = ("HM", "HT1", "HT2")
MODES = ("clamped", "latent")
GAINS = ("Agent", "Global")
BOOTSTRAP_DRAWS = 20_000


def reduction(log_ratio: float) -> float:
    """Return percentage reduction of the numerator relative to denominator."""
    return 100.0 * (1.0 - np.exp(log_ratio))


def clustered_interval(
    frame: pd.DataFrame,
    *,
    value: str,
    columns: list[str],
    seed: int,
) -> tuple[float, float, float]:
    """Bootstrap conditions while retaining associated configuration effects."""
    matrix = (
        frame.pivot(index=CONDITION, columns=columns, values=value)
        .sort_index(axis=1)
        .to_numpy(float)
    )
    if matrix.shape[0] != 144 or np.isnan(matrix).any():
        raise RuntimeError(
            f"Expected 144 complete condition clusters for {value}, found {matrix.shape}"
        )
    rng = np.random.default_rng(seed)
    estimates = np.empty(BOOTSTRAP_DRAWS, dtype=float)
    batch = 500
    for start in range(0, BOOTSTRAP_DRAWS, batch):
        stop = min(start + batch, BOOTSTRAP_DRAWS)
        indices = rng.integers(
            0, matrix.shape[0], size=(stop - start, matrix.shape[0])
        )
        estimates[start:stop] = np.median(
            matrix[indices].reshape(stop - start, -1), axis=1
        )
    median = float(np.median(matrix))
    low, high = np.quantile(estimates, [0.025, 0.975])
    return median, float(low), float(high)


def pair_effects(
    left: pd.DataFrame,
    right: pd.DataFrame,
    *,
    match: list[str],
    left_label: str,
    right_label: str,
) -> pd.DataFrame:
    left_columns = match + list(METRICS.values())
    right_columns = match + list(METRICS.values())
    paired = left[left_columns].merge(
        right[right_columns],
        on=match,
        how="inner",
        validate="one_to_one",
        suffixes=(f"_{left_label}", f"_{right_label}"),
    )
    for label, metric in METRICS.items():
        numerator = paired[f"{metric}_{left_label}"].to_numpy(float)
        denominator = paired[f"{metric}_{right_label}"].to_numpy(float)
        if np.any(numerator <= 0) or np.any(denominator <= 0):
            raise RuntimeError(f"Non-positive values prevent log ratios for {label}")
        paired[f"{label}_log_ratio"] = np.log(numerator / denominator)
    return paired


def summarize_gain_transfer(pta: pd.DataFrame) -> tuple[pd.DataFrame, pd.DataFrame]:
    match = CONDITION + ["threshold_range", "threshold_mode"]
    effects = pair_effects(
        pta[pta["gain_scheme"].eq("Agent")],
        pta[pta["gain_scheme"].eq("Global")],
        match=match,
        left_label="agent",
        right_label="global",
    )
    if len(effects) != 864:
        raise RuntimeError(f"Expected 864 gain effects, found {len(effects)}")

    rows: list[dict[str, object]] = []
    groups = [("All", effects)] + [
        (threshold_range, effects[effects["threshold_range"].eq(threshold_range)])
        for threshold_range in RANGES
    ]
    for range_index, (threshold_range, group) in enumerate(groups):
        record: dict[str, object] = {
            "threshold_range": threshold_range,
            "comparisons": len(group),
        }
        bootstrap_columns = (
            ["threshold_range", "threshold_mode"]
            if threshold_range == "All"
            else ["threshold_mode"]
        )
        for metric_index, label in enumerate(METRICS):
            value = f"{label}_log_ratio"
            median, low, high = clustered_interval(
                group,
                value=value,
                columns=bootstrap_columns,
                seed=20260821 + range_index * 100 + metric_index,
            )
            record[f"{label}_agent_lower"] = int((group[value] < 0).sum())
            record[f"{label}_reduction_percent"] = reduction(median)
            record[f"{label}_ci_low"] = reduction(high)
            record[f"{label}_ci_high"] = reduction(low)
        rows.append(record)
    return effects, pd.DataFrame(rows)


def summarize_seta_storage(
    seta: pd.DataFrame,
) -> tuple[pd.DataFrame, pd.DataFrame]:
    match = CONDITION + ["threshold_range"]
    effects = pair_effects(
        seta[seta["threshold_mode"].eq("latent")],
        seta[seta["threshold_mode"].eq("clamped")],
        match=match,
        left_label="latent",
        right_label="clamped",
    )
    if len(effects) != 432:
        raise RuntimeError(f"Expected 432 SETA storage effects, found {len(effects)}")

    rows: list[dict[str, object]] = []
    for range_index, threshold_range in enumerate(RANGES):
        group = effects[effects["threshold_range"].eq(threshold_range)]
        record: dict[str, object] = {
            "threshold_range": threshold_range,
            "comparisons": len(group),
        }
        for metric_index, label in enumerate(METRICS):
            value = f"{label}_log_ratio"
            median, low, high = clustered_interval(
                group,
                value=value,
                columns=["threshold_range"],
                seed=20261821 + range_index * 100 + metric_index,
            )
            record[f"{label}_latent_lower"] = int((group[value] < 0).sum())
            record[f"{label}_reduction_percent"] = reduction(median)
            record[f"{label}_ci_low"] = reduction(high)
            record[f"{label}_ci_high"] = reduction(low)
        rows.append(record)
    return effects, pd.DataFrame(rows)


def summarize_pta_storage(pta: pd.DataFrame) -> tuple[pd.DataFrame, pd.DataFrame]:
    match = CONDITION + ["threshold_range", "gain_scheme"]
    effects = pair_effects(
        pta[pta["threshold_mode"].eq("latent")],
        pta[pta["threshold_mode"].eq("clamped")],
        match=match,
        left_label="latent",
        right_label="clamped",
    )
    if len(effects) != 864:
        raise RuntimeError(f"Expected 864 PTA storage effects, found {len(effects)}")

    rows: list[dict[str, object]] = []
    for range_index, threshold_range in enumerate(RANGES):
        group = effects[effects["threshold_range"].eq(threshold_range)]
        record: dict[str, object] = {
            "threshold_range": threshold_range,
            "comparisons": len(group),
        }
        for metric_index, label in enumerate(METRICS):
            value = f"{label}_log_ratio"
            median, low, high = clustered_interval(
                group,
                value=value,
                columns=["gain_scheme"],
                seed=20262821 + range_index * 100 + metric_index,
            )
            record[f"{label}_latent_lower"] = int((group[value] < 0).sum())
            record[f"{label}_reduction_percent"] = reduction(median)
            record[f"{label}_ci_low"] = reduction(high)
            record[f"{label}_ci_high"] = reduction(low)
        rows.append(record)
    return effects, pd.DataFrame(rows)


def main() -> None:
    columns = CONDITION + [
        "family",
        "threshold_range",
        "threshold_mode",
        "gain_scheme",
        "run_method_id",
        "rep",
        "seed",
        "target_path_seed",
        *METRICS.values(),
    ]
    runs = pd.read_csv(INPUT, usecols=columns)
    runs = runs[runs["step_ratio"].isin((1.5, 2.0, 2.5))].copy()
    if len(runs) != 388_800:
        raise RuntimeError(f"Expected 388,800 clean runs, found {len(runs)}")
    config = [
        "family",
        "threshold_range",
        "threshold_mode",
        "gain_scheme",
        "run_method_id",
    ]
    counts = runs.groupby(CONDITION + config, dropna=False).agg(
        repetitions=("rep", "nunique"),
        simulation_seeds=("seed", "nunique"),
        target_path_seeds=("target_path_seed", "nunique"),
    )
    if not (counts == 100).all().all():
        raise RuntimeError("Every configuration-condition must contain 100 matched repetitions")
    data = (
        runs.groupby(CONDITION + config, dropna=False)[list(METRICS.values())]
        .mean()
        .reset_index()
    )
    if len(data) != 3_888:
        raise RuntimeError(f"Expected 3,888 clean configuration means, found {len(data)}")
    if data[CONDITION].drop_duplicates().shape[0] != 144:
        raise RuntimeError("Expected 144 operating conditions")

    pta = data[data["family"].eq("PTA")].copy()
    seta = data[data["family"].eq("SETA")].copy()
    if set(pta["threshold_mode"]) != set(MODES) or set(pta["gain_scheme"]) != set(GAINS):
        raise RuntimeError("PTA gain/mode grid is incomplete")
    if set(seta["threshold_mode"]) != set(MODES):
        raise RuntimeError("SETA mode grid is incomplete")

    gain_effects, gain_summary = summarize_gain_transfer(pta)
    seta_effects, seta_summary = summarize_seta_storage(seta)
    pta_storage_effects, pta_storage_summary = summarize_pta_storage(pta)

    gain_effects.to_csv(HERE / "gain_transfer_condition_effects.csv", index=False)
    gain_summary.to_csv(HERE / "gain_transfer_summary.csv", index=False)
    seta_effects.to_csv(HERE / "seta_storage_transfer_condition_effects.csv", index=False)
    seta_summary.to_csv(HERE / "seta_storage_transfer_summary.csv", index=False)
    pta_storage_effects.to_csv(HERE / "pta_storage_transfer_condition_effects.csv", index=False)
    pta_storage_summary.to_csv(HERE / "pta_storage_transfer_summary.csv", index=False)

    report = [
        "Frozen-reference implementation-choice transfer analysis",
        "Analysis window: full 1,000-step clean allocation run",
        "Bootstrap unit: 144 operating conditions; associated modes/gains retained",
        "",
        "Agent versus Global PTA (positive reduction favors Agent):",
        gain_summary.to_string(index=False),
        "",
        "Latent versus clamped SETA (positive reduction favors latent):",
        seta_summary.to_string(index=False),
        "",
        "Latent versus clamped PTA (positive reduction favors latent):",
        pta_storage_summary.to_string(index=False),
    ]
    (HERE / "implementation_transfer_audit.txt").write_text("\n".join(report) + "\n")
    print("\n".join(report))


if __name__ == "__main__":
    main()
