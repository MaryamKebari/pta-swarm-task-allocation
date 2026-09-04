#!/usr/bin/env python3
"""Reviewer-facing stratification checks for the agent-removal results."""

from pathlib import Path

import pandas as pd

REPO_ROOT = Path(__file__).resolve().parents[2]
BASE = REPO_ROOT / "data" / "processed" / "agent_removal"


def main() -> None:
    condition = pd.read_csv(BASE / "condition_means.csv")
    pta = condition[
        condition["family"].eq("PTA") & condition["kill_percent"].gt(0)
    ].copy()
    operating_keys = ["pop", "n", "step_ratio", "pattern", "kill_percent"]
    aggregate = (
        pta.groupby(operating_keys)[["post_removal_R", "post_removal_R_abs"]]
        .median()
        .reset_index()
    )

    print("PTA medians by removal and step ratio")
    print(
        aggregate.groupby(["kill_percent", "step_ratio"])[
            ["post_removal_R", "post_removal_R_abs"]
        ]
        .median()
        .round(4)
        .to_string()
    )

    at_fifty = aggregate[aggregate["kill_percent"].eq(50)]
    print("\nPTA medians at 50% removal by task count")
    print(
        at_fifty.groupby("n")[["post_removal_R", "post_removal_R_abs"]]
        .median()
        .round(4)
        .to_string()
    )
    print("\nPTA medians at 50% removal by population")
    print(
        at_fifty.groupby("pop")[["post_removal_R", "post_removal_R_abs"]]
        .median()
        .round(4)
        .to_string()
    )

    comparisons = pd.read_csv(BASE / "pta_matched_comparisons.csv")
    print("\nMedian PTA reductions by comparator, removal, and step ratio")
    for kill in (5, 40, 50):
        subset = comparisons[comparisons["kill_percent"].eq(kill)]
        effects = (
            subset.groupby(["comparator", "step_ratio", "pop", "n", "pattern"])[
                [
                    "post_removal_R_reduction_percent",
                    "post_removal_R_abs_reduction_percent",
                ]
            ]
            .median()
            .reset_index()
        )
        print(f"\nRemoval: {kill}%")
        print(
            effects.groupby(["comparator", "step_ratio"])[
                [
                    "post_removal_R_reduction_percent",
                    "post_removal_R_abs_reduction_percent",
                ]
            ]
            .median()
            .round(3)
            .to_string()
        )

    wide = aggregate.pivot_table(
        index=["pop", "n", "step_ratio", "pattern"],
        columns="kill_percent",
        values="post_removal_R",
    )
    increments = wide.diff(axis=1).iloc[:, 1:]
    print("\nCondition-level monotonicity of aggregate PTA post-removal R")
    print(
        f"Strictly increasing across every severity: {(increments > 0).all(axis=1).sum()}/{len(wide)}"
    )
    print(f"50% greater than 40%: {(wide[50] > wide[40]).sum()}/{len(wide)}")
    print(f"40% greater than 30%: {(wide[40] > wide[30]).sum()}/{len(wide)}")


if __name__ == "__main__":
    main()
