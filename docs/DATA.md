# Data guide

## Included data

The repository commits compact processed results used by the statistical
analyses and figures. They are grouped by experiment:

| Directory | Examples | Interpretation |
|---|---|---|
| `clean_transfer` | range matched summaries, winner tables, Wilcoxon tests | Clean feedback comparison and design factors |
| `ablation` | direct and persistent term summaries | P, I, and D roles |
| `population` | replay validation and trajectories | Recruitment scaling diagnostic |
| `agent_removal` | post window means, degradation, comparator summaries | Performance after removal |
| `imperfect_feedback` | condition means and severity summaries | Noise and bias robustness |

Every CSV uses a header row. Counts, aggregation units, and statistical columns
are kept in the files rather than inferred from filenames.

## Raw data layout

The per run files are too large for ordinary Git hosting. To repeat analyses
from raw output, place them at:

```text
data/raw/
├── allocation_grid/per_run_results.csv
├── controller_ablation/per_run_results.csv
├── agent_removal/per_run_results.csv
├── agent_removal_zero_post_control/per_run_results.csv
└── imperfect_feedback/per_run_results.csv
```

The analysis entry points also accept these environment variables:

| Variable | Raw file |
|---|---|
| `PTA_ALLOCATION_CSV` | clean feedback allocation grid |
| `PTA_ABLATION_CSV` | crossed controller ablation |
| `PTA_PLATEAU_ABLATION_CSV` | plateau diagnostic, when separate |
| `PTA_REMOVAL_CSV` | agent removal runs |
| `PTA_FEEDBACK_CSV` | imperfect feedback runs |

Large raw files should be published as a release asset or archival dataset, not
committed directly to Git history.

## Parameter table

`data/parameters/reference_tuned_parameters.csv` contains one row for every
configuration. The core columns identify method family, threshold range, stored
threshold mode, gain scheme, and the selected update parameters. The
`source_row` column preserves the complete original tuning record for audit.

## Data integrity

Run `make verify` to check the experiment grid and source provenance. Run
`make checksums` to regenerate `CHECKSUMS.sha256` after an intentional data or
code update.
