# Figure guide

`make figures` regenerates publication files in `figures/`. The images in
`docs/assets/` are README previews of a subset of those figures.

A positive percentage reduction means PTA has lower error than the comparator.
Dashed zero or equality lines are neutral references, not fitted trends.

## Regenerated paper figures

| File in `figures/` | Claim | Script | Source tables |
|---|---|---|---|
| `demand_class_pta_advantage` | PTA's advantage varies with demand structure | `analysis/figures.py` | `data/processed/allocation/` |
| `step_ratio_pta_error` | Absolute error changes with capacity | `analysis/figures.py` | `data/processed/allocation/` |
| `step_ratio_pta_advantage` | PTA's relative advantage is smallest at the highest tested capacity | `analysis/figures.py` | `data/processed/allocation/` |
| `population_pta_advantage` | The advantage persists across population sizes | `analysis/figures.py` | `data/processed/allocation/population_task_scaling_summary.csv` |
| `task_count_pta_advantage` | The advantage persists across task counts | `analysis/figures.py` | `data/processed/allocation/population_task_scaling_summary.csv` |
| `threshold_range_preference` | Preferred threshold range depends on demand class and step ratio | `analysis/figures.py` | `data/processed/allocation/winner_by_condition.csv` |
| `frozen_reference_winner_atlas` | The preferred PTA configuration changes across the grid | `analysis/allocation/winner_atlas.py` | `data/processed/allocation/` |
| `integral_term_role` | Integral action matters for persistent error | `analysis/figures.py` | `data/processed/ablation/ablation_selected_path_summary.csv` |
| `derivative_term_role` | Derivative action matters for rapid reversals | `analysis/figures.py` | `data/processed/ablation/ablation_selected_path_summary.csv` |
| `population_mechanism_diagnostic` | Recruitment variability changes with population size | `analysis/population/plot.py` | `data/processed/population/` |
| `agent_removal_deterioration` | Error rises as agents and capacity are removed | `analysis/figures.py` | `data/processed/removal/pta_degradation_by_step_ratio.csv` |
| `agent_removal_paired_advantage` | PTA retains a paired-imbalance advantage after removal | `analysis/figures.py` | `data/processed/removal/pta_comparator_summary.csv` |
| `agent_removal_raw_advantage` | PTA retains a raw-residual advantage after removal | `analysis/figures.py` | `data/processed/removal/pta_comparator_summary.csv` |
| `imperfect_feedback_pta_comparators` | PTA retains a relative advantage under noise and bias | `analysis/feedback/compare.py` | `data/processed/feedback/` |
| `imperfect_feedback_own_clean` | Degradation relative to each method's own clean-feedback result | `analysis/feedback/compare.py` | `data/processed/feedback/` |

Each stem is written as both `.pdf` and `.png`.

## How to regenerate

```bash
make figures
```

That command uses only `data/processed`. It does not re-run the C simulator.

To rebuild a statistical summary from raw campaign output, place the raw CSV
files as described in [DATA.md](DATA.md) and run the corresponding script in
`analysis/`.

The numbered conclusion list in [PAPER_TRACEABILITY.md](PAPER_TRACEABILITY.md)
is the manuscript-facing map.
