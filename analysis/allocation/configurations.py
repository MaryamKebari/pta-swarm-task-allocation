#!/usr/bin/env python3
"""Comprehensive read-only audit of the frozen-reference allocation grid."""

from __future__ import annotations

import os
from pathlib import Path

import numpy as np
import pandas as pd
from scipy.stats import wilcoxon

REPO_ROOT = Path(__file__).resolve().parents[2]
INPUT = Path(
    os.environ.get(
        "PTA_ALLOCATION_CSV",
        REPO_ROOT / "data" / "raw" / "allocation" / "per_run_results.csv",
    )
)
OUT = REPO_ROOT / "data" / "processed" / "allocation"

CONDITION = ["pop", "n", "step_ratio", "pattern"]
CONFIG = [
    "family",
    "threshold_range",
    "threshold_mode",
    "gain_scheme",
    "run_method_id",
]
METRICS = ["R", "R_abs"]
REFERENCE = {"pop": 500, "n": 4, "step_ratio": 2.0, "pattern": "scurve"}
PATTERN_LABELS = {
    "random": "Non-iterative gradual",
    "sharp": "Non-iterative non-gradual",
    "scurve": "Iterative gradual",
    "zigzag": "Iterative non-gradual",
}


def config_label(row: pd.Series) -> str:
    parts = [str(row["family"]), str(row["threshold_range"])]
    if row["family"] in {"SETA", "PTA"}:
        parts.append(str(row["threshold_mode"]))
    if row["family"] == "PTA":
        parts.append(str(row["gain_scheme"]))
    return "-".join(parts)


def reduction_from_log(log_ratio: pd.Series | np.ndarray) -> float:
    values = np.asarray(log_ratio, dtype=float)
    return 100.0 * (1.0 - np.exp(np.median(values)))


def bootstrap_reduction(
    effects: pd.DataFrame,
    cluster_columns: list[str],
    n_boot: int = 5000,
    seed: int = 20260822,
) -> tuple[float, float, float]:
    clustered = effects.groupby(cluster_columns, sort=False)["log_ratio"].apply(
        np.asarray
    )
    arrays = clustered.to_list()
    estimate = reduction_from_log(np.concatenate(arrays))
    rng = np.random.default_rng(seed)
    boot = np.empty(n_boot)
    for index in range(n_boot):
        sampled = rng.integers(0, len(arrays), len(arrays))
        boot[index] = reduction_from_log(np.concatenate([arrays[i] for i in sampled]))
    low, high = np.percentile(boot, [2.5, 97.5])
    return estimate, low, high


def bh_adjust(p_values: np.ndarray) -> np.ndarray:
    p = np.asarray(p_values, dtype=float)
    order = np.argsort(p)
    ranked = p[order]
    adjusted = ranked * len(p) / np.arange(1, len(p) + 1)
    adjusted = np.minimum.accumulate(adjusted[::-1])[::-1]
    adjusted = np.minimum(adjusted, 1.0)
    result = np.empty_like(adjusted)
    result[order] = adjusted
    return result


def audit_grid(runs: pd.DataFrame) -> list[str]:
    notes: list[str] = []
    notes.append(f"Rows: {len(runs):,}")
    notes.append(f"Conditions: {runs[CONDITION].drop_duplicates().shape[0]}")
    notes.append(f"Configurations: {runs[CONFIG].drop_duplicates().shape[0]}")
    notes.append(
        f"Repetitions: {sorted(runs['rep'].unique().tolist())[:3]} ... {sorted(runs['rep'].unique().tolist())[-3:]}"
    )
    counts = runs.groupby(CONDITION + CONFIG, dropna=False).size()
    notes.append(
        f"Rows per condition-configuration: {sorted(counts.unique().tolist())}"
    )
    seed_check = runs.groupby(CONDITION + ["rep"])[
        ["seed", "target_path_seed"]
    ].nunique()
    notes.append(
        f"Matched seeds across configurations: {bool(seed_check.eq(1).all().all())}"
    )
    duplicate_keys = runs.duplicated(CONDITION + CONFIG + ["rep"], keep=False).sum()
    notes.append(f"Duplicate condition-configuration-repetition rows: {duplicate_keys}")
    return notes


