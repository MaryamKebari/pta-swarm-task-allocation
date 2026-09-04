#!/usr/bin/env python3
"""Create one-purpose figures for the manuscript Results section.

Every output is tied to one explicit conclusion.  The script uses only the
audited summary data already produced by the manuscript analyses; it does not
rerun simulations or change any reported result.
"""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
ANALYSIS = ROOT / "data" / "processed"
FIGURES = ROOT / "figures"
ALLOCATION = ANALYSIS / "allocation"
REMOVAL = ANALYSIS / "removal"
FEEDBACK = ANALYSIS / "feedback"
ABLATION = ANALYSIS / "ablation"

PTA_BLUE = "#0072B2"
GRAY = "#666666"
RANGE_COLORS = {"HM": "#2A9D8F", "HT1": "#C58B20", "HT2": "#9B6FC3"}
RANGE_MARKERS = {"HM": "o", "HT1": "s", "HT2": "^"}
RANGE_LINES = {"HM": "-", "HT1": "--", "HT2": ":"}
METHOD_COLORS = {
    "CT": "#6B6B6B",
    "LFTA": "#D55E00",
    "SBTA": "#CC79A7",
    "SETA": "#009E73",
}
METHOD_MARKERS = {"CT": "o", "LFTA": "s", "SBTA": "D", "SETA": "^"}
STEP_STYLES = {
    1.5: ("#08306B", "o", "-"),
    2.0: ("#2171B5", "s", "--"),
    2.5: ("#6BAED6", "^", "-."),
}
PATTERN_NAMES = {
    "random": "Non-iterative\ngradual",
    "scurve": "Iterative\ngradual",
    "sharp": "Non-iterative\nnon-gradual",
    "zigzag": "Iterative\nnon-gradual",
}
BOOTSTRAP_DRAWS = 20_000


def configure_style() -> None:
    plt.rcParams.update(
        {
            "font.family": "serif",
            "font.size": 12.0,
            "axes.titlesize": 12.8,
            "axes.labelsize": 12.3,
            "xtick.labelsize": 11.2,
            "ytick.labelsize": 11.2,
            "legend.fontsize": 11.0,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
        }
    )


def save(fig: plt.Figure, stem: str) -> None:
    FIGURES.mkdir(parents=True, exist_ok=True)
    fig.savefig(FIGURES / f"{stem}.pdf", bbox_inches="tight")
    fig.savefig(FIGURES / f"{stem}.png", dpi=300, bbox_inches="tight")
    plt.close(fig)


def bootstrap_median_log(values: np.ndarray, seed: int) -> tuple[float, float, float]:
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


def reduction(log_value: float | np.ndarray) -> float | np.ndarray:
    return 100.0 * (1.0 - np.exp(log_value))


def paired_summary(
    values: pd.DataFrame,
    *,
    focal: str,
    comparator: str,
    seed: int,
) -> dict[str, float | int]:
    log_values = np.log(
        values[focal].to_numpy(float) / values[comparator].to_numpy(float)
    )
    median, low, high = bootstrap_median_log(log_values, seed)
    return {
        "focal_lower": int(np.sum(log_values < 0)),
        "total": len(log_values),
        "median_reduction_percent": float(reduction(median)),
        "ci_low": float(reduction(high)),
        "ci_high": float(reduction(low)),
    }


def factor_effects() -> pd.DataFrame:
    pta = pd.read_csv(ALLOCATION / "conditionwise_preferred_pta.csv")
    alternative = pd.read_csv(ALLOCATION / "conditionwise_best_alternative.csv")
    keys = ["pop", "n", "step_ratio", "pattern"]
    paired = pta[keys + ["R"]].merge(
        alternative[keys + ["R"]], on=keys, suffixes=("_pta", "_alternative")
    )
    paired["log_ratio"] = np.log(paired["R_pta"] / paired["R_alternative"])
    rows: list[dict[str, object]] = []
    factors = [
        ("pattern", ["random", "scurve", "sharp", "zigzag"]),
        ("step_ratio", [1.5, 2.0, 2.5]),
    ]
    for factor_index, (factor, levels) in enumerate(factors):
        for level_index, level in enumerate(levels):
            group = paired.loc[paired[factor].eq(level)]
            median, low, high = bootstrap_median_log(
                group["log_ratio"].to_numpy(float),
                seed=20260901 + factor_index * 100 + level_index,
            )
            rows.append(
                {
                    "factor": factor,
                    "level": level,
                    "operating_conditions": len(group),
                    "median_pta_R": float(group["R_pta"].median()),
                    "median_alternative_R": float(group["R_alternative"].median()),
                    "median_reduction_percent": float(reduction(median)),
                    "ci_low": float(reduction(high)),
                    "ci_high": float(reduction(low)),
                }
            )
    result = pd.DataFrame(rows)
    result.to_csv(ALLOCATION / "conclusion_specific_factor_summary.csv", index=False)
    return result


