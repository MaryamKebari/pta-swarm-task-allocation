# Figure guide

The committed images in `docs/assets` are quick previews. The analysis scripts
regenerate publication files in `figures/`.

| Preview | Claim supported | Horizontal axis | Vertical axis or rows |
|---|---|---|---|
| `demand_class_pta_advantage.png` | PTA's advantage varies with demand structure | median reduction in paired imbalance, `R` | four demand classes |
| `population_pta_advantage.png` | PTA's advantage persists across tested population sizes | population size | median reduction in `R` |
| `frozen_reference_winner_atlas.png` | the preferred PTA configuration changes across settings | step ratio and task count | demand class and population groups |
| `agent_removal_deterioration.png` | error rises as capacity is removed | removal percentage | post removal error or relative degradation |
| `imperfect_feedback_pta_comparators.png` | PTA retains a relative advantage under noise and bias | feedback severity | comparator specific effect |

Effects are paired within the matching unit defined by the corresponding
analysis. A positive percentage reduction means PTA has lower error. Equality
lines and zero effect references are therefore neutral boundaries, not fitted
trends.