def select_reference_config(means: pd.DataFrame) -> pd.DataFrame:
    reference = means.copy()
    for key, value in REFERENCE.items():
        reference = reference[reference[key].eq(value)]
    selected = reference.loc[reference.groupby("family")["R"].idxmin()].copy()
    return selected.sort_values("family").reset_index(drop=True)


def best_by_condition(means: pd.DataFrame, family: str | None = None) -> pd.DataFrame:
    subset = means if family is None else means[means["family"].eq(family)]
    return subset.loc[subset.groupby(CONDITION)["R"].idxmin()].copy()


def fixed_config_summary(
    means: pd.DataFrame,
    pta_means: pd.DataFrame,
    best_pta: pd.DataFrame,
    best_alt: pd.DataFrame,
) -> pd.DataFrame:
    best_pta_keyed = best_pta.set_index(CONDITION)
    best_alt_keyed = best_alt.set_index(CONDITION)
    rows = []
    for run_method_id, group in pta_means.groupby("run_method_id", sort=False):
        group = group.set_index(CONDITION).sort_index()
        pta_best = best_pta_keyed.loc[group.index]
        alt_best = best_alt_keyed.loc[group.index]
        regret = group["R"].to_numpy() / pta_best["R"].to_numpy() - 1.0
        log_ratio = np.log(group["R"].to_numpy() / alt_best["R"].to_numpy())
        effects = group.reset_index()[CONDITION].copy()
        effects["log_ratio"] = log_ratio
        estimate, low, high = bootstrap_reduction(effects, CONDITION)
        first = group.iloc[0]
        rows.append(
            {
                "configuration": first["config_label"],
                "run_method_id": run_method_id,
                "threshold_range": first["threshold_range"],
                "threshold_mode": first["threshold_mode"],
                "gain_scheme": first["gain_scheme"],
                "mean_R": group["R"].mean(),
                "median_R": group["R"].median(),
                "geometric_mean_R": np.exp(np.log(group["R"]).mean()),
                "PTA_conditions_won": int((regret <= 1e-12).sum()),
                "within_5pct_of_best_PTA": int((regret <= 0.05).sum()),
                "within_10pct_of_best_PTA": int((regret <= 0.10).sum()),
                "median_regret_pct": 100.0 * np.median(regret),
                "p90_regret_pct": 100.0 * np.percentile(regret, 90),
                "max_regret_pct": 100.0 * np.max(regret),
                "wins_vs_conditionwise_best_alternative": int((log_ratio < 0).sum()),
                "median_reduction_vs_best_alt_pct": estimate,
                "reduction_ci_low": low,
                "reduction_ci_high": high,
            }
        )
    return pd.DataFrame(rows)


def fixed_vs_family_oracles(means: pd.DataFrame, pta_config: str) -> pd.DataFrame:
    pta = means[means["config_label"].eq(pta_config)].set_index(CONDITION)
    rows = []
    for family in ["CT", "LFTA", "SBTA", "SETA"]:
        comparator = (
            best_by_condition(means, family).set_index(CONDITION).loc[pta.index]
        )
        log_ratio = np.log(pta["R"].to_numpy() / comparator["R"].to_numpy())
        effects = pta.reset_index()[CONDITION].copy()
        effects["log_ratio"] = log_ratio
        estimate, low, high = bootstrap_reduction(effects, CONDITION)
        raw_log = np.log(pta["R_abs"].to_numpy() / comparator["R_abs"].to_numpy())
        rows.append(
            {
                "PTA_configuration": pta_config,
                "comparator_family": family,
                "PTA_lower_R": int((log_ratio < 0).sum()),
                "total": len(log_ratio),
                "median_R_reduction_pct": estimate,
                "R_ci_low": low,
                "R_ci_high": high,
                "PTA_lower_R_abs": int((raw_log < 0).sum()),
                "median_R_abs_reduction_pct": reduction_from_log(raw_log),
            }
        )
    return pd.DataFrame(rows)


