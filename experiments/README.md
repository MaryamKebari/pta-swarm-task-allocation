# Experiment runners

The two entry points separate parameter selection from validation.

| Entry point | Purpose | Full scale |
|---|---|---:|
| `tune_reference.py` | Tune the 24 adaptive configurations at the reference setting | 64,800 simulations |
| `run_campaign.py clean` | Evaluate all 27 configurations with clean feedback | 388,800 rows |
| `run_campaign.py ablation` | Cross P, PI, PD, and full PTA under repeated reversal demand | 12,800 rows |
| `run_campaign.py removal` | Remove 0% through 50% of agents at step 500 | 2,721,600 rows |
| `run_campaign.py feedback` | Cross five noise and four bias levels | 1,296,000 rows |

Both runners execute the verified C simulator in isolated temporary directories.
They never edit `simulator/src`, `configs/params.default`, or previous results.

## Before a full run

```bash
make build
make campaign-smoke
make tuning-smoke
```

The campaign smoke test exercises CT, LFTA, SBTA, SETA, and PTA. The unit test
suite also checks these outputs against values archived from the final paper
campaign.

## Shards and restarts

Validation runs accept filters such as:

```bash
python experiments/run_campaign.py clean \
  --families PTA SETA \
  --populations 50 100 \
  --tasks 4 8 \
  --step-ratios 1.5 2.0 \
  --patterns scurve zigzag \
  --output data/raw/shards/pta_seta_part1.csv
```

Completed rows are appended immediately. An identical restart skips rows whose
complete experimental key is already present. Use a different output path when
changing filters or source versions, and retain the adjacent `manifest.json`.

`--scratch-root` places temporary simulator directories on fast local storage.
`--workers` controls concurrent simulator processes. Start conservatively and
increase it only after checking memory, storage, and CPU use.

## Identifiers

`run_method_id` has four fields separated by pipes:

```text
family|method|stored threshold mode|gain scheme
```

The final field is empty for CT, LFTA, SBTA, and SETA. Ablation rows append the
variant as a fifth field. This reproduces the archived raw data convention.
