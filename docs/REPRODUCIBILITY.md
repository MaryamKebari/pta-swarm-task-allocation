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
initialization order, feedback clipping default, and processed data coverage.

## Reproduce public analyses

The default figure commands consume `data/processed`, so they do not require the
large per run data. To rebuild statistical summaries from raw data, use the
experiment specific analysis modules and the layout described in `DATA.md`.
Each module writes to its corresponding processed data directory.

## Reproduce the full campaigns

Full campaigns require millions of simulator calls and are intended for a Linux
compute host. Use `configs/experiment_design.json` as the authoritative factor
specification, `configs/methods.json` for implementation mappings, and
`data/parameters/reference_tuned_parameters.csv` for the selected parameters.
Maintain these invariants:

1. use 1,000 steps and 100 validation repetitions;
2. match simulation and target path seeds across methods;
3. initialize the target path stream from `TargetPathSeed` after reading params;
4. use the same standardized feedback perturbations across methods;
5. do not clip perturbed feedback;
6. evaluate using true post service residuals;
7. calculate removal metrics on steps 500 through 999 for every severity;
8. record source, specification, and parameter checksums with every campaign.

The final campaign audit files are retained in `data/manifests`.

## Environment capture

For an archival release, record the compiler version, operating system, Python
version, installed packages, commit hash, and `CHECKSUMS.sha256`. A tagged GitHub
release can then be archived with Zenodo to obtain a DOI.