def fixed_vs_reference_selected(
    runs: pd.DataFrame,
    selected: pd.DataFrame,
    pta_config: str,
    exclude_reference: bool = False,
) -> tuple[pd.DataFrame, pd.DataFrame]:
    selected_ids = dict(zip(selected["family"], selected["run_method_id"]))
    pta_id = runs.loc[runs["config_label"].eq(pta_config), "run_method_id"].iloc[0]
    rows = []
    tests = []
    for family in ["CT", "LFTA", "SBTA", "SETA"]:
        subset = runs[runs["run_method_id"].isin([pta_id, selected_ids[family]])].copy()
        pivot = subset.pivot_table(
            index=CONDITION + ["rep"], columns="run_method_id", values=["R", "R_abs"]
        )
        condition_means = pivot.groupby(level=CONDITION).mean()
        if exclude_reference:
            index_frame = condition_means.index.to_frame(index=False)
            reference_match = np.ones(len(index_frame), dtype=bool)
            for key, value in REFERENCE.items():
                reference_match &= index_frame[key].eq(value).to_numpy()
            condition_means = condition_means.loc[~reference_match]
            pivot_frame = pivot.index.to_frame(index=False)
            reference_rows = np.ones(len(pivot_frame), dtype=bool)
            for key, value in REFERENCE.items():
                reference_rows &= pivot_frame[key].eq(value).to_numpy()
            pivot = pivot.loc[~reference_rows]
        log_ratio = np.log(
            condition_means[("R", pta_id)].to_numpy()
            / condition_means[("R", selected_ids[family])].to_numpy()
        )
        raw_log = np.log(
            condition_means[("R_abs", pta_id)].to_numpy()
            / condition_means[("R_abs", selected_ids[family])].to_numpy()
        )
        effects = condition_means.reset_index()[CONDITION].copy()
        effects["log_ratio"] = log_ratio
        estimate, low, high = bootstrap_reduction(effects, CONDITION)
        raw_effects = condition_means.reset_index()[CONDITION].copy()
        raw_effects["log_ratio"] = raw_log
        raw_estimate, raw_low, raw_high = bootstrap_reduction(
            raw_effects, CONDITION, seed=20260823
        )
        rows.append(
            {
                "PTA_configuration": pta_config,
                "evaluation_scope": "held-out conditions"
                if exclude_reference
                else "complete grid",
                "comparator_configuration": selected.loc[
                    selected["family"].eq(family), "config_label"
                ].iloc[0],
                "PTA_lower_R": int((log_ratio < 0).sum()),
                "total": len(log_ratio),
                "median_R_reduction_pct": estimate,
                "R_ci_low": low,
                "R_ci_high": high,
                "PTA_lower_R_abs": int((raw_log < 0).sum()),
                "median_R_abs_reduction_pct": raw_estimate,
                "R_abs_ci_low": raw_low,
                "R_abs_ci_high": raw_high,
            }
        )
        for condition, local in pivot.groupby(level=CONDITION):
            x = local[("R", pta_id)].to_numpy()
            y = local[("R", selected_ids[family])].to_numpy()
            try:
                statistic, p_value = wilcoxon(x, y, alternative="two-sided")
            except ValueError:
                statistic, p_value = 0.0, 1.0
            record = dict(zip(CONDITION, condition))
            record.update(
                {
                    "PTA_configuration": pta_config,
                    "evaluation_scope": "held-out conditions"
                    if exclude_reference
                    else "complete grid",
                    "comparator_family": family,
                    "median_difference": float(np.median(x - y)),
                    "wilcoxon_statistic": float(statistic),
                    "p_value": float(p_value),
                }
            )
            tests.append(record)
    tests_df = pd.DataFrame(tests)
    tests_df["q_value"] = bh_adjust(tests_df["p_value"].to_numpy())
    tests_df["significant"] = tests_df["q_value"].lt(0.05)
    tests_df["direction"] = np.where(
        tests_df["median_difference"].lt(0), "PTA", "comparator"
    )
    return pd.DataFrame(rows), tests_df