def plot_demand_effect(summary: pd.DataFrame) -> None:
    data = summary.loc[summary["factor"].eq("pattern")].copy()
    order = ["random", "scurve", "sharp", "zigzag"]
    data = data.set_index("level").loc[order].reset_index()
    y = np.arange(len(data))
    values = data["median_reduction_percent"].to_numpy(float)
    low = data["ci_low"].to_numpy(float)
    high = data["ci_high"].to_numpy(float)
    fig, ax = plt.subplots(figsize=(7.1, 3.65))
    ax.errorbar(
        values,
        y,
        xerr=np.vstack((values - low, high - values)),
        fmt="o",
        color=PTA_BLUE,
        markerfacecolor=PTA_BLUE,
        markeredgecolor="#111111",
        capsize=4,
        linewidth=1.5,
        markersize=7,
    )
    ax.axvline(0, color=GRAY, linestyle="--", linewidth=0.9)
    ax.set_yticks(y, [PATTERN_NAMES[value].replace("\n", " ") for value in order])
    ax.invert_yaxis()
    ax.set_xlabel("Median PTA reduction in paired imbalance, $R$ (%)")
    ax.set_title("PTA advantage by demand class", loc="left", fontweight="semibold")
    ax.grid(axis="x", color="#D9D9D9", linewidth=0.6)
    ax.spines[["top", "right", "left"]].set_visible(False)
    for yi, value in zip(y, values):
        ax.text(value + 1.5, yi, f"{value:.1f}%", va="center", fontsize=10.8)
    fig.tight_layout()
    save(fig, "demand_class_pta_advantage")


def plot_step_effects(summary: pd.DataFrame) -> None:
    data = summary.loc[summary["factor"].eq("step_ratio")].sort_values("level")
    x = np.arange(len(data))

    fig, ax = plt.subplots(figsize=(6.7, 3.45))
    ax.plot(
        x,
        data["median_pta_R"],
        color=PTA_BLUE,
        marker="o",
        linewidth=2.0,
        markersize=7,
    )
    ax.set_xticks(x, [f"{value:.1f}" for value in data["level"]])
    ax.set_xlabel("Step ratio, $s$")
    ax.set_ylabel("Median paired imbalance, $R$")
    ax.set_title(
        "PTA error decreases as capacity increases", loc="left", fontweight="semibold"
    )
    ax.grid(axis="y", color="#D9D9D9", linewidth=0.6)
    ax.spines[["top", "right"]].set_visible(False)
    for index, (xi, value) in enumerate(zip(x, data["median_pta_R"])):
        if index == len(data) - 1:
            ax.text(
                xi,
                value + 0.020,
                f"{value:.3f}",
                ha="center",
                va="bottom",
                fontsize=10.5,
            )
        else:
            ax.text(
                xi, value - 0.045, f"{value:.3f}", ha="center", va="top", fontsize=10.5
            )
    fig.tight_layout()
    save(fig, "step_ratio_pta_error")

    values = data["median_reduction_percent"].to_numpy(float)
    low = data["ci_low"].to_numpy(float)
    high = data["ci_high"].to_numpy(float)
    fig, ax = plt.subplots(figsize=(6.7, 3.45))
    ax.errorbar(
        x,
        values,
        yerr=np.vstack((values - low, high - values)),
        color=PTA_BLUE,
        marker="o",
        linewidth=2.0,
        markersize=7,
        capsize=4,
    )
    ax.axhline(0, color=GRAY, linestyle="--", linewidth=0.9)
    ax.set_xticks(x, [f"{value:.1f}" for value in data["level"]])
    ax.set_xlabel("Step ratio, $s$")
    ax.set_ylabel("Median PTA reduction in $R$ (%)")
    ax.set_title(
        "PTA advantage is smallest at the highest tested capacity",
        loc="left",
        fontweight="semibold",
    )
    ax.grid(axis="y", color="#D9D9D9", linewidth=0.6)
    ax.spines[["top", "right"]].set_visible(False)
    for xi, value in zip(x, values):
        ax.text(xi, value + 2.0, f"{value:.1f}%", ha="center", fontsize=10.5)
    fig.tight_layout()
    save(fig, "step_ratio_pta_advantage")


