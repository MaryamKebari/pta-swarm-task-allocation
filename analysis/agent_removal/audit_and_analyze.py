#!/usr/bin/env python3
"""Audit and summarize the final frozen-reference agent-removal grid.

The script reads the large per-run CSV in chunks, preserves the manuscript's
three reported step ratios, and writes condition-level means plus matched PTA
comparisons. Post-removal comparisons are refused at 0% removal unless the
row actually contains the 500-step post-event window.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import numpy as np
import pandas as pd

REPORTED_STEP_RATIOS = (1.5, 2.0, 2.5)
KEYS = [
    "pop",
    "n",
    "step_ratio",
    "pattern",
    "family",
    "threshold_range",
    "threshold_mode",
    "gain_scheme",
    "kill_percent",
]
METRICS = [
    "R",
    "R_abs",
    "R2_norm",
    "R2_max_norm",
    "post_removal_R",
    "post_removal_R_abs",
    "post_removal_R2_norm",
    "post_removal_R2_max_norm",
]
USECOLS = (
    KEYS
    + METRICS
    + [
        "rep",
        "seed",
        "target_path_seed",
        "post_removal_steps",
        "reused_from_allocation",
    ]
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input", type=Path, required=True)
    parser.add_argument("--zero-control", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--chunksize", type=int, default=250_000)
    parser.add_argument("--bootstrap", type=int, default=20_000)
    return parser.parse_args()


def config_id(frame: pd.DataFrame) -> pd.Series:
    family = frame["family"].astype(str)
    result = family + "-" + frame["threshold_range"].astype(str)
    adaptive_mode = family.isin(["SETA", "PTA"])
    result = result.where(
        ~adaptive_mode,
        result + "-" + frame["threshold_mode"].astype(str),
    )
    is_pta = family.eq("PTA")
    result = result.where(
        ~is_pta,
        result + "-" + frame["gain_scheme"].astype(str),
    )
    return result


def bootstrap_median(values: np.ndarray, draws: int, seed: int) -> tuple[float, float]:
    values = np.asarray(values, dtype=float)
    rng = np.random.default_rng(seed)
    estimates = np.empty(draws, dtype=float)
    batch = 500
    for start in range(0, draws, batch):
        stop = min(start + batch, draws)
        indices = rng.integers(0, len(values), size=(stop - start, len(values)))
        estimates[start:stop] = np.median(values[indices], axis=1)
    low, high = np.quantile(estimates, [0.025, 0.975])
    return float(low), float(high)


def aggregate_condition_means(
    paths: list[Path], chunksize: int
) -> tuple[pd.DataFrame, pd.DataFrame]:
    partials: list[pd.DataFrame] = []
    audits: list[pd.DataFrame] = []
    for path in paths:
        for chunk in pd.read_csv(path, usecols=USECOLS, chunksize=chunksize):
            chunk = chunk[chunk["step_ratio"].isin(REPORTED_STEP_RATIOS)].copy()
            chunk["gain_scheme"] = chunk["gain_scheme"].fillna("")
            chunk["config"] = config_id(chunk)

            audit = (
                chunk.groupby(
                    ["kill_percent", "post_removal_steps", "reused_from_allocation"],
                    dropna=False,
                )
                .size()
                .rename("rows")
                .reset_index()
            )
            audit["source"] = path.name
            audits.append(audit)

            valid = chunk[
                chunk["kill_percent"].gt(0) | chunk["post_removal_steps"].eq(500)
            ].copy()
            grouped = valid.groupby(KEYS + ["config"], dropna=False)
            sums = grouped[METRICS].sum().add_suffix("__sum")
            counts = grouped.size().rename("repetitions")
            partials.append(pd.concat([sums, counts], axis=1).reset_index())

    combined = pd.concat(partials, ignore_index=True)
    sum_cols = [f"{metric}__sum" for metric in METRICS]
    condition = (
        combined.groupby(KEYS + ["config"], dropna=False)[sum_cols + ["repetitions"]]
        .sum()
        .reset_index()
    )
    for metric in METRICS:
        condition[metric] = condition[f"{metric}__sum"] / condition["repetitions"]
    condition = condition.drop(columns=sum_cols)

    audit = (
        pd.concat(audits, ignore_index=True)
        .groupby(
            ["source", "kill_percent", "post_removal_steps", "reused_from_allocation"]
        )["rows"]
        .sum()
        .reset_index()
    )
    return condition, audit


def matched_comparisons(condition: pd.DataFrame) -> pd.DataFrame:
    pta = condition[condition["family"].eq("PTA")].copy()
    outputs: list[pd.DataFrame] = []
    base_condition = [
        "pop",
        "n",
        "step_ratio",
        "pattern",
        "kill_percent",
        "threshold_range",
    ]
    for comparator in ("CT", "LFTA", "SBTA", "SETA"):
        baseline = condition[condition["family"].eq(comparator)].copy()
        join = list(base_condition)
        if comparator == "SETA":
            join.append("threshold_mode")
        baseline = baseline[join + ["post_removal_R", "post_removal_R_abs"]].rename(
            columns={
                "post_removal_R": "comparator_post_removal_R",
                "post_removal_R_abs": "comparator_post_removal_R_abs",
            }
        )
        comparison = pta.merge(baseline, on=join, how="inner", validate="many_to_one")
        comparison["comparator"] = comparator
        for metric in ("post_removal_R", "post_removal_R_abs"):
            base = f"comparator_{metric}"
            comparison[f"{metric}_reduction_percent"] = 100.0 * (
                1.0 - comparison[metric] / comparison[base]
            )
            comparison[f"{metric}_pta_lower"] = comparison[metric] < comparison[base]
        outputs.append(comparison)
    return pd.concat(outputs, ignore_index=True)


def summarize_comparisons(comparisons: pd.DataFrame, draws: int) -> pd.DataFrame:
    rows: list[dict] = []
    for (comparator, kill), group in comparisons.groupby(
        ["comparator", "kill_percent"]
    ):
        cluster_keys = ["pop", "n", "step_ratio", "pattern"]
        record: dict[str, object] = {
            "comparator": comparator,
            "kill_percent": int(kill),
            "operating_conditions": group[cluster_keys].drop_duplicates().shape[0],
            "configuration_condition_comparisons": len(group),
        }
        for offset, metric in enumerate(("post_removal_R", "post_removal_R_abs")):
            # Each operating condition contributes once. The within-condition
            # median first summarizes the 12 applicable PTA configurations,
            # preventing shared demand paths from being treated as independent
            # observations in the across-condition interval.
            reductions = (
                group.groupby(cluster_keys)[f"{metric}_reduction_percent"]
                .median()
                .to_numpy(float)
            )
            low, high = bootstrap_median(
                reductions,
                draws=draws,
                seed=20260808 + int(kill) * 100 + offset * 10 + len(comparator),
            )
            record[f"{metric}_pta_lower"] = int(group[f"{metric}_pta_lower"].sum())
            record[f"{metric}_median_reduction_percent"] = float(np.median(reductions))
            record[f"{metric}_ci_low"] = low
            record[f"{metric}_ci_high"] = high
        rows.append(record)
    return pd.DataFrame(rows).sort_values(["kill_percent", "comparator"])


def summarize_pta_severity(condition: pd.DataFrame, draws: int) -> pd.DataFrame:
    data = condition[condition["family"].eq("PTA")].copy()
    rows: list[dict] = []
    for kill, group in data.groupby("kill_percent"):
        cluster_keys = ["pop", "n", "step_ratio", "pattern"]
        rec: dict[str, object] = {
            "kill_percent": int(kill),
            "operating_conditions": group[cluster_keys].drop_duplicates().shape[0],
            "configuration_conditions": len(group),
        }
        for offset, metric in enumerate(("post_removal_R", "post_removal_R_abs")):
            values = group.groupby(cluster_keys)[metric].median().to_numpy(float)
            low, high = bootstrap_median(
                values,
                draws=draws,
                seed=20260908 + int(kill) * 100 + offset,
            )
            rec[f"median_{metric}"] = float(np.median(values))
            rec[f"{metric}_ci_low"] = low
            rec[f"{metric}_ci_high"] = high
        rows.append(rec)
    return pd.DataFrame(rows).sort_values("kill_percent")


def summarize_pta_severity_by_step_ratio(
    condition: pd.DataFrame, draws: int
) -> pd.DataFrame:
    """Summarize PTA after removal without averaging over capacity regimes."""
    data = condition[condition["family"].eq("PTA")].copy()
    rows: list[dict] = []
    for (kill, step_ratio), group in data.groupby(["kill_percent", "step_ratio"]):
        cluster_keys = ["pop", "n", "pattern"]
        rec: dict[str, object] = {
            "kill_percent": int(kill),
            "step_ratio": float(step_ratio),
            "effective_step_ratio": float(step_ratio) * (1.0 - float(kill) / 100.0),
            "operating_conditions": group[cluster_keys].drop_duplicates().shape[0],
            "configuration_conditions": len(group),
        }
        for offset, metric in enumerate(("post_removal_R", "post_removal_R_abs")):
            values = group.groupby(cluster_keys)[metric].median().to_numpy(float)
            low, high = bootstrap_median(
                values,
                draws=draws,
                seed=(
                    20261108 + int(kill) * 100 + round(float(step_ratio) * 10) + offset
                ),
            )
            rec[f"median_{metric}"] = float(np.median(values))
            rec[f"{metric}_ci_low"] = low
            rec[f"{metric}_ci_high"] = high
        rows.append(rec)
    return pd.DataFrame(rows).sort_values(["kill_percent", "step_ratio"])


def summarize_comparisons_by_step_ratio(
    comparisons: pd.DataFrame, draws: int
) -> pd.DataFrame:
    """Preserve step ratio when summarizing PTA's comparator advantage."""
    rows: list[dict] = []
    grouped = comparisons.groupby(["comparator", "kill_percent", "step_ratio"])
    for (comparator, kill, step_ratio), group in grouped:
        cluster_keys = ["pop", "n", "pattern"]
        rec: dict[str, object] = {
            "comparator": comparator,
            "kill_percent": int(kill),
            "step_ratio": float(step_ratio),
            "effective_step_ratio": float(step_ratio) * (1.0 - float(kill) / 100.0),
            "operating_conditions": group[cluster_keys].drop_duplicates().shape[0],
            "configuration_condition_comparisons": len(group),
        }
        for offset, metric in enumerate(("post_removal_R", "post_removal_R_abs")):
            effects = (
                group.groupby(cluster_keys)[f"{metric}_reduction_percent"]
                .median()
                .to_numpy(float)
            )
            low, high = bootstrap_median(
                effects,
                draws=draws,
                seed=(
                    20261208
                    + int(kill) * 100
                    + round(float(step_ratio) * 10)
                    + offset * 10
                    + len(comparator)
                ),
            )
            rec[f"{metric}_median_reduction_percent"] = float(np.median(effects))
            rec[f"{metric}_ci_low"] = low
            rec[f"{metric}_ci_high"] = high
        rows.append(rec)
    return pd.DataFrame(rows).sort_values(["kill_percent", "comparator", "step_ratio"])


