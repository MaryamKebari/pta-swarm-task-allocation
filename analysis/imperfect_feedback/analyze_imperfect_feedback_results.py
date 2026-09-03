#!/usr/bin/env python3
"""Analyze the audited frozen-reference imperfect-feedback grid.

The script uses the 100-repetition condition means produced by
``audit_and_summarize.py``.  Effects are expressed as log ratios before
aggregation so that improvements and degradations have a symmetric scale.
Bootstrap intervals resample operating conditions as clusters and retain all
associated configurations, thereby preserving the dependence created by
shared target paths within each condition.
"""

from pathlib import Path

import matplotlib as mpl
mpl.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from matplotlib.colors import TwoSlopeNorm


REPO_ROOT = Path(__file__).resolve().parents[2]
HERE = REPO_ROOT / "data" / "processed" / "imperfect_feedback"
INPUT = HERE / "condition_means.csv"
FIGURE_DIR = REPO_ROOT / "figures"
FIGURE_DIR.mkdir(parents=True, exist_ok=True)

METHOD_ORDER = ["CT", "LFTA", "SBTA", "SETA", "PTA"]
COMPARATOR_ORDER = ["CT", "LFTA", "SBTA", "SETA"]
METHOD_COLORS = {
    "CT": "#6B6B6B",
    "LFTA": "#D55E00",
    "SBTA": "#CC79A7",
    "SETA": "#009E73",
    "PTA": "#0072B2",
}
ALPHAS = [0.00, 0.05, 0.10, 0.20, 0.40]
BETAS = [0.00, 0.05, 0.10, 0.20]


def clustered_bootstrap_median(frame, value_col, cluster_cols, rng, draws=10_000):
    """Bootstrap a median by resampling complete operating conditions."""
    values = frame[value_col].to_numpy(dtype=float)
    point = float(np.median(values))
    clusters = [
        group[value_col].to_numpy(dtype=float)
        for _, group in frame.groupby(cluster_cols, sort=True, dropna=False)
    ]
    if len(clusters) == 1:
        return point, point, point, 1

    cluster_sizes = {len(cluster) for cluster in clusters}
    estimates = np.empty(draws, dtype=float)
    chunk = 250
    if len(cluster_sizes) == 1:
        matrix = np.stack(clusters)
        for start in range(0, draws, chunk):
            stop = min(start + chunk, draws)
            indices = rng.integers(
                0, len(clusters), size=(stop - start, len(clusters))
            )
            samples = matrix[indices].reshape(stop - start, -1)
            estimates[start:stop] = np.median(samples, axis=1)
    else:
        for draw in range(draws):
            indices = rng.integers(0, len(clusters), size=len(clusters))
            estimates[draw] = np.median(
                np.concatenate([clusters[index] for index in indices])
            )
    lo, hi = np.percentile(estimates, [2.5, 97.5])
    return point, float(lo), float(hi), len(clusters)


def percent_from_log_degradation(value):
    return 100.0 * np.expm1(value)


def percent_reduction_from_log_ratio(value):
    return 100.0 * (1.0 - np.exp(value))


def summarize_effects(frame, group_cols, effect_cols, cluster_cols, seed):
    rows = []
    rng = np.random.default_rng(seed)
    for group, subset in frame.groupby(group_cols, sort=True, dropna=False):
        if not isinstance(group, tuple):
            group = (group,)
        row = dict(zip(group_cols, group))
        row["units"] = len(subset)
        for effect_col in effect_cols:
            point, lo, hi, clusters = clustered_bootstrap_median(
                subset, effect_col, cluster_cols, rng
            )
            row["operating_conditions"] = clusters
            stem = effect_col.removeprefix("log_")
            row[f"median_log_{stem}"] = point
            row[f"ci_low_log_{stem}"] = lo
            row[f"ci_high_log_{stem}"] = hi
        rows.append(row)
    return pd.DataFrame(rows)


def exact_configuration_id(frame):
    gain = frame["gain_scheme"].fillna("-")
    return (
        frame["family"].astype(str)
        + "|"
        + frame["threshold_range"].astype(str)
        + "|"
        + frame["threshold_mode"].astype(str)
        + "|"
        + gain.astype(str)
    )