def plot_scaling() -> None:
    summary = pd.read_csv(ALLOCATION / "population_task_scaling_summary.csv")
    specs = [
        (
            "pop",
            [50, 100, 500, 1000],
            "Population size, $n$",
            "population_pta_advantage",
            "PTA advantage across population sizes",
        ),
        (
            "n",
            [4, 8, 12],
            "Number of tasks, $m$",
            "task_count_pta_advantage",
            "PTA advantage across task counts",
        ),
    ]
    for factor, levels, xlabel, stem, title in specs:
        fig, ax = plt.subplots(figsize=(6.75, 3.65))
        for threshold_range in ("HM", "HT1", "HT2"):
            data = (
                summary.loc[
                    summary["factor"].eq(factor)
                    & summary["threshold_range"].eq(threshold_range)
                ]
                .set_index("level")
                .loc[levels]
            )
            x = np.arange(len(levels), dtype=float)
            y = data["median_reduction_percent"].to_numpy(float)
            low = data["ci_low"].to_numpy(float)
            high = data["ci_high"].to_numpy(float)
            ax.errorbar(
                x,
                y,
                yerr=np.vstack((y - low, high - y)),
                color=RANGE_COLORS[threshold_range],
                marker=RANGE_MARKERS[threshold_range],
                linestyle=RANGE_LINES[threshold_range],
                linewidth=2.0,
                markersize=6.5,
                capsize=3.5,
                label=threshold_range,
            )
        ax.axhline(0, color=GRAY, linestyle="--", linewidth=0.9)
        ax.set_xticks(np.arange(len(levels)), [str(level) for level in levels])
        ax.set_xlabel(xlabel)
        ax.set_ylabel("Median PTA reduction in $R$ (%)")
        ax.set_title(title, loc="left", fontweight="semibold")
        ax.grid(axis="y", color="#D9D9D9", linewidth=0.6)
        ax.spines[["top", "right"]].set_visible(False)
        ax.legend(title="Threshold range", frameon=False, ncol=3, loc="lower center")
        fig.tight_layout()
        save(fig, stem)


def plot_threshold_range_preference() -> None:
    winners = pd.read_csv(ALLOCATION / "winner_by_condition.csv")
    counts = (
        winners.groupby(["pattern", "step_ratio", "threshold_range"])
        .size()
        .rename("conditions")
        .reset_index()
    )
    patterns = ["random", "scurve", "sharp", "zigzag"]
    steps = [1.5, 2.0, 2.5]
    fig, axes = plt.subplots(
        2, 2, figsize=(7.0, 5.3), sharey=True, constrained_layout=True
    )
    for panel, (ax, pattern) in enumerate(zip(axes.flat, patterns)):
        bottom = np.zeros(len(steps))
        for threshold_range in ("HM", "HT1", "HT2"):
            values = []
            for step in steps:
                row = counts.loc[
                    counts["pattern"].eq(pattern)
                    & counts["step_ratio"].eq(step)
                    & counts["threshold_range"].eq(threshold_range),
                    "conditions",
                ]
                values.append(int(row.iloc[0]) if len(row) else 0)
            values_array = np.asarray(values, dtype=float)
            ax.bar(
                np.arange(len(steps)),
                100.0 * values_array / 12.0,
                bottom=bottom,
                color=RANGE_COLORS[threshold_range],
                edgecolor="white",
                linewidth=0.8,
                label=threshold_range,
            )
            for xi, value, base in zip(np.arange(len(steps)), values_array, bottom):
                if value >= 2:
                    ax.text(
                        xi,
                        base + 50.0 * value / 12.0,
                        f"{int(value)}/12",
                        ha="center",
                        va="center",
                        fontsize=9.5,
                        color="white" if threshold_range != "HT1" else "#2B220E",
                        fontweight="semibold",
                    )
            bottom += 100.0 * values_array / 12.0
        ax.set_xticks(np.arange(len(steps)), [f"{step:.1f}" for step in steps])
        ax.set_xlabel("Step ratio, $s$")
        ax.set_title(
            f"({chr(97 + panel)}) {PATTERN_NAMES[pattern].replace(chr(10), ' ')}",
            loc="left",
            fontweight="semibold",
        )
        ax.spines[["top", "right"]].set_visible(False)
        ax.grid(axis="y", color="#E2E2E2", linewidth=0.5)
    axes[0, 0].set_ylabel("Preferred range (%)")
    axes[1, 0].set_ylabel("Preferred range (%)")
    handles, labels = axes[0, 0].get_legend_handles_labels()
    fig.legend(
        handles,
        labels,
        title="Threshold range",
        frameon=False,
        ncol=3,
        loc="lower center",
        bbox_to_anchor=(0.5, -0.035),
    )
    save(fig, "threshold_range_preference")