def summarize_strongest_removal_by_pattern(
    comparisons: pd.DataFrame, draws: int
) -> pd.DataFrame:
    data = comparisons[comparisons["kill_percent"].eq(50)].copy()
    rows: list[dict] = []
    for (comparator, pattern), group in data.groupby(["comparator", "pattern"]):
        cluster_keys = ["pop", "n", "step_ratio"]
        rec: dict[str, object] = {
            "comparator": comparator,
            "pattern": pattern,
            "operating_conditions": group[cluster_keys].drop_duplicates().shape[0],
            "configuration_condition_comparisons": len(group),
        }
        for offset, metric in enumerate(("post_removal_R", "post_removal_R_abs")):
            effects = (
                group.groupby(cluster_keys)[f"{metric}_reduction_percent"]
                .median()
                .to_numpy(float)
            )
            low, high = bootstrap_median(
                effects,
                draws=draws,
                seed=20261008 + offset * 100 + len(comparator) * 10 + len(pattern),
            )
            rec[f"{metric}_median_reduction_percent"] = float(np.median(effects))
            rec[f"{metric}_ci_low"] = low
            rec[f"{metric}_ci_high"] = high
        rows.append(rec)
    return pd.DataFrame(rows).sort_values(["comparator", "pattern"])


def degradation_relative_to_zero(condition: pd.DataFrame) -> pd.DataFrame:
    """Match every configuration-condition to its valid 0% post-event control."""
    match = [
        "pop",
        "n",
        "step_ratio",
        "pattern",
        "family",
        "threshold_range",
        "threshold_mode",
        "gain_scheme",
        "config",
    ]
    zero = condition[condition["kill_percent"].eq(0)][
        match + ["post_removal_R", "post_removal_R_abs"]
    ].rename(
        columns={
            "post_removal_R": "zero_post_removal_R",
            "post_removal_R_abs": "zero_post_removal_R_abs",
        }
    )
    data = condition.merge(zero, on=match, how="left", validate="many_to_one")
    if data[["zero_post_removal_R", "zero_post_removal_R_abs"]].isna().any().any():
        raise RuntimeError("Missing matched 0% post-event control")
    for metric in ("post_removal_R", "post_removal_R_abs"):
        data[f"{metric}_degradation_percent"] = 100.0 * (
            data[metric] / data[f"zero_{metric}"] - 1.0
        )
    return data


