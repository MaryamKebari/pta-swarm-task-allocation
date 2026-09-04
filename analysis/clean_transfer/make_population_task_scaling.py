#!/usr/bin/env python3
"""Plot frozen-reference PTA advantage by population size and task count."""

from __future__ import annotations

import os
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

SCRIPT_DIR = Path(__file__).resolve().parent
ROOT = SCRIPT_DIR.parents[1]
HERE = ROOT / "data" / "processed" / "clean_transfer"
FIGURES = ROOT / "figures"
INPUT = Path(
    os.environ.get(
        "PTA_ALLOCATION_CSV",
        ROOT / "data" / "raw" / "allocation_grid" / "per_run_results.csv",
    )
)
CONDITION = ["pop", "n", "step_ratio", "pattern"]
RANGE_CONDITION = CONDITION + ["threshold_range"]
RANGES = ("HM", "HT1", "HT2")
COLORS = {"HM": "#2A9D8F", "HT1": "#C58B20", "HT2": "#9B6FC3"}
MARKERS = {"HM": "o", "HT1": "s", "HT2": "^"}
LINESTYLES = {"HM": "-", "HT1": "--", "HT2": ":"}
BOOTSTRAP_DRAWS = 20_000


def bootstrap_median(values: np.ndarray, seed: int) -> tuple[float, float, float]:
    values = np.asarray(values, dtype=float)
    rng = np.random.default_rng(seed)
    estimates = np.empty(BOOTSTRAP_DRAWS, dtype=float)
    batch = 500
    for start in range(0, BOOTSTRAP_DRAWS, batch):
        stop = min(start + batch, BOOTSTRAP_DRAWS)
        indices = rng.integers(0, len(values), size=(stop - start, len(values)))
        estimates[start:stop] = np.median(values[indices], axis=1)
    low, high = np.quantile(estimates, [0.025, 0.975])
    return float(np.median(values)), float(low), float(high)


def load_effects() -> pd.DataFrame:
    columns = CONDITION + [
        "family",
        "threshold_range",
        "threshold_mode",
        "gain_scheme",
        "run_method_id",
        "rep",
        "R",
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
    means = runs.groupby(CONDITION + config, dropna=False)["R"].mean().reset_index()
    if len(means) != 3_888:
        raise RuntimeError(f"Expected 3,888 configuration means, found {len(means):,}")

    selected = means.loc[
        means.groupby(RANGE_CONDITION + ["family"])["R"].idxmin()
    ].copy()
    pta = selected[selected["family"].eq("PTA")].set_index(RANGE_CONDITION)
    alternative = (
        selected[~selected["family"].eq("PTA")]
        .loc[lambda frame: frame.groupby(RANGE_CONDITION)["R"].idxmin()]
        .set_index(RANGE_CONDITION)
    )
    effects = (
        pta[["R"]]
        .join(
            alternative[["R", "family"]],
            lsuffix="_pta",
            rsuffix="_alternative",
        )
        .reset_index()
    )
    if len(effects) != 432:
        raise RuntimeError(
            f"Expected 432 condition-range effects, found {len(effects)}"
        )
    effects["reduction_percent"] = 100.0 * (
        1.0 - effects["R_pta"] / effects["R_alternative"]
    )
    return effects


def summarize(effects: pd.DataFrame) -> pd.DataFrame:
    rows: list[dict[str, float | int | str]] = []
    for factor_index, (factor, levels) in enumerate(
        (("pop", (50, 100, 500, 1000)), ("n", (4, 8, 12)))
    ):
        for range_index, threshold_range in enumerate(RANGES):
            for level_index, level in enumerate(levels):
                values = effects.loc[
                    effects["threshold_range"].eq(threshold_range)
                    & effects[factor].eq(level),
                    "reduction_percent",
                ].to_numpy(float)
                expected = 36 if factor == "pop" else 48
                if len(values) != expected:
                    raise RuntimeError(
                        f"Expected {expected} values for {factor}={level}, "
                        f"{threshold_range}; found {len(values)}"
                    )
                median, low, high = bootstrap_median(
                    values,
                    seed=20260831
                    + factor_index * 1000
                    + range_index * 100
                    + level_index,
                )
                rows.append(
                    {
                        "factor": factor,
                        "threshold_range": threshold_range,
                        "level": level,
                        "comparisons": len(values),
                        "median_reduction_percent": median,
                        "ci_low": low,
                        "ci_high": high,
                    }
                )
    return pd.DataFrame(rows)


def make_figure(summary: pd.DataFrame) -> None:
    plt.rcParams.update(
        {
            "font.family": "serif",
            "font.size": 12.0,
            "axes.titlesize": 12.8,
            "axes.labelsize": 12.3,
            "xtick.labelsize": 11.2,
            "ytick.labelsize": 11.2,
            "legend.fontsize": 11.2,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
        }
    )
    fig, axes = plt.subplots(1, 2, figsize=(7.15, 3.85), sharey=True)
    panels = (
        (
            axes[0],
            "pop",
            (50, 100, 500, 1000),
            "(a) Population size",
            "Population size, $n$",
        ),
        (axes[1], "n", (4, 8, 12), "(b) Task count", "Number of tasks, $m$"),
    )
    for ax, factor, levels, title, xlabel in panels:
        for threshold_range in RANGES:
            data = (
                summary[
                    summary["factor"].eq(factor)
                    & summary["threshold_range"].eq(threshold_range)
                ]
                .set_index("level")
                .loc[list(levels)]
            )
            x = np.arange(len(levels), dtype=float)
            y = data["median_reduction_percent"].to_numpy(float)
            low = data["ci_low"].to_numpy(float)
            high = data["ci_high"].to_numpy(float)
            ax.errorbar(
                x,
                y,
                yerr=np.vstack((y - low, high - y)),
                color=COLORS[threshold_range],
                marker=MARKERS[threshold_range],
                linestyle=LINESTYLES[threshold_range],
                linewidth=2.0,
                markersize=6.2,
                elinewidth=1.25,
                capsize=3.0,
                capthick=1.15,
                label=threshold_range,
            )
        ax.axhline(0, color="#555555", linewidth=0.9, linestyle="--")
        ax.set_title(title, loc="left", fontweight="semibold")
        ax.set_xlabel(xlabel)
        ax.set_xticks(np.arange(len(levels)), [str(level) for level in levels])
        ax.grid(axis="y", color="#D9D9D9", linewidth=0.6)
        ax.spines[["top", "right"]].set_visible(False)
    axes[0].set_ylabel(r"Median PTA reduction in $R$ (\%)")
    handles, labels = axes[0].get_legend_handles_labels()
    fig.legend(
        handles,
        labels,
        title="Threshold range",
        loc="upper center",
        ncol=3,
        frameon=False,
        bbox_to_anchor=(0.5, 1.03),
    )
    fig.text(
        0.5,
        0.015,
        "Points are medians; bars are 95% bootstrap intervals across matched operating conditions.",
        ha="center",
        fontsize=11.2,
        color="#444444",
    )
    fig.tight_layout(rect=(0, 0.07, 1, 0.88), w_pad=2.0)
    FIGURES.mkdir(parents=True, exist_ok=True)
    fig.savefig(
        FIGURES / "frozen_reference_population_task_scaling.pdf", bbox_inches="tight"
    )
    fig.savefig(
        FIGURES / "frozen_reference_population_task_scaling.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.close(fig)


def main() -> None:
    effects = load_effects()
    summary = summarize(effects)
    summary.to_csv(HERE / "population_task_scaling_summary.csv", index=False)
    make_figure(summary)
    print(summary.to_string(index=False))


if __name__ == "__main__":
    main()