def make_own_clean_pairs(data):
    config_cols = [
        "family",
        "method_label",
        "threshold_mode",
        "threshold_range",
        "gain_scheme",
    ]
    op_cols = ["n", "step_ratio", "pattern"]
    clean = data.loc[
        (data.feedback_noise_alpha == 0) & (data.feedback_bias_alpha == 0),
        config_cols + op_cols + ["R", "R_abs"],
    ].rename(columns={"R": "R_clean", "R_abs": "R_abs_clean"})
    paired = data.merge(clean, on=config_cols + op_cols, how="left", validate="many_to_one")
    paired["configuration"] = exact_configuration_id(paired)
    paired["log_R_degradation"] = np.log(paired["R"] / paired["R_clean"])
    paired["log_R_abs_degradation"] = np.log(paired["R_abs"] / paired["R_abs_clean"])
    return paired


def make_pta_comparisons(data):
    pta = data.loc[data.family == "PTA"].copy()
    op_severity = [
        "n",
        "step_ratio",
        "pattern",
        "threshold_range",
        "feedback_noise_alpha",
        "feedback_bias_alpha",
    ]
    outputs = []
    for comparator in COMPARATOR_ORDER:
        baseline = data.loc[data.family == comparator].copy()
        join_cols = list(op_severity)
        if comparator == "SETA":
            join_cols.append("threshold_mode")
        baseline = baseline[join_cols + ["R", "R_abs"]].rename(
            columns={"R": "R_comparator", "R_abs": "R_abs_comparator"}
        )
        comparison = pta.merge(baseline, on=join_cols, how="inner", validate="many_to_one")
        comparison["comparator"] = comparator
        comparison["log_R_ratio"] = np.log(comparison["R"] / comparison["R_comparator"])
        comparison["log_R_abs_ratio"] = np.log(
            comparison["R_abs"] / comparison["R_abs_comparator"]
        )
        outputs.append(comparison)
    return pd.concat(outputs, ignore_index=True)


def format_percent(value):
    if abs(value) < 0.05:
        return "0"
    rounded = int(np.rint(value))
    return "0" if rounded == 0 else str(rounded)


def heatmap(
    ax,
    matrix,
    title,
    cmap,
    norm,
    annotation_transform,
    annotation_suffix="",
    annotation_fontsize=15.5,
    title_color="black",
):
    image = ax.imshow(matrix, aspect="auto", cmap=cmap, norm=norm)
    ax.set_title(title, color=title_color, fontweight="bold", loc="left")
    ax.set_xticks(range(len(ALPHAS)), [f"{value:g}" for value in ALPHAS])
    ax.set_yticks(range(len(BETAS)), [f"{value:g}" for value in BETAS])
    ax.set_xlabel(r"Noise level, $\alpha$")
    for row in range(len(BETAS)):
        for col in range(len(ALPHAS)):
            raw = matrix[row, col]
            shown = annotation_transform(raw)
            rgba = image.cmap(image.norm(raw))
            luminance = 0.2126 * rgba[0] + 0.7152 * rgba[1] + 0.0722 * rgba[2]
            ax.text(
                col,
                row,
                format_percent(shown) + annotation_suffix,
                ha="center",
                va="center",
                fontsize=annotation_fontsize,
                color="black" if luminance > 0.55 else "white",
            )
    return image


