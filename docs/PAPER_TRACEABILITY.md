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
| Demand classes | `simulator/src/ftarget.c`; `experiments/definitions.py` | `data/provenance/random_path_runtime_audit.json` |
| CT, LFTA, SBTA, SETA, and PTA | `simulator/src/ftracker.c`; `configs/methods.json` | `tests/test_simulator_contract.py` |
| HM, HT1, and HT2 threshold ranges | `simulator/src/ftracker.c`; `configs/methods.json` | 27 configuration check in `experiments/definitions.py` |
| Clamped and latent stored threshold modes | `simulator/src/ftracker.c`; `configs/methods.json` | `data/provenance/preflight_source_audit.json` |
| Global and Agent gain schemes | `simulator/src/ftracker.c`; `experiments/definitions.py` | selected values in `data/parameters/reference_tuned_parameters.csv` |
| Reference tuning | `experiments/tune.py` | exact seeds in `data/seeds/tuning_seed_map.csv` |
| Clean feedback allocation grid | `experiments/run.py allocation` | exact seeds in `data/seeds/allocation_seed_map.csv` |
| Repeated reversal ablation | `experiments/run.py ablation` | `analysis/ablation/term_roles.py` |
| Sustained plateau diagnostic | archived raw schema in `docs/DATA.md` | processed paired results in `data/processed/ablation` |
| Population diagnostic | `analysis/population/run.py` | replay files in `data/processed/population` |
| Agent removal | `experiments/run.py removal` | `analysis/removal` |
| Imperfect feedback | `experiments/run.py feedback` | exact perturbation seeds in `data/seeds/feedback_seed_map.csv` |
| Metrics | `simulator/src/output.c` | golden values in `tests/golden_smoke_results.csv` |

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
| Evaluation grids | `configs/experiment_design.json`; `experiments/run.py` |
| Reference tuning search spaces | `experiments/tune.py`; `docs/EXPERIMENTS.md` |
| Selected PTA transfer, paired imbalance | `data/processed/allocation/family_comparison_summary.csv`; `range_matched_confirmatory_summary.csv` |
| Selected PTA transfer, raw residual | `data/processed/allocation/family_comparison_summary.csv` |
| All PTA configurations, paired imbalance and raw residual | `data/processed/allocation/all_variant_summary.csv` |
| Three broadly useful PTA configurations | `data/processed/allocation/winner_counts.csv`; `all_variant_summary.csv` |
| Stored threshold modes | `data/processed/allocation/pta_storage_transfer_summary.csv`; `seta_storage_transfer_summary.csv`; `storage_mode_wilcoxon_summary.csv` |
| Agent and Global gain schemes | `data/processed/allocation/gain_transfer_summary.csv`; `gain_scheme_wilcoxon_summary.csv` |
| Selected controller after removal | `data/processed/removal/selected_configuration_comparison.csv` |
| Selected controller under imperfect feedback | `data/processed/feedback/selected_configuration_strongest.csv` |
| Selected controller degradation | `data/processed/feedback/selected_configuration_own_degradation.csv` |
| Strongest feedback raw residual | `data/processed/feedback/selected_severity_summary.csv` |

## Figures and explicit conclusions

| Conclusions | Figure or table | Regeneration source |
|---|---|---|
| 1 and 2 | selected transfer tables | `analysis/allocation/compare.py` |
| 3 and 4 | all configuration tables | `analysis/allocation/configurations.py` |
| 5 | three configuration coverage table | `data/processed/allocation/all_variant_summary.csv` |
| 6 | demand class advantage | `analysis/figures.py` |
| 7 and 8 | step ratio error and advantage | `analysis/figures.py` |
| 9 and 10 | population and task count advantage | `analysis/allocation/scaling.py`; `analysis/figures.py` |
| 11 | population recruitment variability | `analysis/population/plot.py` |
| 12 | preferred PTA configuration atlas | `analysis/allocation/winner_atlas.py` |
| 13 | stored threshold mode table | `analysis/allocation/storage_and_gains.py`; `analysis/allocation/wilcoxon.py` |
| 14 and 15 | gain scheme tables | `analysis/allocation/storage_and_gains.py`; `analysis/allocation/wilcoxon.py` |
| 16 and 17 | derivative and integral role figures | `analysis/ablation/term_roles.py`; `analysis/figures.py` |
| 18 | selected controller removal table | `analysis/figures.py` |
| 19 through 21 | removal figures | `analysis/figures.py` |
| 22 and 23 | selected controller feedback tables | `analysis/figures.py` |
| 24 and 25 | feedback heatmaps | `analysis/feedback/compare.py` |
| 26 | strongest feedback raw residual table | `analysis/feedback/compare.py` |

Run `make figures` to regenerate the paper figures from the processed data.
Run `make test` to check method definitions, seed maps, and archived numerical
values.
