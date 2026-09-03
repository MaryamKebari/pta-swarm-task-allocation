"""Regression checks for paper critical simulator behavior."""

from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]


class SimulatorContractTests(unittest.TestCase):
    def test_pta_derivative_uses_selector_stimulus(self) -> None:
        source = (ROOT / "simulator/src/ftracker.c").read_text()
        self.assertIn("fmax(Task_feedback_error[task], 0.0)", source)
        self.assertIn("fmax(Task_prev_feedback_error[task], 0.0)", source)

    def test_required_metrics_are_emitted(self) -> None:
        source = (ROOT / "simulator/src/output.c").read_text()
        for metric in ["R,R_abs,R2,R2_norm", "post_removal_R", "post_removal_R_abs", "post_removal_steps"]:
            self.assertIn(metric, source)

    def test_random_path_seed_is_initialized_after_parameters(self) -> None:
        source = (ROOT / "simulator/src/sim.c").read_text()
        self.assertLess(source.index("read_params(params_file)"), source.index("init_target_path_rng()"))

    def test_feedback_is_not_clipped_by_default(self) -> None:
        defaults = (ROOT / "configs/params.default").read_text()
        globals_source = (ROOT / "simulator/src/global.h").read_text()
        self.assertNotIn("Feedback_noise_clip 1", defaults)
        self.assertIn("int Feedback_noise_clip;", globals_source)


if __name__ == "__main__":
    unittest.main()
