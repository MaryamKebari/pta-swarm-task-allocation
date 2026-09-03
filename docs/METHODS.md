# Methods and terminology

## Task allocation model

The simulator represents demand and delivered service as cumulative vectors over
paired task channels. Each pair encodes the positive and negative directions of
one signed demand axis. At a simulation step, an agent deterministically becomes
eligible for tasks whose nonnegative stimulus exceeds its effective threshold.
If several tasks are eligible, it selects one uniformly at random. An agent that
selects no task contributes no service during that step.

Every active agent has the same service capacity. The service delivered to task
`j` is the number of agents selecting that task multiplied by the per agent
capacity. Excess service is allowed, so the cumulative residual may be negative.

## Feedback and PTA

For task `j` and its opposite task `j'`, PTA receives signed paired feedback

```text
e[j,t] = r[j,t] - r[j',t]
```

where `r` is the post service cumulative residual. The selector stimulus is
`S[j,t] = max(e[j,t], 0)`. PTA combines:

- proportional input: the current signed feedback;
- integral input: bounded, leaky accumulated signed feedback;
- derivative input: `S[j,t] - S[j,t-1]`.

The derivative is intentionally computed from selector stimulus rather than raw
signed feedback. Tests in `tests/test_simulator_contract.py` guard this behavior.

## Method families

| Acronym | Meaning | Simulator mode |
|---|---|---:|
| CT | Constant thresholds | `Pid = 4` |
| LFTA | Learning and forgetting threshold adaptation | `Pid = 0` |
| SBTA | Sign based threshold adaptation | `Pid = 3` |
| SETA | Single error threshold adaptation | `Pid = 2` |
| PTA | Proportional, integral, and derivative threshold adaptation | `Pid = 1` |

The complete machine readable mapping is in `configs/methods.json`.

## Design factors

- **Threshold range** means HM, HT1, or HT2.
- **Stored threshold mode** means clamped or latent. Clamped mode stores every
  update inside the admissible interval. Latent mode retains the unconstrained
  stored state and clamps only the threshold used for selection.
- **Gain scheme** means Global or Agent. Global assigns one tuned set of gains
  to all agents. Agent reproducibly perturbs each gain around its tuned base
  value.

These terms are used consistently in the code, tables, and documentation.

## Primary outcomes

`R` is the time averaged root mean square of the paired residual. It measures
tracking imbalance along the signed task axes. `R_abs` is the complementary time
averaged root mean square of the raw directional channel residual. Lower values
are better. Additional Euclidean and switching measures are retained in the raw
and processed results.