def fixed_vs_range_matched(means: pd.DataFrame, pta_config: str) -> pd.DataFrame:
    """Compare one fixed PTA configuration with alternatives using the same range.

    SETA additionally uses the same stored-threshold mode as PTA. CT, LFTA, and
    SBTA do not have a stored-threshold-mode factor.
    """
    pta = means[means["config_label"].eq(pta_config)].set_index(CONDITION).sort_index()
    if len(pta) != 144:
        raise ValueError(f"Expected 144 conditions for {pta_config}; found {len(pta)}")
    threshold_range = pta["threshold_range"].iloc[0]
    threshold_mode = pta["threshold_mode"].iloc[0]
    rows = []
    for family in ["CT", "LFTA", "SBTA", "SETA"]:
        comparator = means[
            means["family"].eq(family) & means["threshold_range"].eq(threshold_range)
        ].copy()
        if family == "SETA":
            comparator = comparator[comparator["threshold_mode"].eq(threshold_mode)]
        if comparator["run_method_id"].nunique() != 1:
            raise ValueError(
                f"Expected one range-matched {family} configuration for {pta_config}; "
                f"found {comparator['run_method_id'].nunique()}"
            )
        comparator = comparator.set_index(CONDITION).sort_index().loc[pta.index]
        log_ratio = np.log(pta["R"].to_numpy() / comparator["R"].to_numpy())
        raw_log = np.log(pta["R_abs"].to_numpy() / comparator["R_abs"].to_numpy())
        effects = pta.reset_index()[CONDITION].copy()
        effects["log_ratio"] = log_ratio
        estimate, low, high = bootstrap_reduction(effects, CONDITION)
        raw_effects = pta.reset_index()[CONDITION].copy()
        raw_effects["log_ratio"] = raw_log
        raw_estimate, raw_low, raw_high = bootstrap_reduction(
            raw_effects, CONDITION, seed=20260823
        )
        rows.append(
            {
                "PTA_configuration": pta_config,
                "comparator_configuration": comparator["config_label"].iloc[0],
                "PTA_lower_R": int((log_ratio < 0).sum()),
                "total": len(log_ratio),
                "median_R_reduction_pct": estimate,
                "R_ci_low": low,
                "R_ci_high": high,
                "PTA_lower_R_abs": int((raw_log < 0).sum()),
                "median_R_abs_reduction_pct": raw_estimate,
                "R_abs_ci_low": raw_low,
                "R_abs_ci_high": raw_high,
            }
        )
    return pd.DataFrame(rows)


def factor_summary(
    means: pd.DataFrame,
    pta_config: str,
    best_alt: pd.DataFrame,
    factor: str,
) -> pd.DataFrame:
    pta = means[means["config_label"].eq(pta_config)].set_index(CONDITION)
    alt = best_alt.set_index(CONDITION).loc[pta.index]
    merged = pta.reset_index()[CONDITION + ["R", "R_abs"]].copy()
    merged["alt_R"] = alt["R"].to_numpy()
    merged["alt_R_abs"] = alt["R_abs"].to_numpy()
    merged["log_ratio"] = np.log(merged["R"] / merged["alt_R"])
    merged["raw_log_ratio"] = np.log(merged["R_abs"] / merged["alt_R_abs"])
    rows = []
    for level, group in merged.groupby(factor, sort=True):
        rows.append(
            {
                "PTA_configuration": pta_config,
                "factor": factor,
                "level": PATTERN_LABELS.get(level, level),
                "conditions": len(group),
                "mean_R": group["R"].mean(),
                "median_R": group["R"].median(),
                "wins_vs_best_alternative": int(group["log_ratio"].lt(0).sum()),
                "median_R_reduction_pct": reduction_from_log(group["log_ratio"]),
                "wins_R_abs": int(group["raw_log_ratio"].lt(0).sum()),
                "median_R_abs_reduction_pct": reduction_from_log(
                    group["raw_log_ratio"]
                ),
            }
        )
    return pd.DataFrame(rows)


