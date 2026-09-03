#!/usr/bin/env python3
"""Paired Wilcoxon tests for PTA design comparisons.

The input is the completed clean frozen-reference allocation grid. Tests use
the 100 repetitions matched by simulation and target-path seed within each
operating condition. The stored-threshold family contains the SETA and PTA
clamped-versus-latent tests; the gain-scheme family contains the PTA
Agent-versus-Global tests. Benjamini-Hochberg correction is applied once
within each declared family.
"""

from __future__ import annotations

import os
from pathlib import Path

import numpy as np
import pandas as pd
from scipy import stats


REPO_ROOT = Path(__file__).resolve().parents[2]
HERE = REPO_ROOT / "data" / "processed" / "clean_transfer"
INPUT = Path(
    os.environ.get(
        "PTA_ALLOCATION_CSV",
        REPO_ROOT / "data" / "raw" / "allocation_grid" / "per_run_results.csv",
    )
)
CONDITION = ["pop", "n", "step_ratio", "pattern"]
STEP_RATIOS = (1.5, 2.0, 2.5)


def bh_adjust(p_values: np.ndarray) -> np.ndarray:
    """Return Benjamini-Hochberg adjusted p-values."""
    values = np.asarray(p_values, dtype=float)
    order = np.argsort(values)
    ranked = values[order] * len(values) / np.arange(1, len(values) + 1)
    ranked = np.minimum.accumulate(ranked[::-1])[::-1]
    adjusted = np.empty_like(ranked)
    adjusted[order] = np.minimum(ranked, 1.0)
    return adjusted


def paired_tests(
    left: pd.DataFrame,
    right: pd.DataFrame,
    *,
    match: list[str],
    group: list[str],
    comparison: str,
    left_label: str,
    right_label: str,
) -> pd.DataFrame:
    """Run one two-sided paired Wilcoxon test per design comparison."""
    value_columns = match + ["R", "seed", "target_path_seed"]
    paired = left[value_columns].merge(
        right[value_columns],
        on=match,
        how="inner",
        validate="one_to_one",
        suffixes=("_left", "_right"),
    )
    if not paired["seed_left"].eq(paired["seed_right"]).all():
        raise RuntimeError(f"Simulation seeds do not match for {comparison}")
    if not paired["target_path_seed_left"].eq(
        paired["target_path_seed_right"]
    ).all():
        raise RuntimeError(f"Target-path seeds do not match for {comparison}")

    rows: list[dict[str, object]] = []
    for keys, values in paired.groupby(group, sort=True, dropna=False):
        differences = values["R_left"].to_numpy() - values["R_right"].to_numpy()
        if len(differences) != 100:
            raise RuntimeError(
                f"Expected 100 paired repetitions for {comparison}, found "
                f"{len(differences)}"
            )
        if np.allclose(differences, 0.0):
            statistic, p_value = 0.0, 1.0
        else:
            result = stats.wilcoxon(differences, alternative="two-sided")
            statistic, p_value = float(result.statistic), float(result.pvalue)
        key_values = keys if isinstance(keys, tuple) else (keys,)
        rows.append(
            {
                **dict(zip(group, key_values)),
                "comparison": comparison,
                "left_label": left_label,
                "right_label": right_label,
                "paired_repetitions": len(differences),
                "median_paired_difference_R": float(np.median(differences)),
                "wilcoxon_statistic": statistic,
                "wilcoxon_two_sided_p": p_value,
            }
        )
    return pd.DataFrame(rows)


def summarize(
    tests: pd.DataFrame, *, group: list[str], left_favored_label: str
) -> pd.DataFrame:
    """Summarize adjusted significance and effect direction."""
    rows: list[dict[str, object]] = []
    for keys, values in tests.groupby(group, sort=False, dropna=False):
        key_values = keys if isinstance(keys, tuple) else (keys,)
        significant = values["bh_q"] < 0.05
        left_lower = values["median_paired_difference_R"] < 0
        right_lower = values["median_paired_difference_R"] > 0
        rows.append(
            {
                **dict(zip(group, key_values)),
                "tests": len(values),
                "significant_q_lt_0_05": int(significant.sum()),
                f"significant_{left_favored_label}": int(
                    (significant & left_lower).sum()
                ),
                "significant_opposite": int((significant & right_lower).sum()),
                "not_significant": int((~significant).sum()),
            }
        )
    return pd.DataFrame(rows)


