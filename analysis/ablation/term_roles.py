#!/usr/bin/env python3
"""Create the two-path, two-term controller-ablation figure.

The figure crosses derivative and integral additions with iterative
non-gradual demand and a sustained plateau. Every panel reports the same
quantity and direction: percentage error reduction after adding the named
term, with positive values favoring the added term.
"""

from __future__ import annotations

import os
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from scipy import stats

ROOT = Path(__file__).resolve().parents[2]
AUDIT_DIR = ROOT / "data" / "processed" / "ablation"
FIGURE_DIR = ROOT / "figures"

FULL_GRID_RUNS = Path(
    os.environ.get(
        "PTA_ABLATION_CSV",
        ROOT / "data" / "raw" / "ablation" / "per_run_results.csv",
    )
)
PLATEAU_RUNS = Path(
    os.environ.get(
        "PTA_PLATEAU_ABLATION_CSV",
        ROOT / "data" / "raw" / "ablation" / "plateau_metrics.csv",
    )
)

CACHED_DIRECT = AUDIT_DIR / "ablation_direct_term_effects.csv"
CACHED_PERSISTENT = AUDIT_DIR / "ablation_persistent_term_summary.csv"
OUT_SUMMARY = AUDIT_DIR / "ablation_selected_path_summary.csv"
OUT_PDF = FIGURE_DIR / "controller_ablation_term_roles.pdf"
OUT_PNG = FIGURE_DIR / "controller_ablation_term_roles.png"

BOOTSTRAP_REPLICATES = 20_000
CONDITION_KEYS = ["pop", "n", "step_ratio", "pattern", "threshold_range"]


def percentile_interval(
    values: np.ndarray, statistic, seed: int
) -> tuple[float, float]:
    """Return a percentile bootstrap interval over the supplied units."""
    values = np.asarray(values, dtype=float)
    rng = np.random.default_rng(seed)
    indices = rng.integers(0, values.size, size=(BOOTSTRAP_REPLICATES, values.size))
    sampled = statistic(values[indices], axis=1)
    return tuple(np.quantile(sampled, [0.025, 0.975]).astype(float))


def bh_adjust(p_values: np.ndarray) -> np.ndarray:
    """Apply Benjamini-Hochberg correction to one declared test family."""
    values = np.asarray(p_values, dtype=float)
    order = np.argsort(values)
    ranked = values[order] * len(values) / np.arange(1, len(values) + 1)
    ranked = np.minimum.accumulate(ranked[::-1])[::-1]
    adjusted = np.empty_like(ranked)
    adjusted[order] = np.minimum(ranked, 1.0)
    return adjusted


def iterative_non_gradual_effects() -> pd.DataFrame:
    """Return D- and I-addition effects for eight iterative conditions."""
    if not FULL_GRID_RUNS.exists():
        cached = pd.read_csv(CACHED_DIRECT)
        return cached.loc[cached["pattern"].eq("zigzag")].copy()

    runs = pd.read_csv(FULL_GRID_RUNS)
    # The archived final campaign names the complete controller ``full_PTA``.
    # Older exploratory files used ``PID``; normalize both at the input edge.
    runs["ablation_variant"] = runs["ablation_variant"].replace({"full_PTA": "PID"})
    runs["threshold_range"] = runs["method_label"].str.extract(r"PTA-(HT1|HT2)")[0]
    runs["r2_norm"] = runs["avg_post"] / np.sqrt(runs["n"])
    runs = runs.loc[runs["pattern"].eq("zigzag")]
    wide = runs.pivot(
        index=CONDITION_KEYS + ["rep"],
        columns="ablation_variant",
        values="r2_norm",
    ).reset_index()
    if wide[["P", "PI", "PD", "PID"]].isna().any().any():
        raise AssertionError("Zigzag ablation pairing is incomplete")

    records: list[dict[str, object]] = []
    for added_term, comparison, baseline, with_term in [
        ("D", "P $\\rightarrow$ PD", "P", "PD"),
        ("D", "PI $\\rightarrow$ full PTA", "PI", "PID"),
        ("I", "P $\\rightarrow$ PI", "P", "PI"),
        ("I", "PD $\\rightarrow$ full PTA", "PD", "PID"),
    ]:
        for keys, group in wide.groupby(CONDITION_KEYS, sort=True):
            before = group[baseline].to_numpy()
            after = group[with_term].to_numpy()
            reduction = before - after
            before_mean = float(before.mean())
            p_value = (
                1.0
                if np.allclose(reduction, 0.0)
                else float(stats.wilcoxon(reduction, alternative="two-sided").pvalue)
            )
            records.append(
                {
                    **dict(zip(CONDITION_KEYS, keys)),
                    "added_term": added_term,
                    "comparison": comparison,
                    "baseline": baseline,
                    "with_term": "full PTA" if with_term == "PID" else with_term,
                    "baseline_mean_r2_norm": before_mean,
                    "with_term_mean_r2_norm": float(after.mean()),
                    "condition_mean_reduction_norm": float(reduction.mean()),
                    "relative_reduction_percent": 100.0
                    * float(reduction.mean())
                    / before_mean,
                    "term_reduced_error": bool(reduction.mean() > 0),
                    "p_value": p_value,
                }
            )
    return pd.DataFrame(records)