def preferred_counts(
    means: pd.DataFrame, family: str, factors: list[str]
) -> pd.DataFrame:
    winners = best_by_condition(means, family)
    result = (
        winners.groupby(factors + ["config_label"])
        .size()
        .rename("conditions_won")
        .reset_index()
    )
    for factor in factors:
        if factor == "pattern":
            result[factor] = result[factor].map(PATTERN_LABELS)
    return result


def write_report(
    audit: list[str],
    selected: pd.DataFrame,
    fixed: pd.DataFrame,
    oracle_tables: pd.DataFrame,
    reference_tables: pd.DataFrame,
    significance: pd.DataFrame,
) -> None:
    lines = ["FROZEN-REFERENCE FIXED-CONFIGURATION AUDIT", ""]
    lines.extend(audit)
    lines.extend(
        [
            "",
            "REFERENCE-SETTING SELECTIONS",
            selected[["family", "config_label", "R", "R_abs"]].to_string(index=False),
        ]
    )
    lines.extend(["", "ALL FIXED PTA CONFIGURATIONS", fixed.to_string(index=False)])
    lines.extend(
        [
            "",
            "KEY PTA CONFIGURATIONS VS CONDITION-WISE FAMILY ORACLES",
            oracle_tables.to_string(index=False),
        ]
    )
    lines.extend(
        [
            "",
            "KEY PTA CONFIGURATIONS VS REFERENCE-SELECTED FIXED COMPARATORS",
            reference_tables.to_string(index=False),
        ]
    )
    sig = (
        significance.groupby(["PTA_configuration", "comparator_family", "direction"])[
            "significant"
        ]
        .sum()
        .unstack(fill_value=0)
        .reset_index()
    )
    lines.extend(
        [
            "",
            "WILCOXON RESULTS FOR REFERENCE-SELECTED FIXED COMPARATORS",
            sig.to_string(index=False),
        ]
    )
    (OUT / "audit_report.txt").write_text("\n".join(lines) + "\n")