def plot_term_role(term: str, color: str, stem: str) -> None:
    summary = pd.read_csv(ABLATION / "ablation_selected_path_summary.csv")
    data = summary.loc[summary["added_term"].eq(term)].copy()
    path_order = ["Iterative non-gradual", "Sustained plateau"]
    fig, axes = plt.subplots(1, 2, figsize=(7.8, 3.65), sharey=True)
    for panel, (ax, path) in enumerate(zip(axes, path_order)):
        group = data.loc[data["path"].eq(path)].reset_index(drop=True)
        x = np.arange(len(group))
        effect = group["effect_percent"].to_numpy(float)
        low = group["ci95_low"].to_numpy(float)
        high = group["ci95_high"].to_numpy(float)
        ax.axhline(0, color=GRAY, linewidth=0.9)
        ax.errorbar(
            x,
            effect,
            yerr=np.vstack((effect - low, high - effect)),
            fmt="D",
            color="#111111",
            markerfacecolor=color,
            markeredgecolor="#111111",
            markersize=7,
            capsize=4,
            linewidth=1.5,
        )
        labels = [value.replace("full PTA", "PTA") for value in group["comparison"]]
        ax.set_xticks(x, labels)
        ax.set_xlim(-0.10, len(group) - 0.90)
        ax.set_title(f"({chr(97 + panel)}) {path}", loc="left", fontweight="semibold")
        ax.grid(axis="y", color="#D9D9D9", linewidth=0.6)
        ax.spines[["top", "right"]].set_visible(False)
        for xi, value in zip(x, effect):
            label = "no change" if abs(value) < 0.05 else f"{value:.1f}%"
            ax.text(xi, value + 5.0, label, ha="center", fontsize=10.5)
    axes[0].set_ylabel(f"Error reduction after adding {term} (%)")
    axes[0].set_ylim(
        min(-5.0, float(data["ci95_low"].min()) - 5.0),
        float(data["ci95_high"].max()) + 12.0,
    )
    fig.subplots_adjust(left=0.11, right=0.98, bottom=0.18, top=0.84, wspace=0.32)
    save(fig, stem)


def style_removal_axis(ax: plt.Axes) -> None:
    ax.set_xticks([0, 10, 20, 30, 40, 50])
    ax.grid(True, color="#D9D9D9", linewidth=0.55)
    ax.set_axisbelow(True)
    ax.spines[["top", "right"]].set_visible(False)


