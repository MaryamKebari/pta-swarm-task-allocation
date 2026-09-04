# Research data

- `parameters` contains the selected adaptive parameters.
- `manifests` contains source hashes, seed maps, and runtime audits.
- `processed` contains compact tables used by public statistics and plots.
- `raw` is the documented location for optional large per-run outputs and is
  excluded from Git.

The seed maps are required to re-run the paper campaigns:

| File | What it pins |
|---|---|
| `manifests/tuning_seed_map.csv` | 20 shared tuning seeds |
| `manifests/allocation_seed_map.csv` | 14,400 clean-feedback validation seed pairs |
| `manifests/imperfect_feedback_seed_map.csv` | matched noise and bias seeds |

See `docs/DATA.md` for schemas, expected paths, environment variables, and
archival guidance.