def plot_own_clean(summary):
    matrices = {}
    for family in METHOD_ORDER:
        subset = summary.loc[summary.family == family]
        matrix = subset.pivot(
            index="feedback_bias_alpha",
            columns="feedback_noise_alpha",
            values="median_log_R_degradation",
        ).reindex(index=BETAS, columns=ALPHAS).to_numpy()
        matrices[family] = matrix
    vmax = max(float(np.nanmax(np.abs(matrix))) for matrix in matrices.values())
    norm = TwoSlopeNorm(vmin=-vmax, vcenter=0, vmax=vmax)
    fig = plt.figure(figsize=(6.8, 7.35))
    grid = fig.add_gridspec(
        3,
        4,
        left=0.07,
        right=0.88,
        bottom=0.07,
        top=0.96,
        wspace=0.36,
        hspace=0.58,
    )
    axes = {
        "PTA": fig.add_subplot(grid[0, 1:3]),
        "CT": fig.add_subplot(grid[1, 0:2]),
        "LFTA": fig.add_subplot(grid[1, 2:4]),
        "SBTA": fig.add_subplot(grid[2, 0:2]),
        "SETA": fig.add_subplot(grid[2, 2:4]),
    }
    last_image = None
    panel_letters = {"PTA": "a", "CT": "b", "LFTA": "c", "SBTA": "d", "SETA": "e"}
    for family in ["PTA", "CT", "LFTA", "SBTA", "SETA"]:
        ax = axes[family]
        last_image = heatmap(
            ax,
            matrices[family],
            f"({panel_letters[family]}) {family}",
            "RdBu_r",
            norm,
            percent_from_log_degradation,
            "",
            annotation_fontsize=15.5,
            title_color=METHOD_COLORS[family],
        )
        if family in ("PTA", "CT", "SBTA"):
            ax.set_ylabel(r"Bias level, $\beta$")
        else:
            ax.set_ylabel("")
            ax.tick_params(axis="y", labelleft=False)
    colorbar_axis = fig.add_axes([0.91, 0.18, 0.018, 0.64])
    colorbar = fig.colorbar(last_image, cax=colorbar_axis)
    colorbar.set_label(r"Median $\log(R/R_{\mathrm{clean}})$")
    fig.savefig(FIGURE_DIR / "imperfect_feedback_own_clean.pdf", bbox_inches="tight")
    fig.savefig(FIGURE_DIR / "imperfect_feedback_own_clean.png", dpi=300, bbox_inches="tight")
    plt.close(fig)


def plot_pta_advantage(summary):
    matrices = {}
    for comparator in COMPARATOR_ORDER:
        subset = summary.loc[summary.comparator == comparator]
        matrix = subset.pivot(
            index="feedback_bias_alpha",
            columns="feedback_noise_alpha",
            values="median_log_R_ratio",
        ).reindex(index=BETAS, columns=ALPHAS).to_numpy()
        matrices[comparator] = matrix
    vmax = max(float(np.nanmax(np.abs(matrix))) for matrix in matrices.values())
    norm = TwoSlopeNorm(vmin=-vmax, vcenter=0, vmax=vmax)
    fig, axes = plt.subplots(2, 2, figsize=(6.3, 6.3), constrained_layout=True)
    last_image = None
    for index, comparator in enumerate(COMPARATOR_ORDER):
        ax = axes.ravel()[index]
        last_image = heatmap(
            ax,
            matrices[comparator],
            f"({chr(ord('a') + index)}) {comparator}",
            "RdBu_r",
            norm,
            percent_reduction_from_log_ratio,
            "",
            title_color=METHOD_COLORS[comparator],
        )
        if index in (0, 2):
            ax.set_ylabel(r"Bias level, $\beta$")
        else:
            ax.set_ylabel("")
    colorbar = fig.colorbar(last_image, ax=axes.ravel().tolist(), shrink=0.85, pad=0.025)
    colorbar.set_label(r"Median $\log(R_{\mathrm{PTA}}/R_{\mathrm{comparator}})$")
    fig.savefig(FIGURE_DIR / "imperfect_feedback_pta_comparators.pdf", bbox_inches="tight")
    fig.savefig(FIGURE_DIR / "imperfect_feedback_pta_comparators.png", dpi=300, bbox_inches="tight")
    plt.close(fig)