def plateau_effects() -> tuple[pd.DataFrame, pd.DataFrame | None]:
    """Return D- and I-addition summaries for the sustained plateau."""
    comparisons = [
        ("D", "P $\\rightarrow$ PD", "P", "PD"),
        ("D", "PI $\\rightarrow$ full PTA", "PI", "PID"),
        ("I", "P $\\rightarrow$ PI", "P", "PI"),
        ("I", "PD $\\rightarrow$ full PTA", "PD", "PID"),
    ]
    if PLATEAU_RUNS.exists():
        runs = pd.read_csv(PLATEAU_RUNS)
        selected = runs.loc[
            runs["path"].eq("long_plateau"),
            ["replicate", "condition", "late_abs_y_error"],
        ]
        paired = selected.pivot(
            index="replicate", columns="condition", values="late_abs_y_error"
        ).dropna()
        records: list[dict[str, object]] = []
        unit_records: list[pd.DataFrame] = []
        for index, (added_term, comparison, baseline, with_term) in enumerate(
            comparisons
        ):
            before = paired[baseline].to_numpy()
            after = paired[with_term].to_numpy()
            reduction = before - after
            before_mean = float(before.mean())
            low, high = percentile_interval(reduction, np.mean, 20260820 + index)
            p_value = (
                1.0
                if np.allclose(reduction, 0.0)
                else float(stats.wilcoxon(reduction, alternative="two-sided").pvalue)
            )
            records.append(
                {
                    "path": "long_plateau",
                    "added_term": added_term,
                    "comparison": comparison,
                    "paired_repetitions": len(reduction),
                    "baseline_mean_late_abs_error": before_mean,
                    "with_term_mean_late_abs_error": float(after.mean()),
                    "relative_reduction_percent": 100.0
                    * float(reduction.mean())
                    / before_mean,
                    "relative_ci95_low": 100.0 * low / before_mean,
                    "relative_ci95_high": 100.0 * high / before_mean,
                    "with_term_lower_repetitions": int((reduction > 0).sum()),
                    "baseline_lower_repetitions": int((reduction < 0).sum()),
                    "ties": int((reduction == 0).sum()),
                    "p_value": p_value,
                }
            )
            unit_records.append(
                pd.DataFrame(
                    {
                        "comparison": comparison,
                        "unit_effect_percent": 100.0 * reduction / before_mean,
                    }
                )
            )
        return pd.DataFrame(records), pd.concat(unit_records, ignore_index=True)

    cached = pd.read_csv(CACHED_PERSISTENT)
    cached = cached.loc[cached["path"].eq("long_plateau")].copy()
    cached["relative_ci95_low"] = (
        100.0
        * cached["paired_bootstrap_ci95_low"]
        / cached["baseline_mean_late_abs_error"]
    )
    cached["relative_ci95_high"] = (
        100.0
        * cached["paired_bootstrap_ci95_high"]
        / cached["baseline_mean_late_abs_error"]
    )
    cached["p_value"] = cached["wilcoxon_two_sided_p"]
    return cached, None