def plot_removal() -> None:
    degradation = pd.read_csv(REMOVAL / "pta_degradation_by_step_ratio.csv")
    comparison = pd.read_csv(REMOVAL / "pta_comparator_summary.csv")

    fig, ax = plt.subplots(figsize=(6.8, 3.75))
    for step_ratio, (color, marker, line) in STEP_STYLES.items():
        group = degradation.loc[degradation["step_ratio"].eq(step_ratio)].sort_values(
            "kill_percent"
        )
        fold = 1.0 + group["median_post_removal_R_degradation_percent"] / 100.0
        low = 1.0 + group["post_removal_R_degradation_ci_low"] / 100.0
        high = 1.0 + group["post_removal_R_degradation_ci_high"] / 100.0
        ax.plot(
            group["kill_percent"],
            fold,
            color=color,
            marker=marker,
            linestyle=line,
            linewidth=2.0,
            label=rf"$s={step_ratio:.1f}$",
        )
        ax.fill_between(
            group["kill_percent"], low, high, color=color, alpha=0.10, linewidth=0
        )
    ax.axhline(1.0, color=GRAY, linewidth=0.9, linestyle="--")
    ax.set_yscale("log")
    ax.set_xlabel("Agents removed (%)")
    ax.set_ylabel(r"$R^{\mathrm{post}}/R^{\mathrm{post}}(0)$")
    ax.set_title(
        "PTA deterioration relative to no removal", loc="left", fontweight="semibold"
    )
    ax.legend(title="Step ratio", frameon=False, ncol=3)
    style_removal_axis(ax)
    fig.tight_layout()
    save(fig, "agent_removal_deterioration")

    for metric, stem, title in [
        (
            "post_removal_R",
            "agent_removal_paired_advantage",
            "PTA paired imbalance advantage after removal",
        ),
        (
            "post_removal_R_abs",
            "agent_removal_raw_advantage",
            "PTA raw residual advantage after removal",
        ),
    ]:
        fig, ax = plt.subplots(figsize=(6.8, 3.75))
        for comparator in ("CT", "LFTA", "SBTA", "SETA"):
            group = comparison.loc[comparison["comparator"].eq(comparator)].sort_values(
                "kill_percent"
            )
            x = group["kill_percent"]
            y = group[f"{metric}_median_reduction_percent"]
            low = group[f"{metric}_ci_low"]
            high = group[f"{metric}_ci_high"]
            ax.plot(
                x,
                y,
                color=METHOD_COLORS[comparator],
                marker=METHOD_MARKERS[comparator],
                linewidth=1.9,
                label=comparator,
            )
            ax.fill_between(
                x, low, high, color=METHOD_COLORS[comparator], alpha=0.10, linewidth=0
            )
        ax.axhline(0, color=GRAY, linewidth=0.9, linestyle="--")
        ax.set_xlabel("Agents removed (%)")
        ax.set_ylabel("PTA reduction (%)")
        ax.set_title(title, loc="left", fontweight="semibold")
        ax.legend(title="Comparator", frameon=False, ncol=4)
        style_removal_axis(ax)
        fig.tight_layout()
        save(fig, stem)


