"""Lightweight integrity tests for committed processed results."""

from pathlib import Path
import unittest

import pandas as pd


ROOT = Path(__file__).resolve().parents[1]


class ProcessedDataTests(unittest.TestCase):
    def test_clean_transfer_summary_has_all_comparators(self) -> None:
        table = pd.read_csv(ROOT / "data/processed/clean_transfer/range_matched_summary.csv")
        self.assertEqual(set(table["comparator"]), {"CT", "LFTA", "SBTA", "SETA"})
        self.assertTrue((table["operating_condition_ranges"] == 432).all())
        self.assertTrue(table.select_dtypes("number").notna().all().all())

    def test_feedback_grid_has_expected_severities(self) -> None:
        table = pd.read_csv(ROOT / "data/processed/imperfect_feedback/pta_comparator_summary.csv")
        self.assertEqual(set(table["feedback_noise_alpha"]), {0.0, 0.05, 0.1, 0.2, 0.4})
        self.assertEqual(set(table["feedback_bias_alpha"]), {0.0, 0.05, 0.1, 0.2})
        self.assertEqual(set(table["comparator"]), {"CT", "LFTA", "SBTA", "SETA"})

    def test_population_summary_is_complete(self) -> None:
        table = pd.read_csv(ROOT / "data/processed/population/population_mechanism_summary.csv")
        self.assertEqual(set(table["population"]), {50, 100, 500, 1000})
        self.assertTrue((table["rms_between_run_sd_recruited_fraction"] > 0).all())


if __name__ == "__main__":
    unittest.main()
