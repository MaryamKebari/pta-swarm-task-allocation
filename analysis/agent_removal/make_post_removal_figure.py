#!/usr/bin/env python3
"""Create the manuscript figure for valid post-removal agent-loss results."""

from __future__ import annotations

import argparse
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import pandas as pd

METHOD_COLORS = {
    "CT": "#6B6B6B",
    "LFTA": "#D55E00",
    "SBTA": "#CC79A7",
    "SETA": "#009E73",
    "PTA": "#0072B2",
}
MARKERS = {"CT": "o", "LFTA": "s", "SBTA": "D", "SETA": "^"}
STEP_STYLES = {
    1.5: ("#08306B", "o", "-"),
    2.0: ("#2171B5", "s", "--"),
    2.5: ("#6BAED6", "^", "-."),
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--analysis", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    return parser.parse_args()


def style_axis(axis: plt.Axes) -> None:
    axis.grid(True, color="#D9D9D9", linewidth=0.55)
    axis.set_axisbelow(True)
    axis.spines["top"].set_visible(False)
    axis.spines["right"].set_visible(False)
    axis.tick_params(labelsize=11.8)
    axis.set_xticks([0, 10, 20, 30, 40, 50])


def main() -> None:
    args = parse_args()
    severity = pd.read_csv(
        args.analysis / "pta_post_removal_severity_by_step_ratio.csv"
    )
    degradation = pd.read_csv(args.analysis / "pta_degradation_by_step_ratio.csv")
    summary = pd.read_csv(args.analysis / "pta_comparator_summary.csv")

    plt.rcParams.update(
        {
            "font.family": "serif",
            "font.size": 12.5,
            "axes.titlesize": 12.5,
            "axes.labelsize": 12.5,
            "legend.fontsize": 11.5,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
        }
    )
    fig, axes = plt.subplots(2, 2, figsize=(7.25, 4.85), sharex=True)
    step_handles = []
    step_labels = []
    for step_ratio, (color, marker, linestyle) in STEP_STYLES.items():
        group = severity[severity["step_ratio"].eq(step_ratio)].sort_values(
            "kill_percent"
        )
        x = group["kill_percent"]
        (line,) = axes[0, 0].plot(
            x,
            group["median_post_removal_R"],
            color=color,
            marker=marker,
            linestyle=linestyle,
            linewidth=1.8,
            markersize=4.4,
        )
        step_handles.append(line)
        step_labels.append(rf"$s={step_ratio:.1f}$")
        axes[0, 0].fill_between(
            x,
            group["post_removal_R_ci_low"],
            group["post_removal_R_ci_high"],
            color=color,
            alpha=0.09,
            linewidth=0,
        )
        deg = degradation[degradation["step_ratio"].eq(step_ratio)].sort_values(
            "kill_percent"
        )
        fold = 1.0 + deg["median_post_removal_R_degradation_percent"] / 100.0
        fold_low = 1.0 + deg["post_removal_R_degradation_ci_low"] / 100.0
        fold_high = 1.0 + deg["post_removal_R_degradation_ci_high"] / 100.0
        axes[0, 1].plot(
            deg["kill_percent"],
            fold,
            color=color,
            marker=marker,
            linestyle=linestyle,
            linewidth=1.8,
            markersize=4.4,
        )
        axes[0, 1].fill_between(
            deg["kill_percent"],
            fold_low,
            fold_high,
            color=color,
            alpha=0.09,
            linewidth=0,
        )
    axes[0, 0].set_yscale("log")
    axes[0, 0].set_ylabel(r"$R^{\mathrm{post}}$")
    axes[0, 0].set_title(r"(a) $R$ after removal", loc="left", fontweight="bold")

    axes[0, 1].axhline(1, color="#777777", linewidth=0.8)
    axes[0, 1].set_yscale("log")
    axes[0, 1].set_ylabel(r"$R^{\mathrm{post}}/R^{\mathrm{post}}(0)$")
    axes[0, 1].set_title(
        r"(b) Relative change in $R$",
        loc="left",
        fontweight="bold",
    )
    axes[0, 1].annotate(
        "no change",
        xy=(49, 1),
        xytext=(0, 4),
        textcoords="offset points",
        color="#666666",
        fontsize=10.0,
        ha="right",
        va="bottom",
    )

    method_handles = []
    method_labels = []
    for comparator in ("CT", "LFTA", "SBTA", "SETA"):
        group = summary[summary["comparator"].eq(comparator)].sort_values(
            "kill_percent"
        )
        for index, (axis, metric) in enumerate(
            (
                (axes[1, 0], "post_removal_R"),
                (axes[1, 1], "post_removal_R_abs"),
            )
        ):
            (line,) = axis.plot(
                group["kill_percent"],
                group[f"{metric}_median_reduction_percent"],
                color=METHOD_COLORS[comparator],
                marker=MARKERS[comparator],
                linewidth=1.65,
                markersize=4.2,
            )
            if index == 0:
                method_handles.append(line)
                method_labels.append(comparator)
            axis.fill_between(
                group["kill_percent"],
                group[f"{metric}_ci_low"],
                group[f"{metric}_ci_high"],
                color=METHOD_COLORS[comparator],
                alpha=0.09,
                linewidth=0,
            )

    axes[1, 0].axhline(0, color="#777777", linewidth=0.8)
    axes[1, 0].set_ylabel("PTA reduction (%)")
    axes[1, 0].set_title(r"(c) PTA advantage in $R$", loc="left", fontweight="bold")

    axes[1, 1].axhline(0, color="#777777", linewidth=0.8)
    axes[1, 1].set_ylabel("PTA reduction (%)")
    axes[1, 1].set_title(
        r"(d) PTA advantage in $R_{\mathrm{abs}}$", loc="left", fontweight="bold"
    )
    for axis in axes.flat:
        style_axis(axis)
    for axis in axes[1, :]:
        axis.set_xlabel("Agents removed (%)")
    fig.legend(
        step_handles,
        step_labels,
        frameon=False,
        ncol=3,
        loc="upper center",
        bbox_to_anchor=(0.5, 1.005),
        columnspacing=1.2,
        handlelength=2.3,
        title="Step ratio, $s$ (lower = tighter capacity)",
    )
    fig.legend(
        method_handles,
        method_labels,
        frameon=False,
        ncol=4,
        loc="lower center",
        bbox_to_anchor=(0.5, 0.005),
        columnspacing=1.4,
        handlelength=2.3,
        title="Comparator",
    )
    fig.tight_layout(rect=[0, 0.14, 1, 0.88], h_pad=0.85, w_pad=1.05)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    for suffix in ("pdf", "png"):
        fig.savefig(args.output.with_suffix(f".{suffix}"), dpi=300, bbox_inches="tight")
    plt.close(fig)


if __name__ == "__main__":
    main()
