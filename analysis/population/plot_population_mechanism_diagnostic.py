#!/usr/bin/env python3
"""Summarize and plot the population scaling diagnostic."""

from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy import stats

PTA_BLUE = "#0072B2"
TASK_ORANGE = "#D55E00"
DEMAND_GRAY = "#4D4D4D"
GRID_GRAY = "#D9D9D9"
L_T = 3.0
STEP_RATIO = 2.0


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--trajectories", type=Path, required=True)
    parser.add_argument("--validation", type=Path, required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--summary", type=Path, required=True)
    parser.add_argument("--representative", type=Path, required=True)
    args = parser.parse_args()

    data = np.load(args.trajectories)
    populations = data["populations"].astype(int)
    demand = data["demand"].astype(np.float64)
    counts = data["counts"].astype(np.float64)
    thresholds = data["thresholds"].astype(np.float64)
    validation = pd.read_csv(args.validation)

    rms_sd = []
    for index, population in enumerate(populations):
        fractions = counts[index] / population
        across_run_variance = np.var(fractions, axis=0, ddof=1)
        rms_sd.append(float(np.sqrt(np.mean(across_run_variance))))
    rms_sd = np.asarray(rms_sd)
    slope, _intercept = np.polyfit(np.log(populations), np.log(rms_sd), 1)
    regression = stats.linregress(np.log(populations), np.log(rms_sd))
    slope_critical = stats.t.ppf(0.975, len(populations) - 2)
    slope_ci_low = slope - slope_critical * regression.stderr
    slope_ci_high = slope + slope_critical * regression.stderr

    n500 = validation[validation["pop"] == 500].sort_values(["R_replayed", "rep"])
    representative_row = n500.iloc[(len(n500) - 1) // 2]
    rep = int(representative_row["rep"])
    pidx = int(np.flatnonzero(populations == 500)[0])
    time = np.arange(demand.shape[2])
    signed_arrival = (demand[pidx, rep, :, 0] - demand[pidx, rep, :, 2]) / L_T
    normalized_recruited_service = (
        STEP_RATIO * (counts[pidx, rep, :, 0] - counts[pidx, rep, :, 2]) / 500.0
    )
    task1_threshold = thresholds[pidx, rep, :, 0]
    task3_threshold = thresholds[pidx, rep, :, 2]
    correlation = float(np.corrcoef(signed_arrival, normalized_recruited_service)[0, 1])
    n500_correlations = np.asarray(
        [
            np.corrcoef(
                (demand[pidx, replay, :, 0] - demand[pidx, replay, :, 2]) / L_T,
                STEP_RATIO
                * (counts[pidx, replay, :, 0] - counts[pidx, replay, :, 2])
                / 500.0,
            )[0, 1]
            for replay in range(demand.shape[1])
        ],
        dtype=float,
    )
    if np.isnan(n500_correlations).any():
        raise RuntimeError(
            "Undefined demand--recruitment correlation in n=500 repetitions"
        )
    correlation_low, correlation_high = np.quantile(n500_correlations, [0.025, 0.975])

    summary = pd.DataFrame(
        {
            "population": populations,
            "rms_between_run_sd_recruited_fraction": rms_sd,
            "log_log_slope": [slope] + [np.nan] * (len(populations) - 1),
            "log_log_slope_ci95_low": [slope_ci_low]
            + [np.nan] * (len(populations) - 1),
            "log_log_slope_ci95_high": [slope_ci_high]
            + [np.nan] * (len(populations) - 1),
            "log_log_slope_r_squared": [regression.rvalue**2]
            + [np.nan] * (len(populations) - 1),
            "representative_rep": [rep] + [np.nan] * (len(populations) - 1),
            "representative_R": [representative_row["R_replayed"]]
            + [np.nan] * (len(populations) - 1),
            "signed_arrival_recruitment_correlation": [correlation]
            + [np.nan] * (len(populations) - 1),
            "n500_correlation_median": [np.median(n500_correlations)]
            + [np.nan] * (len(populations) - 1),
            "n500_correlation_2.5pct": [correlation_low]
            + [np.nan] * (len(populations) - 1),
            "n500_correlation_97.5pct": [correlation_high]
            + [np.nan] * (len(populations) - 1),
        }
    )
    args.summary.parent.mkdir(parents=True, exist_ok=True)
    summary.to_csv(args.summary, index=False)
    pd.DataFrame(
        {
            "t": time,
            "signed_target_increment_over_LT": signed_arrival,
            "normalized_net_recruited_service": normalized_recruited_service,
            "mean_stored_threshold_task1": task1_threshold,
            "mean_stored_threshold_task3": task3_threshold,
        }
    ).to_csv(args.representative, index=False)
    pd.DataFrame(
        {
            "rep": np.arange(len(n500_correlations)),
            "signed_arrival_recruitment_correlation": n500_correlations,
        }
    ).to_csv(args.summary.with_name("n500_recruitment_correlations.csv"), index=False)

    plt.rcParams.update(
        {
            "font.family": "serif",
            "font.size": 12.0,
            "axes.titlesize": 12.5,
            "axes.labelsize": 11.5,
            "legend.fontsize": 10.0,
            "xtick.labelsize": 10.5,
            "ytick.labelsize": 10.5,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
        }
    )
    figure, ax = plt.subplots(figsize=(5.0, 3.15))
    figure.subplots_adjust(left=0.16, right=0.97, bottom=0.19, top=0.86)

    rms_sd_pct = 100.0 * rms_sd
    reference_pct = rms_sd_pct[0] * np.sqrt(populations[0] / populations)
    ax.plot(
        populations,
        reference_pct,
        color="#777777",
        linestyle="--",
        linewidth=1.45,
        label=r"$n^{-1/2}$ reference",
    )
    ax.plot(
        populations,
        rms_sd_pct,
        color=PTA_BLUE,
        linewidth=1.55,
        marker="o",
        markersize=5.5,
        markeredgecolor="white",
        markeredgewidth=0.7,
        label="Observed variability",
    )
    for population, value in zip(populations, rms_sd_pct):
        ax.annotate(
            f"{value:.1f}%",
            (population, value),
            xytext=(0, 7),
            textcoords="offset points",
            ha="center",
            fontsize=10.5,
            color=PTA_BLUE,
        )
    ax.set_xscale("log")
    ax.set_xticks(populations, labels=[str(value) for value in populations])
    ax.tick_params(axis="x", labelsize=10.5)
    ax.set_ylim(0, max(rms_sd_pct) * 1.28)
    ax.set_xlabel("Population size, $n$")
    ax.set_ylabel("Variability across repetitions\n(percentage points)")
    ax.set_title("Recruitment variability falls as population grows", fontweight="bold")
    ax.text(
        0.96,
        0.74,
        "Same total capacity\nabout one sixth as variable",
        transform=ax.transAxes,
        ha="right",
        va="top",
        fontsize=10.2,
        color="#333333",
        bbox={
            "boxstyle": "round,pad=0.28",
            "facecolor": "#EAF4FA",
            "edgecolor": "none",
        },
    )
    ax.text(
        175,
        2.56,
        r"Expected $n^{-1/2}$ trend",
        color="#666666",
        fontsize=10.0,
        ha="center",
    )
    ax.text(
        175,
        1.62,
        "Observed variability",
        color=PTA_BLUE,
        fontsize=10.0,
        ha="center",
    )

    ax.grid(True, color=GRID_GRAY, linewidth=0.45, alpha=0.65)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)

    args.out.parent.mkdir(parents=True, exist_ok=True)
    figure.savefig(args.out, bbox_inches="tight")
    figure.savefig(args.out.with_suffix(".png"), dpi=300, bbox_inches="tight")
    plt.close(figure)

    print(summary.to_string(index=False))


if __name__ == "__main__":
    main()
