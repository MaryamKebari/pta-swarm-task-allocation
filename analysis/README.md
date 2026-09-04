# Analysis

Analysis is grouped by the scientific question it answers.

| Directory | Question |
|---|---|
| `clean_transfer` | How does PTA compare with alternative update methods, and how do operating and design factors change the result? |
| `ablation` | What roles do integral and derivative action play? |
| `population` | How does recruitment variability change with population size? |
| `agent_removal` | How does performance change after agents and their capacity are removed? |
| `imperfect_feedback` | How do feedback noise and persistent bias affect performance? |

`generate_paper_figures.py` creates the principal one-conclusion figures from
the committed processed tables. `scripts/make_figures.py` is the public entry
point; run it with `make figures`. The file-by-file map is in
[`docs/FIGURES.md`](../docs/FIGURES.md).

Scripts that rebuild summaries from raw data use the environment variables in
`docs/DATA.md`. They validate expected columns and experimental coverage before
reporting statistics.