def selected_summary() -> pd.DataFrame:
    """Build eight effects arranged in four path-by-term panels."""
    iterative = iterative_non_gradual_effects()
    plateau, _ = plateau_effects()
    records: list[dict[str, object]] = []

    panel_specs = [
        (
            "a",
            "Iterative non-gradual",
            "D",
            ["P $\\rightarrow$ PD", "PI $\\rightarrow$ full PTA"],
        ),
        (
            "b",
            "Iterative non-gradual",
            "I",
            ["P $\\rightarrow$ PI", "PD $\\rightarrow$ full PTA"],
        ),
        (
            "c",
            "Sustained plateau",
            "D",
            ["P $\\rightarrow$ PD", "PI $\\rightarrow$ full PTA"],
        ),
        (
            "d",
            "Sustained plateau",
            "I",
            ["P $\\rightarrow$ PI", "PD $\\rightarrow$ full PTA"],
        ),
    ]

    # Correct all 32 condition-level tests and four plateau tests together.
    all_p = np.concatenate(
        [
            iterative["p_value"].to_numpy(),
            plateau["p_value"].to_numpy(),
        ]
    )
    adjusted = bh_adjust(all_p)
    iterative = iterative.copy()
    plateau = plateau.copy()
    iterative["bh_q"] = adjusted[: len(iterative)]
    plateau["bh_q"] = adjusted[len(iterative) :]

    seed = 20260830
    for panel, path, added_term, comparisons in panel_specs:
        for comparison in comparisons:
            if path == "Iterative non-gradual":
                group = iterative.loc[
                    iterative["added_term"].eq(added_term)
                    & iterative["comparison"].eq(comparison)
                ]
                effects = group["relative_reduction_percent"].to_numpy()
                low, high = percentile_interval(effects, np.median, seed)
                seed += 1
                records.append(
                    {
                        "panel": panel,
                        "path": path,
                        "added_term": added_term,
                        "comparison": comparison,
                        "effect_percent": float(np.median(effects)),
                        "ci95_low": low,
                        "ci95_high": high,
                        "units": len(group),
                        "units_with_lower_error": int(
                            group["term_reduced_error"].sum()
                        ),
                        "significant_units": int((group["bh_q"] < 0.05).sum()),
                        "unit_label": "conditions",
                        "max_bh_q": float(group["bh_q"].max()),
                    }
                )
            else:
                row = plateau.loc[
                    plateau["added_term"].eq(added_term)
                    & plateau["comparison"].eq(comparison)
                ].iloc[0]
                records.append(
                    {
                        "panel": panel,
                        "path": path,
                        "added_term": added_term,
                        "comparison": comparison,
                        "effect_percent": float(row["relative_reduction_percent"]),
                        "ci95_low": float(row["relative_ci95_low"]),
                        "ci95_high": float(row["relative_ci95_high"]),
                        "units": int(row["paired_repetitions"]),
                        "units_with_lower_error": int(
                            row["with_term_lower_repetitions"]
                        ),
                        "significant_units": int(row["bh_q"] < 0.05),
                        "unit_label": "repetitions",
                        "max_bh_q": float(row["bh_q"]),
                    }
                )
    return pd.DataFrame(records)