def main():
    mpl.rcParams.update(
        {
            "font.family": "serif",
            "font.size": 15.5,
            "axes.titlesize": 16.0,
            "axes.labelsize": 15.0,
            "xtick.labelsize": 14.0,
            "ytick.labelsize": 14.0,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
        }
    )
    data = pd.read_csv(INPUT)
    data["gain_scheme"] = data["gain_scheme"].fillna("-")

    own = make_own_clean_pairs(data)
    own.to_csv(HERE / "own_clean_condition_effects.csv", index=False)
    own_summary = summarize_effects(
        own,
        ["family", "feedback_noise_alpha", "feedback_bias_alpha"],
        ["log_R_degradation", "log_R_abs_degradation"],
        ["n", "step_ratio", "pattern"],
        seed=20260808,
    )
    for metric in ("R_degradation", "R_abs_degradation"):
        own_summary[f"median_{metric}_percent"] = percent_from_log_degradation(
            own_summary[f"median_log_{metric}"]
        )
        own_summary[f"ci_low_{metric}_percent"] = percent_from_log_degradation(
            own_summary[f"ci_low_log_{metric}"]
        )
        own_summary[f"ci_high_{metric}_percent"] = percent_from_log_degradation(
            own_summary[f"ci_high_log_{metric}"]
        )
    own_summary.to_csv(HERE / "own_clean_summary.csv", index=False)

    comparisons = make_pta_comparisons(data)
    comparisons.to_csv(HERE / "pta_comparator_condition_effects.csv", index=False)
    comparison_summary = summarize_effects(
        comparisons,
        ["comparator", "feedback_noise_alpha", "feedback_bias_alpha"],
        ["log_R_ratio", "log_R_abs_ratio"],
        ["n", "step_ratio", "pattern"],
        seed=20260809,
    )
    for metric in ("R_ratio", "R_abs_ratio"):
        comparison_summary[f"median_{metric}_reduction_percent"] = (
            percent_reduction_from_log_ratio(comparison_summary[f"median_log_{metric}"])
        )
        comparison_summary[f"ci_low_{metric}_reduction_percent"] = (
            percent_reduction_from_log_ratio(comparison_summary[f"ci_high_log_{metric}"])
        )
        comparison_summary[f"ci_high_{metric}_reduction_percent"] = (
            percent_reduction_from_log_ratio(comparison_summary[f"ci_low_log_{metric}"])
        )
    wins = (
        comparisons.assign(
            R_pta_lower=comparisons.log_R_ratio < 0,
            R_abs_pta_lower=comparisons.log_R_abs_ratio < 0,
        )
        .groupby(
            ["comparator", "feedback_noise_alpha", "feedback_bias_alpha"],
            as_index=False,
        )[["R_pta_lower", "R_abs_pta_lower"]]
        .mean()
    )
    comparison_summary = comparison_summary.merge(
        wins,
        on=["comparator", "feedback_noise_alpha", "feedback_bias_alpha"],
        validate="one_to_one",
    )
    comparison_summary.to_csv(HERE / "pta_comparator_summary.csv", index=False)

    selected = comparison_summary.loc[
        comparison_summary.feedback_noise_alpha.isin([0.0, 0.2, 0.4])
        & comparison_summary.feedback_bias_alpha.isin([0.0, 0.1, 0.2])
    ]
    selected.to_csv(HERE / "selected_severity_summary.csv", index=False)

    plot_own_clean(own_summary)
    plot_pta_advantage(comparison_summary)

    print("Wrote imperfect-feedback summaries and figures.")
    print("\nStrongest-severity own-clean R degradation:")
    print(
        own_summary.loc[
            (own_summary.feedback_noise_alpha == 0.4)
            & (own_summary.feedback_bias_alpha == 0.2),
            ["family", "units", "median_R_degradation_percent", "ci_low_R_degradation_percent", "ci_high_R_degradation_percent"],
        ].to_string(index=False)
    )
    print("\nStrongest-severity PTA comparison:")
    print(
        comparison_summary.loc[
            (comparison_summary.feedback_noise_alpha == 0.4)
            & (comparison_summary.feedback_bias_alpha == 0.2),
            [
                "comparator",
                "units",
                "median_R_ratio_reduction_percent",
                "ci_low_R_ratio_reduction_percent",
                "ci_high_R_ratio_reduction_percent",
                "R_pta_lower",
                "median_R_abs_ratio_reduction_percent",
                "R_abs_pta_lower",
            ],
        ].to_string(index=False)
    )


if __name__ == "__main__":
    main()
