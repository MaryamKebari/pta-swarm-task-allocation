"""Authoritative experiment and method definitions.

The public runners read the selected parameter table instead of duplicating
gain values in Python.  This module translates each table row into the small
set of simulator overrides needed for that configuration.
"""

from __future__ import annotations

import csv
from collections.abc import Iterable
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PARAMETER_TABLE = ROOT / "data" / "parameters" / "reference_tuned_parameters.csv"

PATTERN_CODES = {
    "random": "legacy_vector_random",
    "sharp": "legacy_vector_sharp",
    "scurve": "legacy_vector_scurve",
    "zigzag": "legacy_vector_zigzag",
}
PATTERN_NAMES = {
    "random": "non-iterative gradual",
    "sharp": "non-iterative non-gradual",
    "scurve": "iterative gradual",
    "zigzag": "iterative non-gradual",
}
THRESHOLD_RANGE_CODES = {"HM": 1, "HT1": 2, "HT2": 3}
METHOD_CODES = {"LFTA": 0, "PTA": 1, "SETA": 2, "SBTA": 3, "CT": 4}

PAPER_POPULATIONS = (50, 100, 500, 1000)
PAPER_TASK_COUNTS = (4, 8, 12)
PAPER_STEP_RATIOS = (1.5, 2.0, 2.5)
PAPER_PATTERNS = ("random", "sharp", "scurve", "zigzag")
REFERENCE_SETTING = {
    "pop": 500,
    "n": 4,
    "step_ratio": 2.0,
    "pattern": "scurve",
}


def _number(row: dict[str, str], name: str, default: float = 0.0) -> float:
    value = row.get(name, "")
    if value is None or value.strip() == "":
        return default
    return float(value)


@dataclass(frozen=True)
class MethodConfiguration:
    """One of the 27 configurations evaluated in the manuscript."""

    family: str
    config_id: str
    method: str
    method_label: str
    threshold_mode: str
    threshold_range: str
    gain_scheme: str
    p_gain: float
    i_gain: float
    d_gain: float
    threshold_increase: float
    threshold_decrease: float
    p_spread: float
    i_spread: float
    d_spread: float

    @property
    def run_method_id(self) -> str:
        # Preserve the four-field identifier used by the archived campaign.
        # Families without a gain scheme therefore end with a trailing pipe.
        return f"{self.family}|{self.method}|{self.threshold_mode}|{self.gain_scheme}"

    @classmethod
    def from_row(cls, row: dict[str, str]) -> MethodConfiguration:
        return cls(
            family=row["family"],
            config_id=row["config_id"],
            method=row["method"],
            method_label=row["method_label"],
            threshold_mode=row["threshold_mode"],
            threshold_range=row["threshold_range"],
            gain_scheme=row.get("gain_scheme", "") or "",
            p_gain=_number(row, "P_gain"),
            i_gain=_number(row, "I_gain"),
            d_gain=_number(row, "D_gain"),
            threshold_increase=_number(row, "Thresh_increase"),
            threshold_decrease=_number(row, "Thresh_decrease"),
            p_spread=_number(row, "Agent_pid_p_spread"),
            i_spread=_number(row, "Agent_pid_i_spread"),
            d_spread=_number(row, "Agent_pid_d_spread"),
        )

    def simulator_overrides(self) -> dict[str, object]:
        """Return the simulator settings for this fixed configuration."""
        if self.family not in METHOD_CODES:
            raise ValueError(f"Unknown method family: {self.family}")
        if self.threshold_range not in THRESHOLD_RANGE_CODES:
            raise ValueError(f"Unknown threshold range: {self.threshold_range}")
        if self.threshold_mode not in {"clamped", "latent"}:
            raise ValueError(f"Unknown stored threshold mode: {self.threshold_mode}")

        return {
            "Thresh_dynamic": THRESHOLD_RANGE_CODES[self.threshold_range],
            "Pid": METHOD_CODES[self.family],
            "Pid_latent_thresholds": int(self.threshold_mode == "latent"),
            "Pid_integral_leak": 0.99,
            "Pid_integral_bound": 500,
            "Agent_pid_gains": int(self.gain_scheme == "Agent"),
            "Agent_pid_gain_apply_id": 1,
            "P_gain": self.p_gain,
            "I_gain": self.i_gain,
            "D_gain": self.d_gain,
            "Agent_pid_gain_spread": 0,
            "Agent_pid_p_spread": self.p_spread,
            "Agent_pid_i_spread": self.i_spread,
            "Agent_pid_d_spread": self.d_spread,
            "Thresh_increase": self.threshold_increase,
            "Thresh_decrease": self.threshold_decrease,
        }

    def metadata(self) -> dict[str, object]:
        return {
            "family": self.family,
            "method": self.method,
            "method_label": self.method_label,
            "threshold_mode": self.threshold_mode,
            "threshold_range": self.threshold_range,
            "gain_scheme": self.gain_scheme,
            "run_method_id": self.run_method_id,
            "td": THRESHOLD_RANGE_CODES[self.threshold_range],
            "pid": METHOD_CODES[self.family],
            "agent_pid_gains": int(self.gain_scheme == "Agent"),
            "pid_latent_thresholds": int(self.threshold_mode == "latent"),
            "P_gain": self.p_gain,
            "I_gain": self.i_gain,
            "D_gain": self.d_gain,
            "Agent_pid_p_spread": self.p_spread,
            "Agent_pid_i_spread": self.i_spread,
            "Agent_pid_d_spread": self.d_spread,
            "Thresh_increase": self.threshold_increase,
            "Thresh_decrease": self.threshold_decrease,
        }


def load_method_configurations(
    path: Path = PARAMETER_TABLE,
) -> list[MethodConfiguration]:
    """Load and validate the complete 27-configuration paper set."""
    with path.open(newline="", encoding="utf-8") as handle:
        configurations = [
            MethodConfiguration.from_row(row) for row in csv.DictReader(handle)
        ]

    expected = {"CT": 3, "LFTA": 3, "SBTA": 3, "SETA": 6, "PTA": 12}
    observed = {
        family: sum(config.family == family for config in configurations)
        for family in expected
    }
    if observed != expected:
        raise ValueError(f"Expected method counts {expected}; found {observed}")
    identifiers = [config.run_method_id for config in configurations]
    if len(identifiers) != len(set(identifiers)):
        raise ValueError(
            "The selected parameter table contains duplicate configurations"
        )
    return configurations


def filter_configurations(
    configurations: Iterable[MethodConfiguration],
    families: set[str] | None = None,
    identifiers: set[str] | None = None,
) -> list[MethodConfiguration]:
    """Filter configurations using optional family or run-identifier sets."""
    selected = list(configurations)
    if families:
        selected = [config for config in selected if config.family in families]
    if identifiers:
        selected = [
            config for config in selected if config.run_method_id in identifiers
        ]
    if not selected:
        raise ValueError("No method configurations match the requested filters")
    return selected
