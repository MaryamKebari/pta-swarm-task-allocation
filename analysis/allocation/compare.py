#!/usr/bin/env python3
"""Analyze the completed clean frozen-reference allocation grid.

Each operating condition contributes the lowest-R configuration from each
method family. Confidence intervals resample the 144 operating conditions and
retain the paired family comparison within every resampled condition.
"""

from __future__ import annotations

import os
from pathlib import Path

import numpy as np
import pandas as pd
from scipy import stats

REPO_ROOT = Path(__file__).resolve().parents[2]
HERE = REPO_ROOT / "data" / "processed" / "allocation"
INPUT = Path(
    os.environ.get(
        "PTA_ALLOCATION_CSV",
        REPO_ROOT / "data" / "raw" / "allocation" / "per_run_results.csv",
    )
)
CONDITION = ["pop", "n", "step_ratio", "pattern"]
RANGE_CONDITION = CONDITION + ["threshold_range"]
COMPARATORS = ("CT", "LFTA", "SBTA", "SETA")
METRICS = ("R", "R_abs")
BOOTSTRAP_DRAWS = 20_000


def bh_adjust(p_values: np.ndarray) -> np.ndarray:
    """Apply Benjamini-Hochberg correction to one declared test family."""
    values = np.asarray(p_values, dtype=float)
    order = np.argsort(values)
    ranked = values[order] * len(values) / np.arange(1, len(values) + 1)
    ranked = np.minimum.accumulate(ranked[::-1])[::-1]
    adjusted = np.empty_like(ranked)
    adjusted[order] = np.minimum(ranked, 1.0)
    return adjusted


def bootstrap_median_log_ratio(
    values: np.ndarray, *, draws: int, seed: int
) -> tuple[float, float, float]:
    values = np.asarray(values, dtype=float)
    rng = np.random.default_rng(seed)
    estimates = np.empty(draws, dtype=float)
    batch = 500
    for start in range(0, draws, batch):
        stop = min(start + batch, draws)
        indices = rng.integers(0, len(values), size=(stop - start, len(values)))
        estimates[start:stop] = np.median(values[indices], axis=1)
    median = float(np.median(values))
    low, high = np.quantile(estimates, [0.025, 0.975])
    return median, float(low), float(high)


def bootstrap_range_cluster_median(
    frame: pd.DataFrame, *, value: str, draws: int, seed: int
) -> tuple[float, float, float]:
    """Resample operating conditions while retaining their three ranges."""
    matrix = (
        frame.pivot(index=CONDITION, columns="threshold_range", values=value)
        .sort_index(axis=1)
        .to_numpy(float)
    )
    if matrix.shape != (144, 3) or np.isnan(matrix).any():
        raise RuntimeError(
            f"Expected a complete 144 by 3 range matrix, found {matrix.shape}"
        )
    rng = np.random.default_rng(seed)
    estimates = np.empty(draws, dtype=float)
    batch = 500
    for start in range(0, draws, batch):
        stop = min(start + batch, draws)
        indices = rng.integers(0, matrix.shape[0], size=(stop - start, matrix.shape[0]))
        estimates[start:stop] = np.median(
            matrix[indices].reshape(stop - start, -1), axis=1
        )
    median = float(np.median(matrix))
    low, high = np.quantile(estimates, [0.025, 0.975])
    return median, float(low), float(high)


def bootstrap_condition_cluster_median(
    frame: pd.DataFrame,
    *,
    value: str,
    expected_variants: int,
    draws: int,
    seed: int,
) -> tuple[float, float, float]:
    """Resample operating conditions while retaining fixed PTA variants."""
    matrix = (
        frame.pivot(index=CONDITION, columns="run_method_id", values=value)
        .sort_index(axis=1)
        .to_numpy(float)
    )
    if matrix.shape != (144, expected_variants) or np.isnan(matrix).any():
        raise RuntimeError(
            f"Expected a complete 144 by {expected_variants} PTA-variant matrix, "
            f"found {matrix.shape}"
        )
    rng = np.random.default_rng(seed)
    estimates = np.empty(draws, dtype=float)
    batch = 500
    for start in range(0, draws, batch):
        stop = min(start + batch, draws)
        indices = rng.integers(0, matrix.shape[0], size=(stop - start, matrix.shape[0]))
        estimates[start:stop] = np.median(
            matrix[indices].reshape(stop - start, -1), axis=1
        )
    median = float(np.median(matrix))
    low, high = np.quantile(estimates, [0.025, 0.975])
    return median, float(low), float(high)


