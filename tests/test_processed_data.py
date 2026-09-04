"""Lightweight integrity tests for committed processed results."""

import unittest
from pathlib import Path

import pandas as pd

from experiments.definitions import load_method_configurations

ROOT = Path(__file__).resolve().parents[1]


class ProcessedDataTests(unittest.TestCase):
    def test_complete_method_configuration_set(self) -> None:
        configurations = load_method_configurations()
        counts = pd.Series([config.family for config in configurations]).value_counts()
        self.assertEqual(
            counts.to_dict(),
            {"PTA": 12, "SETA": 6, "CT": 3, "LFTA": 3, "SBTA": 3},
        )

    def test_seed_maps_cover_the_paper_grids(self) -> None:
        allocation = pd.read_csv(ROOT / "data/manifests/allocation_seed_map.csv")
        self.assertEqual(len(allocation), 14_400)
        condition_columns = ["pop", "n", "step_ratio", "pattern"]
        self.assertEqual(allocation[condition_columns].drop_duplicates().shape[0], 144)
        self.assertTrue((allocation.groupby(condition_columns).size() == 100).all())

        feedback = pd.read_csv(ROOT / "data/manifests/imperfect_feedback_seed_map.csv")
        feedback_conditions = ["n", "step_ratio", "pattern"]
        self.assertEqual(len(feedback), 2_400)
        self.assertEqual(feedback[feedback_conditions].drop_duplicates().shape[0], 24)
        self.assertTrue((feedback.groupby(feedback_conditions).size() == 100).all())
        self.assertEqual(
            len(pd.read_csv(ROOT / "data/manifests/tuning_seed_map.csv")), 20
        )

    def test_clean_transfer_summary_has_all_comparators(self) -> None:
        table = pd.read_csv(
            ROOT / "data/processed/clean_transfer/range_matched_summary.csv"
        )
        self.assertEqual(set(table["comparator"]), {"CT", "LFTA", "SBTA", "SETA"})
        self.assertTrue((table["operating_condition_ranges"] == 432).all())
        self.assertTrue(table.select_dtypes("number").notna().all().all())

    def test_feedback_grid_has_expected_severities(self) -> None:
        table = pd.read_csv(
            ROOT / "data/processed/imperfect_feedback/pta_comparator_summary.csv"
        )
        self.assertEqual(set(table["feedback_noise_alpha"]), {0.0, 0.05, 0.1, 0.2, 0.4})
        self.assertEqual(set(table["feedback_bias_alpha"]), {0.0, 0.05, 0.1, 0.2})
        self.assertEqual(set(table["comparator"]), {"CT", "LFTA", "SBTA", "SETA"})

    def test_population_summary_is_complete(self) -> None:
        table = pd.read_csv(
            ROOT / "data/processed/population/population_mechanism_summary.csv"
        )
        self.assertEqual(set(table["population"]), {50, 100, 500, 1000})
        self.assertTrue((table["rms_between_run_sd_recruited_fraction"] > 0).all())


if __name__ == "__main__":
    unittest.main()
