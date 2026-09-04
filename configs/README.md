# Configuration files

| File | Purpose |
|---|---|
| `experiment_design.json` | Authoritative factor levels, repetition counts, tuning budgets, and metrics |
| `methods.json` | Method family, threshold range, stored threshold mode, and gain scheme definitions |
| `params.default` | Exact base parameter snapshot used by the final campaign |
| `opfiles.full` | Complete simulator output registry and default output choices |
| `smoke.params` | Fast PTA regression run |
| `opfiles.smoke` | Minimal outputs for the smoke run |

The JSON files are preferred for programmatic inspection. The text parameter
files retain the format expected by the C simulator. The checksum of
`params.default` is verified against the final campaign manifest; explanatory
definitions live in `docs/METHODS.md` rather than inside that immutable file.