def reduction(log_ratio: float) -> float:
    return 100.0 * (1.0 - np.exp(log_ratio))


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
        *METRICS,
    ]
    runs = pd.read_csv(INPUT, usecols=columns)
    runs = runs[runs["step_ratio"].isin((1.5, 2.0, 2.5))].copy()
    if len(runs) != 388_800:
        raise RuntimeError(f"Expected 388,800 clean runs, found {len(runs):,}")

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
        raise RuntimeError(
            "Every clean configuration-condition must have 100 matched repetitions"
        )
    data = (
        runs.groupby(CONDITION + config, dropna=False)[list(METRICS)]
        .mean()
        .reset_index()
    )

    if len(data) != 3_888:
        raise RuntimeError(f"Expected 3,888 configuration means, found {len(data):,}")
    if data[CONDITION].drop_duplicates().shape[0] != 144:
        raise RuntimeError("Clean transfer grid does not contain 144 conditions")

    # Parameters are already frozen. Selecting the lowest-R configuration within
    # each family gives every family the same condition-level oracle treatment.
    selected = data.loc[data.groupby(CONDITION + ["family"])["R"].idxmin()].copy()
    winners = selected.loc[selected.groupby(CONDITION)["R"].idxmin()].copy()

    winner_counts = (
        winners.groupby(["family", "run_method_id"])
        .size()
        .rename("conditions_won")
        .reset_index()
    )
    winner_counts.to_csv(HERE / "winner_counts.csv", index=False)

    pta = selected[selected["family"].eq("PTA")].set_index(CONDITION)
    rows: list[dict[str, object]] = []
    for comparator_index, comparator in enumerate(COMPARATORS):
        baseline = selected[selected["family"].eq(comparator)].set_index(CONDITION)
        paired = pta[list(METRICS)].join(
            baseline[list(METRICS)], lsuffix="_pta", rsuffix="_comparator"
        )
        record: dict[str, object] = {
            "comparator": comparator,
            "operating_conditions": len(paired),
        }
        for metric_index, metric in enumerate(METRICS):
            pta_values = paired[f"{metric}_pta"].to_numpy(float)
            baseline_values = paired[f"{metric}_comparator"].to_numpy(float)
            log_ratios = np.log(pta_values / baseline_values)
            median, low, high = bootstrap_median_log_ratio(
                log_ratios,
                draws=BOOTSTRAP_DRAWS,
                seed=20260809 + comparator_index * 100 + metric_index,
            )
            prefix = metric
            record[f"{prefix}_pta_lower"] = int(np.sum(log_ratios < 0))
            record[f"{prefix}_reduction_percent"] = reduction(median)
            # The reduction transform reverses the interval endpoints.
            record[f"{prefix}_ci_low"] = reduction(high)
            record[f"{prefix}_ci_high"] = reduction(low)
        rows.append(record)

    summary = pd.DataFrame(rows)
    summary.to_csv(HERE / "family_comparison_summary.csv", index=False)

    # Summarize how the best frozen PTA family's advantage changes with each
    # experimental factor. These summaries use one value per operating
    # condition: the lowest-R PTA configuration and the lowest-R non-PTA
    # configuration. The log ratio keeps relative improvements symmetric.
    best_alternative = (
        selected[~selected["family"].eq("PTA")]
        .loc[lambda frame: frame.groupby(CONDITION)["R"].idxmin()]
        .set_index(CONDITION)
    )
    condition_effects = (
        pta[["R"]]
        .join(
            best_alternative[["R", "family"]],
            lsuffix="_pta",
            rsuffix="_alternative",
        )
        .reset_index()
    )
    condition_effects["log_ratio"] = np.log(
        condition_effects["R_pta"] / condition_effects["R_alternative"]
    )
    condition_effects["reduction_percent"] = 100.0 * (
        1.0 - condition_effects["R_pta"] / condition_effects["R_alternative"]
    )

    pattern_names = {
        "random": "Non-iterative gradual",
        "sharp": "Non-iterative non-gradual",
        "scurve": "Iterative gradual",
        "zigzag": "Iterative non-gradual",
    }
    factor_rows: list[dict[str, object]] = []
    for factor, label in [
        ("pop", "Population"),
        ("n", "Tasks"),
        ("step_ratio", "Step ratio"),
        ("pattern", "Demand class"),
    ]:
        for level, group in condition_effects.groupby(factor, sort=True):
            factor_rows.append(
                {
                    "factor": label,
                    "level": pattern_names.get(level, level),
                    "operating_conditions": len(group),
                    "median_best_pta_R": float(group["R_pta"].median()),
                    "median_best_alternative_R": float(group["R_alternative"].median()),
                    "median_pta_reduction_percent": reduction(
                        float(group["log_ratio"].median())
                    ),
                }
            )
    pd.DataFrame(factor_rows).to_csv(HERE / "factor_effect_summary.csv", index=False)

    # Paired endpoint checks distinguish changes in PTA's absolute error from
    # changes in its relative advantage over the best alternative.
    endpoint_rows: list[dict[str, object]] = []
    for factor, low, high in [
        ("pop", 50, 1000),
        ("n", 4, 12),
        ("step_ratio", 1.5, 2.5),
    ]:
        match_keys = [key for key in CONDITION if key != factor]
        endpoints = condition_effects[condition_effects[factor].isin([low, high])]
        pta_wide = endpoints.pivot(index=match_keys, columns=factor, values="R_pta")
        advantage_wide = endpoints.pivot(
            index=match_keys, columns=factor, values="reduction_percent"
        )
        endpoint_rows.append(
            {
                "factor": factor,
                "low": low,
                "high": high,
                "matched_conditions": len(pta_wide),
                "pta_R_lower_at_high": int((pta_wide[high] < pta_wide[low]).sum()),
                "median_pta_R_ratio_high_to_low": float(
                    np.median(pta_wide[high] / pta_wide[low])
                ),
                "advantage_larger_at_high": int(
                    (advantage_wide[high] > advantage_wide[low]).sum()
                ),
                "median_advantage_change_percentage_points": float(
                    np.median(advantage_wide[high] - advantage_wide[low])
                ),
            }
        )
    pd.DataFrame(endpoint_rows).to_csv(
        HERE / "factor_endpoint_summary.csv", index=False
    )

    # Range-matched comparisons keep HM, HT1, and HT2 as explicit matched
    # settings. Within a range, the lowest-R applicable mode/gain variant is
    # selected for each family. Bootstrap resampling occurs at the operating-
    # condition level and retains all three ranges in each sampled cluster.
    selected_by_range = data.loc[
        data.groupby(RANGE_CONDITION + ["family"])["R"].idxmin()
    ].copy()
    pta_by_range = selected_by_range[selected_by_range["family"].eq("PTA")].set_index(
        RANGE_CONDITION
    )
    range_rows: list[dict[str, object]] = []
    for comparator_index, comparator in enumerate(COMPARATORS):
        baseline = selected_by_range[
            selected_by_range["family"].eq(comparator)
        ].set_index(RANGE_CONDITION)
        paired = (
            pta_by_range[list(METRICS)]
            .join(baseline[list(METRICS)], lsuffix="_pta", rsuffix="_comparator")
            .reset_index()
        )
        record = {
            "comparator": comparator,
            "operating_condition_ranges": len(paired),
        }
        for metric_index, metric in enumerate(METRICS):
            prefix = metric
            paired[f"{prefix}_log_ratio"] = np.log(
                paired[f"{metric}_pta"] / paired[f"{metric}_comparator"]
            )
            median, low, high = bootstrap_range_cluster_median(
                paired,
                value=f"{prefix}_log_ratio",
                draws=BOOTSTRAP_DRAWS,
                seed=20261809 + comparator_index * 100 + metric_index,
            )
            record[f"{prefix}_pta_lower"] = int(
                np.sum(paired[f"{prefix}_log_ratio"] < 0)
            )
            record[f"{prefix}_reduction_percent"] = reduction(median)
            record[f"{prefix}_ci_low"] = reduction(high)
            record[f"{prefix}_ci_high"] = reduction(low)
        range_rows.append(record)
    range_summary = pd.DataFrame(range_rows)
    range_summary.to_csv(HERE / "range_matched_summary.csv", index=False)

    # Threshold-range summaries show both relative effectiveness within a
    # range and which range supplied the lowest-R PTA implementation.
    best_range_alternative = (
        selected_by_range[~selected_by_range["family"].eq("PTA")]
        .loc[lambda frame: frame.groupby(RANGE_CONDITION)["R"].idxmin()]
        .set_index(RANGE_CONDITION)
    )
    range_effects = (
        pta_by_range[["R"]]
        .join(
            best_range_alternative[["R", "family"]],
            lsuffix="_pta",
            rsuffix="_alternative",
        )
        .reset_index()
    )
    range_effects["log_ratio"] = np.log(
        range_effects["R_pta"] / range_effects["R_alternative"]
    )
    range_effects["demand_class"] = range_effects["pattern"].map(pattern_names)
    range_factor_rows: list[dict[str, object]] = []
    for threshold_range, group in range_effects.groupby("threshold_range"):
        range_factor_rows.append(
            {
                "threshold_range": threshold_range,
                "demand_class": "All",
                "operating_conditions": len(group),
                "median_best_pta_R": float(group["R_pta"].median()),
                "median_pta_reduction_percent": reduction(
                    float(group["log_ratio"].median())
                ),
            }
        )
        for demand_class, demand_group in group.groupby("demand_class"):
            range_factor_rows.append(
                {
                    "threshold_range": threshold_range,
                    "demand_class": demand_class,
                    "operating_conditions": len(demand_group),
                    "median_best_pta_R": float(demand_group["R_pta"].median()),
                    "median_pta_reduction_percent": reduction(
                        float(demand_group["log_ratio"].median())
                    ),
                }
            )
    pd.DataFrame(range_factor_rows).to_csv(
        HERE / "threshold_range_factor_summary.csv", index=False
    )

    best_pta_range = pta_by_range.reset_index().loc[
        lambda frame: frame.groupby(CONDITION)["R"].idxmin()
    ]
    best_pta_range["demand_class"] = best_pta_range["pattern"].map(pattern_names)
    (
        best_pta_range.groupby(
            ["demand_class", "step_ratio", "threshold_range"], sort=True
        )
        .size()
        .rename("conditions_won")
        .reset_index()
        .to_csv(HERE / "threshold_range_winner_counts.csv", index=False)
    )

    # Confirmatory per-condition tests use the same PTA and comparator
    # configurations selected for the range-matched effect summary. Each test
    # pairs the 100 repetitions by repetition and verifies the stored simulator
    # and target-path seeds. BH correction is applied separately within each
    # comparator family across the 432 condition--range tests.
    selected_ids = selected_by_range[
        RANGE_CONDITION + ["family", "run_method_id"]
    ].copy()
    test_rows: list[dict[str, object]] = []
    for comparator in COMPARATORS:
        pta_ids = selected_ids[selected_ids["family"].eq("PTA")][
            RANGE_CONDITION + ["run_method_id"]
        ].rename(columns={"run_method_id": "pta_run_method_id"})
        comparator_ids = selected_ids[selected_ids["family"].eq(comparator)][
            RANGE_CONDITION + ["run_method_id"]
        ].rename(columns={"run_method_id": "comparator_run_method_id"})
        selected_pairs = pta_ids.merge(
            comparator_ids, on=RANGE_CONDITION, validate="one_to_one"
        )
        pta_runs = runs.merge(
            selected_pairs[RANGE_CONDITION + ["pta_run_method_id"]],
            on=RANGE_CONDITION,
            validate="many_to_one",
        )
        pta_runs = pta_runs.loc[
            pta_runs["run_method_id"].eq(pta_runs["pta_run_method_id"]),
            RANGE_CONDITION + ["rep", "seed", "target_path_seed", "run_method_id", "R"],
        ].rename(
            columns={
                "seed": "pta_seed",
                "target_path_seed": "pta_target_path_seed",
                "run_method_id": "pta_run_method_id",
                "R": "pta_R",
            }
        )
        comparator_runs = runs.merge(
            selected_pairs[RANGE_CONDITION + ["comparator_run_method_id"]],
            on=RANGE_CONDITION,
            validate="many_to_one",
        )
        comparator_runs = comparator_runs.loc[
            comparator_runs["run_method_id"].eq(
                comparator_runs["comparator_run_method_id"]
            ),
            RANGE_CONDITION + ["rep", "seed", "target_path_seed", "run_method_id", "R"],
        ].rename(
            columns={
                "seed": "comparator_seed",
                "target_path_seed": "comparator_target_path_seed",
                "run_method_id": "comparator_run_method_id",
                "R": "comparator_R",
            }
        )
        paired_runs = pta_runs.merge(
            comparator_runs,
            on=RANGE_CONDITION + ["rep"],
            validate="one_to_one",
        )
        if len(paired_runs) != 43_200:
            raise RuntimeError(
                f"Expected 43,200 paired repetitions for {comparator}, "
                f"found {len(paired_runs):,}"
            )
        if not paired_runs["pta_seed"].eq(paired_runs["comparator_seed"]).all():
            raise RuntimeError(f"Simulation seeds are not paired for {comparator}")
        if (
            not paired_runs["pta_target_path_seed"]
            .eq(paired_runs["comparator_target_path_seed"])
            .all()
        ):
            raise RuntimeError(f"Target-path seeds are not paired for {comparator}")

        for condition_values, group in paired_runs.groupby(RANGE_CONDITION, sort=True):
            differences = group["pta_R"].to_numpy() - group["comparator_R"].to_numpy()
            _, p_value = stats.wilcoxon(differences, alternative="two-sided")
            test_rows.append(
                {
                    **dict(zip(RANGE_CONDITION, condition_values)),
                    "comparator": comparator,
                    "pta_run_method_id": group["pta_run_method_id"].iloc[0],
                    "comparator_run_method_id": group["comparator_run_method_id"].iloc[
                        0
                    ],
                    "paired_repetitions": len(group),
                    "median_paired_difference_R": float(np.median(differences)),
                    "wilcoxon_two_sided_p": float(p_value),
                }
            )

    confirmatory_tests = pd.DataFrame(test_rows)
    confirmatory_tests["bh_q"] = np.nan
    for comparator, indices in confirmatory_tests.groupby("comparator").groups.items():
        confirmatory_tests.loc[indices, "bh_q"] = bh_adjust(
            confirmatory_tests.loc[indices, "wilcoxon_two_sided_p"].to_numpy()
        )
    confirmatory_tests["pta_median_lower"] = (
        confirmatory_tests["median_paired_difference_R"] < 0
    )
    confirmatory_tests["bh_significant"] = confirmatory_tests["bh_q"] < 0.05
    confirmatory_tests.to_csv(
        HERE / "range_matched_confirmatory_tests.csv", index=False
    )
    confirmatory_summary = (
        confirmatory_tests.groupby("comparator", sort=False)
        .agg(
            tests=("bh_q", "size"),
            pta_median_lower=("pta_median_lower", "sum"),
            bh_significant=("bh_significant", "sum"),
        )
        .reset_index()
    )
    significant_lower = (
        (confirmatory_tests["pta_median_lower"] & confirmatory_tests["bh_significant"])
        .groupby(confirmatory_tests["comparator"], sort=False)
        .sum()
    )
    confirmatory_summary["bh_significant_pta_lower"] = (
        confirmatory_summary["comparator"].map(significant_lower).astype(int)
    )
    confirmatory_summary.to_csv(
        HERE / "range_matched_confirmatory_summary.csv", index=False
    )

    # Sensitivity analysis without condition-wise PTA-variant selection. Every
    # fixed PTA configuration is compared with the baseline using the same
    # threshold range and, for SETA, the same stored-threshold mode. Resampling
    # retains all 12 PTA variants within each operating condition.
    all_pta = data[data["family"].eq("PTA")].copy()
    all_variant_rows: list[dict[str, object]] = []
    for comparator_index, comparator in enumerate(COMPARATORS):
        baseline = data[data["family"].eq(comparator)].copy()
        keys = RANGE_CONDITION.copy()
        if comparator == "SETA":
            keys.append("threshold_mode")
        paired = all_pta.merge(
            baseline[keys + list(METRICS)],
            on=keys,
            how="inner",
            suffixes=("_pta", "_comparator"),
            validate="many_to_one",
        )
        if len(paired) != 1_728:
            raise RuntimeError(
                f"Expected 1,728 all-variant comparisons for {comparator}, found {len(paired)}"
            )
        record = {
            "comparator": comparator,
            "configuration_condition_comparisons": len(paired),
        }
        for metric_index, metric in enumerate(METRICS):
            prefix = metric
            paired[f"{prefix}_log_ratio"] = np.log(
                paired[f"{metric}_pta"] / paired[f"{metric}_comparator"]
            )
            median, low, high = bootstrap_condition_cluster_median(
                paired,
                value=f"{prefix}_log_ratio",
                expected_variants=12,
                draws=BOOTSTRAP_DRAWS,
                seed=20262809 + comparator_index * 100 + metric_index,
            )
            record[f"{prefix}_pta_lower"] = int(
                np.sum(paired[f"{prefix}_log_ratio"] < 0)
            )
            record[f"{prefix}_reduction_percent"] = reduction(median)
            record[f"{prefix}_ci_low"] = reduction(high)
            record[f"{prefix}_ci_high"] = reduction(low)
        all_variant_rows.append(record)
    all_variant_summary = pd.DataFrame(all_variant_rows)
    all_variant_summary.to_csv(HERE / "all_variant_summary.csv", index=False)

    # Equal-tuning-budget sensitivity. Agent PTA configurations received 240
    # tuning trials, whereas Global PTA and every non-PTA adaptive
    # configuration received 100. Excluding Agent PTA therefore compares only
    # configurations with the common 100-trial budget.
    equal_budget = data[
        ~data["family"].eq("PTA") | data["gain_scheme"].eq("Global")
    ].copy()
    equal_selected = equal_budget.loc[
        equal_budget.groupby(CONDITION + ["family"])["R"].idxmin()
    ].copy()
    equal_winners = equal_selected.loc[
        equal_selected.groupby(CONDITION)["R"].idxmin()
    ].copy()
    equal_winner_counts = (
        equal_winners.groupby(["family", "run_method_id"])
        .size()
        .rename("conditions_won")
        .reset_index()
    )
    equal_winner_counts.to_csv(HERE / "equal_budget_winner_counts.csv", index=False)

    equal_by_range = equal_budget.loc[
        equal_budget.groupby(RANGE_CONDITION + ["family"])["R"].idxmin()
    ].copy()
    global_pta_by_range = equal_by_range[equal_by_range["family"].eq("PTA")].set_index(
        RANGE_CONDITION
    )
    equal_range_rows: list[dict[str, object]] = []
    for comparator_index, comparator in enumerate(COMPARATORS):
        baseline = equal_by_range[equal_by_range["family"].eq(comparator)].set_index(
            RANGE_CONDITION
        )
        paired = (
            global_pta_by_range[list(METRICS)]
            .join(baseline[list(METRICS)], lsuffix="_pta", rsuffix="_comparator")
            .reset_index()
        )
        record = {
            "comparator": comparator,
            "operating_condition_ranges": len(paired),
        }
        for metric_index, metric in enumerate(METRICS):
            prefix = metric
            paired[f"{prefix}_log_ratio"] = np.log(
                paired[f"{metric}_pta"] / paired[f"{metric}_comparator"]
            )
            median, low, high = bootstrap_range_cluster_median(
                paired,
                value=f"{prefix}_log_ratio",
                draws=BOOTSTRAP_DRAWS,
                seed=20263809 + comparator_index * 100 + metric_index,
            )
            record[f"{prefix}_pta_lower"] = int(
                np.sum(paired[f"{prefix}_log_ratio"] < 0)
            )
            record[f"{prefix}_reduction_percent"] = reduction(median)
            record[f"{prefix}_ci_low"] = reduction(high)
            record[f"{prefix}_ci_high"] = reduction(low)
        equal_range_rows.append(record)
    equal_range_summary = pd.DataFrame(equal_range_rows)
    equal_range_summary.to_csv(
        HERE / "equal_budget_range_matched_summary.csv", index=False
    )

    report = [
        "Clean frozen-reference transfer analysis",
        f"Run-level simulations represented: {len(runs):,}",
        f"Operating conditions: {data[CONDITION].drop_duplicates().shape[0]}",
        f"Configurations per condition: {len(data) // 144}",
        "Analysis window: full 1,000-step clean allocation run",
        "",
        "Winning configurations:",
        winner_counts.to_string(index=False),
        "",
        "Family-best paired comparisons:",
        summary.to_string(index=False),
        "",
        "Range-matched paired comparisons:",
        range_summary.to_string(index=False),
        "",
        "Range-matched confirmatory Wilcoxon tests:",
        confirmatory_summary.to_string(index=False),
        "",
        "All fixed PTA variants versus range-matched baselines:",
        all_variant_summary.to_string(index=False),
        "",
        "Equal-budget family winners (Agent PTA excluded):",
        equal_winner_counts.to_string(index=False),
        "",
        "Equal-budget Global PTA range-matched comparisons:",
        equal_range_summary.to_string(index=False),
    ]
    (HERE / "audit_report.txt").write_text("\n".join(report) + "\n")
    print("\n".join(report))


if __name__ == "__main__":
    main()