def main() -> None:
    columns = CONDITION + [
        "family",
        "threshold_range",
        "threshold_mode",
        "gain_scheme",
        "rep",
        "seed",
        "target_path_seed",
        "R",
    ]
    runs = pd.read_csv(INPUT, usecols=columns)
    runs = runs[runs["step_ratio"].isin(STEP_RATIOS)].copy()
    if len(runs) != 388_800:
        raise RuntimeError(f"Expected 388,800 runs, found {len(runs):,}")

    pta = runs[runs["family"].eq("PTA")].copy()
    seta = runs[runs["family"].eq("SETA")].copy()

    seta_storage = paired_tests(
        seta[seta["threshold_mode"].eq("latent")],
        seta[seta["threshold_mode"].eq("clamped")],
        match=CONDITION + ["threshold_range", "rep"],
        group=CONDITION + ["threshold_range"],
        comparison="SETA storage mode",
        left_label="latent",
        right_label="clamped",
    )
    pta_storage = paired_tests(
        pta[pta["threshold_mode"].eq("latent")],
        pta[pta["threshold_mode"].eq("clamped")],
        match=CONDITION + ["threshold_range", "gain_scheme", "rep"],
        group=CONDITION + ["threshold_range", "gain_scheme"],
        comparison="PTA storage mode",
        left_label="latent",
        right_label="clamped",
    )
    storage = pd.concat([seta_storage, pta_storage], ignore_index=True)
    if len(storage) != 1_296:
        raise RuntimeError(f"Expected 1,296 storage tests, found {len(storage):,}")
    storage["bh_q"] = bh_adjust(storage["wilcoxon_two_sided_p"].to_numpy())
    storage["significant_q_lt_0_05"] = storage["bh_q"] < 0.05

    gains = paired_tests(
        pta[pta["gain_scheme"].eq("Agent")],
        pta[pta["gain_scheme"].eq("Global")],
        match=CONDITION + ["threshold_range", "threshold_mode", "rep"],
        group=CONDITION + ["threshold_range", "threshold_mode"],
        comparison="PTA gain scheme",
        left_label="Agent",
        right_label="Global",
    )
    if len(gains) != 864:
        raise RuntimeError(f"Expected 864 gain tests, found {len(gains):,}")
    gains["bh_q"] = bh_adjust(gains["wilcoxon_two_sided_p"].to_numpy())
    gains["significant_q_lt_0_05"] = gains["bh_q"] < 0.05

    storage_summary = summarize(
        storage,
        group=["comparison", "threshold_range"],
        left_favored_label="latent_lower",
    )
    gain_summary = summarize(
        gains,
        group=["threshold_range"],
        left_favored_label="agent_lower",
    )

    storage.to_csv(HERE / "storage_mode_wilcoxon_tests.csv", index=False)
    storage_summary.to_csv(HERE / "storage_mode_wilcoxon_summary.csv", index=False)
    gains.to_csv(HERE / "gain_scheme_wilcoxon_tests.csv", index=False)
    gain_summary.to_csv(HERE / "gain_scheme_wilcoxon_summary.csv", index=False)

    report = [
        "Frozen-reference design-comparison Wilcoxon analysis",
        "Significance threshold: Benjamini-Hochberg adjusted q < 0.05",
        "Stored-mode correction family: 1,296 SETA/PTA tests",
        "Gain-scheme correction family: 864 PTA tests",
        "",
        "Stored threshold modes:",
        storage_summary.to_string(index=False),
        "",
        "Gain schemes:",
        gain_summary.to_string(index=False),
    ]
    (HERE / "design_wilcoxon_audit.txt").write_text("\n".join(report) + "\n")
    print("\n".join(report))


if __name__ == "__main__":
    main()