def make_selected_configuration_tables() -> None:
    removal = pd.read_csv(REMOVAL / "condition_means.csv")
    keys = ["pop", "n", "step_ratio", "pattern", "kill_percent"]
    selected = {
        "PTA": "PTA-HT1-clamped-Agent",
        "CT": "CT-HM",
        "LFTA": "LFTA-HM",
        "SBTA": "SBTA-HT2",
        "SETA": "SETA-HT1-latent",
    }
    columns = []
    for family, config in selected.items():
        frame = removal.loc[
            removal["config"].eq(config), keys + ["post_removal_R"]
        ].copy()
        frame = frame.rename(columns={"post_removal_R": family})
        columns.append(frame)
    paired = columns[0]
    for frame in columns[1:]:
        paired = paired.merge(frame, on=keys, validate="one_to_one")
    records = []
    seed = 20260920
    for severity in (5, 10, 20, 30, 40, 50):
        group = paired.loc[paired["kill_percent"].eq(severity)]
        for comparator in ("CT", "LFTA", "SBTA", "SETA"):
            result = paired_summary(
                group, focal="PTA", comparator=comparator, seed=seed
            )
            seed += 1
            records.append(
                {"kill_percent": severity, "comparator": comparator, **result}
            )
    pd.DataFrame(records).to_csv(
        REMOVAL / "selected_configuration_comparison.csv", index=False
    )

    feedback = pd.read_csv(FEEDBACK / "condition_means.csv")
    feedback_keys = [
        "n",
        "step_ratio",
        "pattern",
        "feedback_noise_alpha",
        "feedback_bias_alpha",
    ]
    selectors = {
        "PTA": ("PTA", "PTA-HT1-AgentPID", "clamped"),
        "CT": ("CT", "CT-HM", "clamped"),
        "LFTA": ("LFTA", "LFTA-HM", "clamped"),
        "SBTA": ("SBTA", "SBTA-HT2", "clamped"),
        "SETA": ("SETA", "SETA-HT1", "latent"),
    }
    selected_frames = []
    for label, (family, method, mode) in selectors.items():
        frame = feedback.loc[
            feedback["family"].eq(family)
            & feedback["method_label"].eq(method)
            & feedback["threshold_mode"].eq(mode),
            feedback_keys + ["R", "R_abs"],
        ].copy()
        frame = frame.rename(columns={"R": f"R_{label}", "R_abs": f"R_abs_{label}"})
        selected_frames.append(frame)
    feedback_paired = selected_frames[0]
    for frame in selected_frames[1:]:
        feedback_paired = feedback_paired.merge(
            frame, on=feedback_keys, validate="one_to_one"
        )
    strongest = feedback_paired.loc[
        feedback_paired["feedback_noise_alpha"].eq(0.4)
        & feedback_paired["feedback_bias_alpha"].eq(0.2)
    ]
    records = []
    seed = 20260940
    for comparator in ("CT", "LFTA", "SBTA", "SETA"):
        paired_r = paired_summary(
            strongest, focal="R_PTA", comparator=f"R_{comparator}", seed=seed
        )
        seed += 1
        paired_raw = paired_summary(
            strongest, focal="R_abs_PTA", comparator=f"R_abs_{comparator}", seed=seed
        )
        seed += 1
        records.append(
            {
                "comparator": comparator,
                **{f"R_{key}": value for key, value in paired_r.items()},
                **{f"R_abs_{key}": value for key, value in paired_raw.items()},
            }
        )
    pd.DataFrame(records).to_csv(
        FEEDBACK / "selected_configuration_strongest.csv", index=False
    )

    clean = feedback_paired.loc[
        feedback_paired["feedback_noise_alpha"].eq(0.0)
        & feedback_paired["feedback_bias_alpha"].eq(0.0),
        ["n", "step_ratio", "pattern", "R_PTA", "R_abs_PTA"],
    ]
    noisy = strongest[["n", "step_ratio", "pattern", "R_PTA", "R_abs_PTA"]]
    own = clean.merge(
        noisy, on=["n", "step_ratio", "pattern"], suffixes=("_clean", "_noisy")
    )
    own_records = []
    for index, metric in enumerate(("R_PTA", "R_abs_PTA")):
        log_values = np.log(own[f"{metric}_noisy"] / own[f"{metric}_clean"])
        median, low, high = bootstrap_median_log(
            log_values.to_numpy(float), 20260960 + index
        )
        own_records.append(
            {
                "metric": "R" if metric == "R_PTA" else "R_abs",
                "operating_conditions": len(own),
                "median_increase_percent": float(100.0 * (np.exp(median) - 1.0)),
                "ci_low": float(100.0 * (np.exp(low) - 1.0)),
                "ci_high": float(100.0 * (np.exp(high) - 1.0)),
            }
        )
    pd.DataFrame(own_records).to_csv(
        FEEDBACK / "selected_configuration_own_degradation.csv", index=False
    )


def extra_figures() -> None:
    """Paper figures produced by experiment-specific scripts."""
    subprocess.run(
        [sys.executable, "analysis/allocation/winner_atlas.py"],
        cwd=ROOT,
        check=True,
    )
    subprocess.run(
        [sys.executable, "analysis/feedback/compare.py"],
        cwd=ROOT,
        check=True,
    )
    subprocess.run(
        [
            sys.executable,
            "analysis/population/plot.py",
            "--trajectories",
            "data/processed/population/population_mechanism_trajectories.npz",
            "--validation",
            "data/processed/population/replay_validation.csv",
            "--out",
            "figures/population_mechanism_diagnostic.pdf",
            "--summary",
            "data/processed/population/population_mechanism_summary.csv",
            "--representative",
            "data/processed/population/representative_timeseries.csv",
        ],
        cwd=ROOT,
        check=True,
    )


def main() -> None:
    configure_style()
    factors = factor_effects()
    plot_demand_effect(factors)
    plot_step_effects(factors)
    plot_scaling()
    plot_threshold_range_preference()
    plot_term_role("D", "#D55E00", "derivative_term_role")
    plot_term_role("I", "#009E73", "integral_term_role")
    plot_removal()
    make_selected_configuration_tables()
    extra_figures()
    print("Wrote paper figures to figures/")


if __name__ == "__main__":
    main()
