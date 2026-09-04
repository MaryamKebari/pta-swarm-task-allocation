# Analysis

Each folder matches one paper experiment. Processed tables live in
`data/processed/` under the same names.

| Folder | Paper experiment | Regenerates figures? |
|---|---|---|
| `allocation/` | allocation accuracy across operating conditions | `winner_atlas.py` |
| `ablation/` | integral and derivative term roles | `term_roles.py` |
| `removal/` | agent removal | via `../figures.py` |
| `feedback/` | noise and bias | `compare.py` |
| `population/` | recruitment vs population size | `plot.py` |

`python analysis/figures.py` (or `make figures`) regenerates every paper-facing
plot from `data/processed`. Scripts that rebuild those tables from raw campaign
CSVs are in the same folders; they need the large files described in
[`docs/DATA.md`](../docs/DATA.md).