def main() -> None:
    OUT.mkdir(parents=True, exist_ok=True)
    columns = (
        CONDITION
        + CONFIG
        + [
            "method",
            "method_label",
            "rep",
            "seed",
            "target_path_seed",
            "R",
            "R_abs",
        ]
    )
    runs = pd.read_csv(INPUT, usecols=columns)
    runs = runs[runs["step_ratio"].isin([1.5, 2.0, 2.5])].copy()
    runs["gain_scheme"] = runs["gain_scheme"].fillna("")
    runs["config_label"] = runs.apply(config_label, axis=1)
    audit = audit_grid(runs)

    means = (
        runs.groupby(CONDITION + CONFIG + ["config_label"], dropna=False)[METRICS]
        .mean()
        .reset_index()
    )
    selected = select_reference_config(means)
    selected.to_csv(OUT / "reference_selected_configurations.csv", index=False)

    pta_means = means[means["family"].eq("PTA")].copy()
    best_pta = best_by_condition(means, "PTA")
    best_alt = best_by_condition(means[~means["family"].eq("PTA")])
    fixed = fixed_config_summary(means, pta_means, best_pta, best_alt)
    fixed = fixed.sort_values(
        ["wins_vs_conditionwise_best_alternative", "median_reduction_vs_best_alt_pct"],
        ascending=False,
    ).reset_index(drop=True)
    fixed.to_csv(OUT / "fixed_pta_configuration_summary.csv", index=False)

    key_configurations = list(
        dict.fromkeys(
            [
                selected.loc[selected["family"].eq("PTA"), "config_label"].iloc[0],
                fixed.sort_values("mean_R").iloc[0]["configuration"],
                fixed.iloc[0]["configuration"],
                "PTA-HT2-clamped-Agent",
            ]
        )
    )
    oracle_tables = pd.concat(
        [fixed_vs_family_oracles(means, config) for config in key_configurations],
        ignore_index=True,
    )
    oracle_tables.to_csv(
        OUT / "key_pta_vs_conditionwise_family_oracles.csv", index=False
    )

    reference_tables = []
    significance_tables = []
    held_out_tables = []
    held_out_significance_tables = []
    for config in key_configurations:
        comparison, tests = fixed_vs_reference_selected(runs, selected, config)
        reference_tables.append(comparison)
        significance_tables.append(tests)
        held_out_comparison, held_out_tests = fixed_vs_reference_selected(
            runs, selected, config, exclude_reference=True
        )
        held_out_tables.append(held_out_comparison)
        held_out_significance_tables.append(held_out_tests)
    reference_tables_df = pd.concat(reference_tables, ignore_index=True)
    significance_df = pd.concat(significance_tables, ignore_index=True)
    held_out_tables_df = pd.concat(held_out_tables, ignore_index=True)
    held_out_significance_df = pd.concat(
        held_out_significance_tables, ignore_index=True
    )
    reference_tables_df.to_csv(
        OUT / "key_pta_vs_reference_selected_fixed.csv", index=False
    )
    significance_df.to_csv(OUT / "fixed_comparison_wilcoxon_tests.csv", index=False)
    held_out_tables_df.to_csv(
        OUT / "key_pta_vs_reference_selected_fixed_held_out.csv", index=False
    )
    held_out_significance_df.to_csv(
        OUT / "fixed_comparison_wilcoxon_tests_held_out.csv", index=False
    )

    range_matched_df = pd.concat(
        [fixed_vs_range_matched(means, config) for config in key_configurations],
        ignore_index=True,
    )
    range_matched_df.to_csv(
        OUT / "key_fixed_pta_vs_range_matched_alternatives.csv", index=False
    )

    factor_tables = []
    for config in key_configurations:
        for factor in ["pattern", "step_ratio", "pop", "n"]:
            factor_tables.append(factor_summary(means, config, best_alt, factor))
    factor_df = pd.concat(factor_tables, ignore_index=True)
    factor_df.to_csv(OUT / "key_pta_factor_summary.csv", index=False)

    preferred_tables = []
    for family in ["CT", "LFTA", "SBTA", "SETA", "PTA"]:
        preferred_tables.append(
            preferred_counts(means, family, ["pattern", "step_ratio"])
        )
    preferred_df = pd.concat(
        preferred_tables, keys=["CT", "LFTA", "SBTA", "SETA", "PTA"], names=["family"]
    ).reset_index(level=0)
    preferred_df.to_csv(
        OUT / "family_preferred_configurations_by_demand_and_step.csv", index=False
    )

    pta_preferred_pop_task = preferred_counts(means, "PTA", ["pop", "n"])
    pta_preferred_pop_task.to_csv(
        OUT / "pta_preferred_configurations_by_population_and_tasks.csv", index=False
    )
    best_pta.to_csv(OUT / "conditionwise_preferred_pta.csv", index=False)
    best_alt.to_csv(OUT / "conditionwise_best_alternative.csv", index=False)

    write_report(
        audit,
        selected,
        fixed,
        oracle_tables,
        reference_tables_df,
        significance_df,
    )
    print((OUT / "audit_report.txt").read_text())


if __name__ == "__main__":
    main()
