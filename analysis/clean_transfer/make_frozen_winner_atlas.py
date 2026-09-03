#!/usr/bin/env python3
"""Create a Figure-2-style winner atlas for the frozen-reference grid."""

from __future__ import annotations

import os
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Patch
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

POPS = [50, 100, 500, 1000]
TASKS = [4, 8, 12]
STEPS = [1.5, 2.0, 2.5]
PATTERNS = ["random", "sharp", "scurve", "zigzag"]
PATTERN_LABELS = {
    "random": "Non-iterative\ngradual",
    "sharp": "Non-iterative\nnon-gradual",
    "scurve": "Iterative\ngradual",
    "zigzag": "Iterative\nnon-gradual",
}
CONDITION = ["pop", "n", "pattern", "step_ratio"]

RANGE_FILL_COLORS = {
    "HM": "#B9DED8",
    "HT1": "#F0D28F",
    "HT2": "#D7C2E6",
}
RANGE_LINESTYLES = {"HM": "-", "HT1": "--", "HT2": ":"}


def load_winners() -> pd.DataFrame:
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
    runs = runs[runs["step_ratio"].isin(STEPS)].copy()
    if len(runs) != 388_800:
        raise RuntimeError(f"Expected 388,800 runs; found {len(runs):,}")

    config = [
        "family",
        "threshold_range",
        "threshold_mode",
        "gain_scheme",
        "run_method_id",
    ]
    means = (
        runs.groupby(CONDITION + config, dropna=False)["R"]
        .mean()
        .reset_index()
    )
    if len(means) != 3_888:
        raise RuntimeError(f"Expected 3,888 configuration means; found {len(means):,}")

    winners = means.loc[means.groupby(CONDITION)["R"].idxmin()].copy()
    winners = winners.sort_values(CONDITION).reset_index(drop=True)
    if len(winners) != 144:
        raise RuntimeError(f"Expected 144 winners; found {len(winners)}")
    if set(winners["family"]) != {"PTA"}:
        raise RuntimeError("A non-PTA family appears in the frozen-reference winners")
    if set(winners["gain_scheme"]) != {"Agent"}:
        raise RuntimeError("The winning frozen-reference configurations are not all Agent PTA")
    return winners


def make_figure(winners: pd.DataFrame) -> None:
    plt.rcParams.update(
        {
            "font.family": "serif",
            "font.size": 13.5,
            "axes.titlesize": 15.0,
            "axes.labelsize": 14.0,
            "legend.fontsize": 12.5,
            "xtick.labelsize": 12.5,
            "ytick.labelsize": 12.5,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
        }
    )

    fig, axes = plt.subplots(
        len(PATTERNS), len(TASKS), figsize=(7.15, 8.35), sharex=True, sharey=True
    )
    for row, pattern in enumerate(PATTERNS):
        for col, task_count in enumerate(TASKS):
            ax = axes[row, col]
            local = winners[
                winners["n"].eq(task_count) & winners["pattern"].eq(pattern)
            ]
            for yi, population in enumerate(POPS):
                for xi, step_ratio in enumerate(STEPS):
                    match = local[
                        local["pop"].eq(population)
                        & local["step_ratio"].eq(step_ratio)
                    ]
                    if len(match) != 1:
                        raise RuntimeError(
                            "Missing or duplicate atlas cell for "
                            f"n={population}, m={task_count}, pattern={pattern}, "
                            f"s={step_ratio}"
                        )
                    record = match.iloc[0]
                    threshold_range = str(record["threshold_range"])
                    mode = "L" if record["threshold_mode"] == "latent" else "C"
                    ax.add_patch(
                        plt.Rectangle(
                            (xi - 0.5, yi - 0.5),
                            1,
                            1,
                            facecolor=RANGE_FILL_COLORS[threshold_range],
                            edgecolor="#33434D",
                            linewidth=1.4,
                            linestyle=RANGE_LINESTYLES[threshold_range],
                        )
                    )
                    ax.text(
                        xi,
                        yi,
                        mode,
                        ha="center",
                        va="center",
                        fontsize=19.0,
                        color="#20303C",
                        fontweight="bold",
                    )

            if row == 0:
                ax.set_title(f"$m={task_count}$", fontsize=15.0)
            if col == 0:
                ax.annotate(
                    PATTERN_LABELS[pattern],
                    xy=(-0.56, 0.5),
                    xycoords="axes fraction",
                    ha="right",
                    va="center",
                    fontsize=12.0,
                    fontweight="semibold",
                    annotation_clip=False,
                )
            ax.set_xticks(range(len(STEPS)), [str(value) for value in STEPS])
            ax.set_yticks(range(len(POPS)), [str(value) for value in POPS])
            ax.tick_params(axis="both", labelsize=12.5)
            ax.set_xlim(-0.5, len(STEPS) - 0.5)
            ax.set_ylim(-0.5, len(POPS) - 0.5)

    handles = [
        Patch(facecolor=RANGE_FILL_COLORS["HM"], edgecolor="#33434D", linestyle="-", label="HM"),
        Patch(facecolor=RANGE_FILL_COLORS["HT1"], edgecolor="#33434D", linestyle="--", label="HT1"),
        Patch(facecolor=RANGE_FILL_COLORS["HT2"], edgecolor="#33434D", linestyle=":", label="HT2"),
        Patch(facecolor="none", edgecolor="none", label="C: clamped; L: latent"),
    ]
    fig.legend(
        handles=handles,
        loc="upper center",
        ncol=4,
        frameon=False,
        bbox_to_anchor=(0.5, 1.01),
        fontsize=12.5,
        columnspacing=1.25,
        handletextpad=0.45,
    )
    fig.supylabel("Population size, $n$", x=0.012, fontsize=14.0)
    fig.supxlabel("Step ratio, $s$", y=0.02, fontsize=14.0)
    fig.tight_layout(rect=[0.16, 0.04, 1, 0.95], h_pad=1.2, w_pad=0.8)

    FIGURES.mkdir(parents=True, exist_ok=True)
    fig.savefig(FIGURES / "frozen_reference_winner_atlas.pdf", bbox_inches="tight")
    fig.savefig(
        FIGURES / "frozen_reference_winner_atlas.png",
        dpi=300,
        bbox_inches="tight",
    )
    plt.close(fig)


def main() -> None:
    winners = load_winners()
    winners.to_csv(HERE / "winner_by_condition.csv", index=False)
    make_figure(winners)
    print(
        winners.groupby(
            ["family", "threshold_range", "threshold_mode", "gain_scheme"]
        )
        .size()
        .rename("conditions_won")
        .reset_index()
        .to_string(index=False)
    )


if __name__ == "__main__":
    main()