def draw_panel(ax: plt.Axes, group: pd.DataFrame, color: str) -> None:
    """Draw two direct comparisons in one path-by-term panel."""
    lower, upper = -10.0, 100.0
    ax.set_ylim(lower, upper)
    ax.axhspan(0, upper, color="#EAF4EC", alpha=0.62, zorder=0)
    ax.axhspan(lower, 0, color="#F8ECEB", alpha=0.52, zorder=0)
    ax.axhline(0, color="#333333", linewidth=0.9, zorder=1)

    x = np.arange(len(group))
    for position, (_, row) in zip(x, group.iterrows()):
        effect = float(row["effect_percent"])
        ci_low = float(row["ci95_low"])
        ci_high = float(row["ci95_high"])
        ax.errorbar(
            position,
            effect,
            yerr=[[effect - ci_low], [ci_high - effect]],
            fmt="D",
            markersize=7,
            markerfacecolor=color,
            markeredgecolor="#111111",
            color="#111111",
            capsize=4,
            linewidth=1.4,
            zorder=3,
        )
        if abs(effect) < 0.05:
            effect_text = "no change"
        elif effect > 0:
            effect_text = f"{effect:.1f}% lower"
        else:
            effect_text = f"{abs(effect):.1f}% higher"
        effect_y = min(max(ci_high + 5.0, 20.0), 94.0)
        ax.text(
            position,
            effect_y,
            effect_text,
            ha="center",
            va="bottom",
            fontsize=11.0,
            fontweight="semibold",
        )
        if row["unit_label"] == "conditions":
            if int(row["significant_units"]) == int(row["units"]):
                significance_text = (
                    f"lower in {int(row['units_with_lower_error'])}/{int(row['units'])}\n"
                    f"significant in {int(row['significant_units'])}/{int(row['units'])}"
                )
            else:
                significance_text = (
                    "CI includes 0\n"
                    f"{int(row['significant_units'])}/{int(row['units'])} significant"
                )
        elif float(row["max_bh_q"]) < 0.001:
            significance_text = (
                f"lower in {int(row['units_with_lower_error'])}/{int(row['units'])}\n"
                "$q<0.001$"
            )
        else:
            significance_text = (
                f"lower in {int(row['units_with_lower_error'])}/{int(row['units'])}\n"
                "not significant"
            )
        ax.text(
            position,
            5.0,
            significance_text,
            ha="center",
            va="bottom",
            fontsize=9.5,
            color="#3A4C3D",
        )
    ax.set_xticks(x, group["comparison"].astype(str).tolist())
    ax.set_xlim(-0.55, len(group) - 0.45)
    row = group.iloc[0]
    path_title = (
        "Iterative non-gradual demand"
        if row["path"] == "Iterative non-gradual"
        else "Sustained plateau"
    )
    ax.set_title(
        f"({row['panel']}) {path_title}\nAdd {row['added_term']}",
        loc="left",
        fontweight="semibold",
        fontsize=9.5,
    )
    ax.grid(axis="y", color="#D8D8D8", linewidth=0.55, zorder=0)
    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)


def main() -> None:
    summary = selected_summary()
    summary.to_csv(OUT_SUMMARY, index=False)

    plt.rcParams.update(
        {
            "font.family": "serif",
            "font.size": 11.5,
            "axes.labelsize": 11.5,
            "axes.titlesize": 12.0,
            "xtick.labelsize": 10.5,
            "ytick.labelsize": 10.5,
            "pdf.fonttype": 42,
            "ps.fonttype": 42,
        }
    )
    colors = {"D": "#D55E00", "I": "#009E73"}
    fig, axes = plt.subplots(
        2, 2, figsize=(7.25, 5.15), sharey=True, constrained_layout=True
    )
    for ax, (_, group) in zip(axes.flat, summary.groupby("panel", sort=True)):
        draw_panel(ax, group, colors[str(group.iloc[0]["added_term"])])
    axes[0, 0].set_ylabel(r"$R_{2,\mathrm{norm}}$ reduction (%)")
    axes[1, 0].set_ylabel("Late residual reduction (%)")
    fig.text(
        0.5,
        -0.018,
        "Positive values favor the added term; error bars are 95% bootstrap intervals",
        ha="center",
        fontsize=10.0,
        color="#444444",
    )

    FIGURE_DIR.mkdir(parents=True, exist_ok=True)
    fig.savefig(OUT_PDF, bbox_inches="tight")
    fig.savefig(OUT_PNG, dpi=300, bbox_inches="tight")
    plt.close(fig)

    print(summary.to_string(index=False))
    print(f"wrote {OUT_SUMMARY}")
    print(f"wrote {OUT_PDF}")
    print(f"wrote {OUT_PNG}")


if __name__ == "__main__":
    main()
