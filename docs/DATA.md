# Data guide

Processed tables are grouped by the same experiment names as `analysis/`
and `experiments/run.py`.

| Directory | Contents |
|---|---|
| `data/processed/allocation/` | method comparison, winners, Wilcoxon tests |
| `data/processed/ablation/` | P, I, and D term summaries |
| `data/processed/population/` | recruitment diagnostic |
| `data/processed/removal/` | post-removal means and degradation |
| `data/processed/feedback/` | noise and bias summaries |
| `data/parameters/` | selected reference-tuned parameters |
| `data/seeds/` | exact campaign seeds |
| `data/provenance/` | simulator hashes and runtime audits |
| `data/raw/` | optional large per-run CSVs, not in Git |

Every CSV uses a header row.

## Raw data layout

To rebuild summaries from raw output, place files at:

```text
data/raw/
├── allocation/per_run_results.csv
├── ablation/per_run_results.csv
├── ablation/plateau_metrics.csv
├── removal/per_run_results.csv
└── feedback/per_run_results.csv
```

Environment variables accepted by the analysis scripts:

| Variable | Raw file |
|---|---|
| `PTA_ALLOCATION_CSV` | allocation grid |
| `PTA_ABLATION_CSV` | term ablation |
| `PTA_PLATEAU_ABLATION_CSV` | plateau diagnostic, when separate |
| `PTA_REMOVAL_CSV` | agent removal |
| `PTA_FEEDBACK_CSV` | imperfect feedback |

## Seed maps

`data/seeds/allocation_seed_map.csv` has the 14,400 validation seed pairs for
the 144 allocation conditions. `data/seeds/feedback_seed_map.csv` adds matched
noise and bias seeds. `data/seeds/tuning_seed_map.csv` has the 20 shared
tuning seeds. Campaign runners read these files; they do not draw replacement
seeds at runtime.

## Parameter table

`data/parameters/reference_tuned_parameters.csv` has one row per configuration.

Run `make verify` to check the experiment grid and source provenance.