def summarize_pta_degradation_by_step_ratio(
    degradation: pd.DataFrame, draws: int
) -> pd.DataFrame:
    data = degradation[degradation["family"].eq("PTA")].copy()
    rows: list[dict] = []
    for (kill, step_ratio), group in data.groupby(["kill_percent", "step_ratio"]):
        cluster_keys = ["pop", "n", "pattern"]
        rec: dict[str, object] = {
            "kill_percent": int(kill),
            "step_ratio": float(step_ratio),
            "operating_conditions": group[cluster_keys].drop_duplicates().shape[0],
        }
        for offset, metric in enumerate(("post_removal_R", "post_removal_R_abs")):
            values = (
                group.groupby(cluster_keys)[f"{metric}_degradation_percent"]
                .median()
                .to_numpy(float)
            )
            low, high = bootstrap_median(
                values,
                draws=draws,
                seed=20261308 + int(kill) * 100 + int(step_ratio * 10) + offset,
            )
            rec[f"median_{metric}_degradation_percent"] = float(np.median(values))
            rec[f"{metric}_degradation_ci_low"] = low
            rec[f"{metric}_degradation_ci_high"] = high
        rows.append(rec)
    return pd.DataFrame(rows).sort_values(["kill_percent", "step_ratio"])


def main() -> None:
    args = parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    paths = [args.input]
    if args.zero_control is not None:
        paths.append(args.zero_control)
    condition, audit = aggregate_condition_means(paths, args.chunksize)
    condition.to_csv(args.output / "condition_means.csv", index=False)
    audit.to_csv(args.output / "post_window_audit.csv", index=False)

    comparisons = matched_comparisons(condition)
    comparisons.to_csv(args.output / "pta_matched_comparisons.csv", index=False)
    summary = summarize_comparisons(comparisons, args.bootstrap)
    summary.to_csv(args.output / "pta_comparator_summary.csv", index=False)
    severity = summarize_pta_severity(condition, args.bootstrap)
    severity.to_csv(args.output / "pta_post_removal_severity.csv", index=False)
    severity_by_step = summarize_pta_severity_by_step_ratio(condition, args.bootstrap)
    severity_by_step.to_csv(
        args.output / "pta_post_removal_severity_by_step_ratio.csv", index=False
    )
    comparison_by_step = summarize_comparisons_by_step_ratio(
        comparisons, args.bootstrap
    )
    comparison_by_step.to_csv(
        args.output / "pta_comparator_summary_by_step_ratio.csv", index=False
    )
    pattern_summary = summarize_strongest_removal_by_pattern(
        comparisons, args.bootstrap
    )
    pattern_summary.to_csv(
        args.output / "pta_comparator_by_pattern_at_50pct.csv", index=False
    )

    degradation = degradation_relative_to_zero(condition)
    degradation.to_csv(args.output / "degradation_relative_to_zero.csv", index=False)
    degradation_summary = summarize_pta_degradation_by_step_ratio(
        degradation, args.bootstrap
    )
    degradation_summary.to_csv(
        args.output / "pta_degradation_by_step_ratio.csv", index=False
    )

    zero = audit[audit["kill_percent"].eq(0)]
    zero_valid = int(zero.loc[zero["post_removal_steps"].eq(500), "rows"].sum())
    zero_invalid = int(zero.loc[zero["post_removal_steps"].ne(500), "rows"].sum())
    expected_zero = 4 * 3 * 3 * 4 * 27 * 100
    report = [
        "Frozen-reference agent-removal audit",
        f"Reported-grid condition means: {len(condition):,}",
        f"Valid 0% post-removal rows: {zero_valid:,}",
        f"Invalid/reused 0% rows without the 500-step window: {zero_invalid:,}",
        f"Expected corrected 0% rows: {expected_zero:,}",
        "",
        "Post-window audit:",
        audit.to_string(index=False),
        "",
        "PTA post-removal severity:",
        severity.to_string(index=False),
        "",
        "PTA post-removal severity by step ratio:",
        severity_by_step.to_string(index=False),
        "",
        "Matched PTA comparisons by step ratio:",
        comparison_by_step.to_string(index=False),
        "",
        "Matched PTA comparisons:",
        summary.to_string(index=False),
        "",
        "Strongest-removal comparisons by demand class:",
        pattern_summary.to_string(index=False),
        "",
        "PTA degradation relative to matched 0% post-event controls:",
        degradation_summary.to_string(index=False),
    ]
    (args.output / "audit_report.txt").write_text("\n".join(report) + "\n")
    print("\n".join(report))


if __name__ == "__main__":
    main()
