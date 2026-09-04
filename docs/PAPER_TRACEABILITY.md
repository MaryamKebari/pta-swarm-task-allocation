# Paper to code traceability

This is the shortest path from the manuscript to the public code. Use it if
you want to check a table, figure, or numbered conclusion without browsing the
whole repository.

The numbered conclusions match the Results section of
“Feedback Regulation of Threshold Adaptation for Dynamic Swarm Task Allocation.”
File paths are relative to the repository root.

## Experimental methods

| Paper element | Implementation or specification | Verification |
|---|---|---|
| Demand classes | `simulator/src/ftarget.c`; `experiments/definitions.py` | `data/manifests/random_path_runtime_audit.json` |
| CT, LFTA, SBTA, SETA, and PTA | `simulator/src/ftracker.c`; `configs/methods.json` | `tests/test_simulator_contract.py` |
| HM, HT1, and HT2 threshold ranges | `simulator/src/ftracker.c`; `configs/methods.json` | 27 configuration check in `experiments/definitions.py` |
| Clamped and latent stored threshold modes | `simulator/src/ftracker.c`; `configs/methods.json` | `data/manifests/preflight_source_audit.json` |
| Global and Agent gain schemes | `simulator/src/ftracker.c`; `experiments/definitions.py` | selected values in `data/parameters/reference_tuned_parameters.csv` |
| Reference tuning | `experiments/tune_reference.py` | exact seeds in `data/manifests/tuning_seed_map.csv` |
| Clean feedback allocation grid | `experiments/run_campaign.py clean` | exact seeds in `data/manifests/allocation_seed_map.csv` |
| Repeated reversal ablation | `experiments/run_campaign.py ablation` | `analysis/ablation/make_controller_ablation_term_roles.py` |
| Sustained plateau diagnostic | archived raw schema in `docs/DATA.md` | processed paired results in `data/processed/ablation` |
| Population diagnostic | `analysis/population/run_population_mechanism_diagnostic.py` | replay files in `data/processed/population` |
| Agent removal | `experiments/run_campaign.py removal` | `analysis/agent_removal` |
| Imperfect feedback | `experiments/run_campaign.py feedback` | exact perturbation seeds in `data/manifests/imperfect_feedback_seed_map.csv` |
| Metrics | `simulator/src/output.c` | golden values in `data/manifests/golden_smoke_results.csv` |

The sustained plateau is a focused four task directional diagnostic retained
from the component study. Its complete per repetition output is distributed
with the archival raw dataset rather than this Git repository. The public
analysis accepts it through `PTA_PLATEAU_ABLATION_CSV`; the committed processed
tables preserve every value used in the paper.

## Tables

| Manuscript table | Public source |
|---|---|
| Demand classes | `configs/experiment_design.json`; `experiments/definitions.py` |
| Threshold ranges | `configs/methods.json` |
| Evaluation grids | `configs/experiment_design.json`; `experiments/run_campaign.py` |
| Reference tuning search spaces | `experiments/tune_reference.py`; `docs/EXPERIMENTS.md` |
| Selected PTA transfer, paired imbalance | `data/processed/clean_transfer/family_comparison_summary.csv`; `range_matched_confirmatory_summary.csv` |
| Selected PTA transfer, raw residual | `data/processed/clean_transfer/family_comparison_summary.csv` |
| All PTA configurations, paired imbalance and raw residual | `data/processed/clean_transfer/all_variant_summary.csv` |
| Three broadly useful PTA configurations | `data/processed/clean_transfer/winner_counts.csv`; `all_variant_summary.csv` |
| Stored threshold modes | `data/processed/clean_transfer/pta_storage_transfer_summary.csv`; `seta_storage_transfer_summary.csv`; `storage_mode_wilcoxon_summary.csv` |
| Agent and Global gain schemes | `data/processed/clean_transfer/gain_transfer_summary.csv`; `gain_scheme_wilcoxon_summary.csv` |
| Selected controller after removal | `data/processed/agent_removal/selected_configuration_comparison.csv` |
| Selected controller under imperfect feedback | `data/processed/imperfect_feedback/selected_configuration_strongest.csv` |
| Selected controller degradation | `data/processed/imperfect_feedback/selected_configuration_own_degradation.csv` |
| Strongest feedback raw residual | `data/processed/imperfect_feedback/selected_severity_summary.csv` |

## Figures and explicit conclusions

| Conclusions | Figure or table | Regeneration source |
|---|---|---|
| 1 and 2 | selected transfer tables | `analysis/clean_transfer/analyze_clean_transfer.py` |
| 3 and 4 | all configuration tables | `analysis/clean_transfer/comprehensive_fixed_configuration_audit.py` |
| 5 | three configuration coverage table | `data/processed/clean_transfer/all_variant_summary.csv` |
| 6 | demand class advantage | `analysis/generate_paper_figures.py` |
| 7 and 8 | step ratio error and advantage | `analysis/generate_paper_figures.py` |
| 9 and 10 | population and task count advantage | `analysis/clean_transfer/make_population_task_scaling.py`; `analysis/generate_paper_figures.py` |
| 11 | population recruitment variability | `analysis/population/plot_population_mechanism_diagnostic.py` |
| 12 | preferred PTA configuration atlas | `analysis/clean_transfer/make_frozen_winner_atlas.py` |
| 13 | stored threshold mode table | `analysis/clean_transfer/analyze_implementation_transfer.py`; `analyze_design_wilcoxon.py` |
| 14 and 15 | gain scheme tables | `analysis/clean_transfer/analyze_implementation_transfer.py`; `analyze_design_wilcoxon.py` |
| 16 and 17 | derivative and integral role figures | `analysis/ablation/make_controller_ablation_term_roles.py`; `analysis/generate_paper_figures.py` |
| 18 | selected controller removal table | `analysis/generate_paper_figures.py` |
| 19 through 21 | removal figures | `analysis/generate_paper_figures.py` |
| 22 and 23 | selected controller feedback tables | `analysis/generate_paper_figures.py` |
| 24 and 25 | feedback heatmaps | `analysis/imperfect_feedback/analyze_imperfect_feedback_results.py` |
| 26 | strongest feedback raw residual table | `analysis/imperfect_feedback/analyze_imperfect_feedback_results.py` |

Run `make figures` to regenerate the paper figures from the processed data.
Run `make test` to check method definitions, seed maps, and archived numerical
values.
