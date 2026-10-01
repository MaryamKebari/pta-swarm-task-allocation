"""Regression checks for report-based (per-agent) feedback.

Report mode replaces the shared feedback channel with per-agent estimates built
from private observations and peer reports. With honest peers and no
observation noise every estimate equals the shared feedback, so the simulator
must reproduce shared-feedback runs exactly.
"""

from __future__ import annotations

import unittest

from experiments.definitions import load_method_configurations
from experiments.run import Job, simulator_overrides
from experiments.run_sim import SIMULATOR, run_simulation

SEED = 7386709
SEED_ROW = {
    "pop": "50",
    "n": "4",
    "step_ratio": "1.5",
    "pattern": "random",
    "pattern_full": "legacy_vector_random",
    "rep": "0",
    "seed": str(SEED),
    "target_path_seed": "7399054",
}
REMOVAL = {"Kill_number": 10, "First_extinction": 300, "Extinction_period": 0}


def overrides_for(config, extra=None):
    values = simulator_overrides(Job("allocation", config, SEED_ROW, {}))
    values.update(extra or {})
    return values


@unittest.skipUnless(SIMULATOR.is_file(), "simulator has not been built")
class ReportFeedbackTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        cls.configurations = load_method_configurations()
        cls.pta = next(
            config
            for config in cls.configurations
            if config.family == "PTA" and config.gain_scheme == "Agent"
        )

    def test_noise_free_reports_reduce_to_shared_feedback(self) -> None:
        for config in self.configurations:
            for label, extra in [("clean", {}), ("removal", REMOVAL)]:
                with self.subTest(config=config.run_method_id, case=label):
                    shared = run_simulation(overrides_for(config, extra), seed=SEED)
                    for report_k in (0, 3):
                        report = run_simulation(
                            overrides_for(
                                config,
                                {
                                    **extra,
                                    "Report_feedback": 1,
                                    "Report_k": report_k,
                                    "Obs_noise_alpha": 0.0,
                                },
                            ),
                            seed=SEED,
                        )
                        self.assertEqual(shared, report)

    def test_observation_noise_is_seeded_and_changes_feedback(self) -> None:
        noisy = {
            "Report_feedback": 1,
            "Report_k": 3,
            "Obs_noise_alpha": 0.5,
            "Report_seed": 5,
        }
        shared = run_simulation(overrides_for(self.pta), seed=SEED)
        first = run_simulation(overrides_for(self.pta, noisy), seed=SEED)
        repeat = run_simulation(overrides_for(self.pta, noisy), seed=SEED)
        reseeded = run_simulation(
            overrides_for(self.pta, {**noisy, "Report_seed": 6}), seed=SEED
        )
        self.assertEqual(first, repeat)
        self.assertNotEqual(first["R"], shared["R"])
        self.assertNotEqual(first["R"], reseeded["R"])


if __name__ == "__main__":
    unittest.main()
