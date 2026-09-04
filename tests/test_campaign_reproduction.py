"""End to end regression against archived paper campaign values."""

from __future__ import annotations

import csv
import math
import unittest
from pathlib import Path

from experiments.definitions import load_method_configurations
from experiments.run import Job, simulator_overrides
from experiments.run_sim import SIMULATOR, run_simulation

ROOT = Path(__file__).resolve().parents[1]
GOLDEN = ROOT / "tests" / "golden_smoke_results.csv"
METRICS = ["R", "R_abs", "R2", "R2_norm", "R2_max_norm", "avg_switch_noidle"]


class CampaignReproductionTests(unittest.TestCase):
    @unittest.skipUnless(SIMULATOR.is_file(), "simulator has not been built")
    def test_archived_cross_family_values(self) -> None:
        with GOLDEN.open(newline="", encoding="utf-8") as handle:
            expected = {row["family"]: row for row in csv.DictReader(handle)}

        configurations = load_method_configurations()
        selected = {
            family: next(
                config
                for config in configurations
                if config.family == family
                and config.threshold_range == "HT1"
                and config.threshold_mode == "clamped"
                and (family != "PTA" or config.gain_scheme == "Agent")
            )
            for family in expected
        }
        seed_row = {
            "pop": "50",
            "n": "4",
            "step_ratio": "1.5",
            "pattern": "random",
            "pattern_full": "legacy_vector_random",
            "rep": "0",
            "seed": "7386709",
            "target_path_seed": "7399054",
        }
        for family, config in selected.items():
            with self.subTest(family=family):
                self.assertEqual(
                    config.run_method_id, expected[family]["run_method_id"]
                )
                stats = run_simulation(
                    simulator_overrides(Job("allocation", config, seed_row, {})),
                    seed=int(seed_row["seed"]),
                )
                for metric in METRICS:
                    self.assertTrue(
                        math.isclose(
                            float(stats[metric]),
                            float(expected[family][metric]),
                            rel_tol=0.025,
                            abs_tol=0.005,
                        ),
                        msg=(
                            f"{family} {metric}: observed {stats[metric]}, "
                            f"archived {expected[family][metric]}"
                        ),
                    )


if __name__ == "__main__":
    unittest.main()
