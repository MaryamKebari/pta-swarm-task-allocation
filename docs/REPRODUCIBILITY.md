# Reproducibility guide

## Fast verification

```bash
python3 -m venv .venv
source .venv/bin/activate
python -m pip install -r requirements.txt
make build
make smoke
make test
make verify
make figures
```

The smoke test runs in a temporary directory with fixed agent and target path
seeds. It checks that the simulator completes and emits the paper critical
metrics. The unit tests verify the selector stimulus derivative, target path RNG
initialization order, feedback clipping default, processed data coverage, and
archived numerical regression values for all five method families. The
regression allows a small cross platform tolerance because compiler and math
library differences can alter stochastic threshold decisions near equality.

## Reproduce public analyses

The default figure commands consume `data/processed`, so they do not require the
large per run data. To rebuild statistical summaries from raw data, use the
experiment specific analysis modules and the layout described in `DATA.md`.
Each module writes to its corresponding processed data directory.

## Reproduce the full campaigns

Full campaigns require millions of simulator calls and are intended for a Linux
compute host. `experiments/tune.py` implements reference tuning, and
`experiments/run.py` implements the clean feedback, repeated reversal
ablation, removal, and imperfect feedback grids. Use
`configs/experiment_design.json` as the authoritative factor specification,
`configs/methods.json` for implementation mappings, and
`data/parameters/reference_tuned_parameters.csv` for the published selections.

First confirm the environment:

```bash
make build
make campaign-smoke
make tuning-smoke
```

Then launch one audited stage at a time:

```bash
python experiments/tune.py --workers 8 --scratch-root /local/scratch/pta
python experiments/run.py allocation --workers 8 --scratch-root /local/scratch/pta
python experiments/run.py ablation --workers 8 --scratch-root /local/scratch/pta
python experiments/run.py removal --workers 8 --scratch-root /local/scratch/pta
python experiments/run.py feedback --workers 8 --scratch-root /local/scratch/pta
```

Each validation runner appends completed rows and skips matching rows on an
identical restart. It never changes the simulator source or base parameter
snapshot. Use `--dry-run` to record and inspect a campaign plan, and use factor,
family, or configuration filters to create compute shards. Do not merge shards
unless their manifests, parameter table, and seed maps match.

Expected paper grid rows are 388,800 clean feedback rows, 12,800 repeated
reversal ablation rows, 2,721,600 removal rows including the matched zero
removal window, and 1,296,000 imperfect feedback rows. Reference tuning uses
64,800 simulator calls. The separate sustained plateau diagnostic is described
in `docs/PAPER_TRACEABILITY.md` and analyzed through the committed ablation
module.
Maintain these invariants:

1. use 1,000 steps and 100 validation repetitions;
2. match simulation and target path seeds across methods;
3. initialize the target path stream from `TargetPathSeed` after reading params;
4. use the same standardized feedback perturbations across methods;
5. do not clip perturbed feedback;
6. evaluate using true post service residuals;
7. calculate removal metrics on steps 500 through 999 for every severity;
8. record source, specification, and parameter checksums with every campaign.

The final campaign audit files are retained in `data/provenance`.

## Environment capture

For an archival release, record the compiler version, operating system, Python
version, installed packages, and commit hash.
