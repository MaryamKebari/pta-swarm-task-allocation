"""Regression checks for paper critical simulator behavior."""

import hashlib
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class SimulatorContractTests(unittest.TestCase):
    def test_final_campaign_base_parameters_are_exact(self) -> None:
        digest = hashlib.sha256(
            (ROOT / "configs/params.default").read_bytes()
        ).hexdigest()
        self.assertEqual(
            digest,
            "01d394af9229c4494247f429e5714f6088de4b17d80aa3591a4bcad3088d1156",
        )

    def test_pta_derivative_uses_selector_stimulus(self) -> None:
        source = (ROOT / "simulator/src/ftracker.c").read_text()
        self.assertIn("fmax(Task_feedback_error[task], 0.0)", source)
        self.assertIn("fmax(Task_prev_feedback_error[task], 0.0)", source)

    def test_all_five_method_dispatches_are_present(self) -> None:
        source = (ROOT / "simulator/src/ftracker.c").read_text()
        for dispatch in ["Pid == 0", "Pid == 1", "Pid == 2", "Pid == 3"]:
            self.assertIn(dispatch, source)
        self.assertIn("4=CT", source)
        self.assertIn("Task_feedback_error[task]", source)

    def test_integral_is_leaky_and_bounded(self) -> None:
        source = (ROOT / "simulator/src/fxn.c").read_text()
        self.assertIn("Task_error_integral[task] *= Pid_integral_leak", source)
        self.assertIn("Task_error_integral[task] = Pid_integral_bound", source)
        self.assertIn("Task_error_integral[task] = -Pid_integral_bound", source)

    def test_agent_gains_use_an_independent_reproducible_stream(self) -> None:
        source = (ROOT / "simulator/src/fxn.c").read_text()
        self.assertIn("agent_pid_gain_uniform", source)
        self.assertIn("Keep gain sampling independent", source)
        self.assertIn("Agent[n].agent_P_gain = P_gain * p_mult", source)

    def test_latent_mode_retains_unclamped_stored_values(self) -> None:
        source = (ROOT / "simulator/src/ftracker.c").read_text()
        guard = "(Pid == 1 || Pid == 2) && Pid_latent_thresholds == 1"
        self.assertGreaterEqual(source.count(guard), 2)

    def test_selected_agents_add_equal_uncapped_service(self) -> None:
        source = (ROOT / "simulator/src/fxn.c").read_text()
        self.assertIn("Tracker.max_step_len / (double)service_denominator", source)
        self.assertIn("Task_actor_count[task] * service_per_agent", source)
        self.assertIn("Task_tracker_vector[task] += Task_service[task]", source)

    def test_first_nonidle_assignment_is_not_a_switch(self) -> None:
        source = (ROOT / "simulator/src/ftracker.c").read_text()
        self.assertIn("Agent[i].previous_task_noidle > 0", source)

    def test_required_metrics_are_emitted(self) -> None:
        source = (ROOT / "simulator/src/output.c").read_text()
        for metric in [
            "R,R_abs,R2,R2_norm",
            "post_removal_R",
            "post_removal_R_abs",
            "post_removal_steps",
        ]:
            self.assertIn(metric, source)

    def test_random_path_seed_is_initialized_after_parameters(self) -> None:
        source = (ROOT / "simulator/src/sim.c").read_text()
        self.assertLess(
            source.index("read_params(params_file)"),
            source.index("init_target_path_rng()"),
        )

    def test_feedback_is_not_clipped_by_default(self) -> None:
        runner = (ROOT / "experiments/run.py").read_text()
        smoke = (ROOT / "configs/smoke.params").read_text()
        self.assertIn('"Feedback_noise_clip": 0', runner)
        self.assertIn("Feedback_noise_clip 0", smoke)


if __name__ == "__main__":
    unittest.main()
