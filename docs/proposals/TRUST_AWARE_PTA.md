# Proposal: trust-aware PTA with Byzantine and strategic agents

Status: design sketch for a follow-up paper. Nothing here changes the
simulator yet, and the published campaign results are unaffected.

Working title: *Trust-Aware Threshold Adaptation for Swarm Task Allocation
with Strategic and Byzantine Agents.*

## 1. How feedback flows today

In the current simulator, every agent reads the same oracle feedback signal.
Agents never exchange messages, so there is nobody to trust or distrust yet.

| Step | Where | What happens |
|---|---|---|
| 1 | `fxn.c` `update_pre_service_task_error()` (line ~903) | Computes one global `Task_feedback_error[task]` per step and accumulates one global `Task_error_integral[task]`. |
| 2 | `fxn.c` `noisy_task_feedback()` (~1000) | Optional noise and bias, applied to that single broadcast channel. All agents see the same corrupted value. |
| 3 | `fxn.c` `apply_pid_integral_antiwindup()` (~1157) | Leak `Pid_integral_leak` and bound `Pid_integral_bound`, applied to the global integral. |
| 4 | `ftracker.c` `task_stimulus()` (~24) | Selector stimulus `max(e, 0)`, the same for every agent. |
| 5 | `ftracker.c` `select_tasks()` / `agent_select_task()` (~189, ~317) | Threshold test and task choice. |
| 6 | `ftracker.c` `adjust_agent_thresholds()` (~505) | PTA update from the global P, I and D terms. Only the gains can differ per agent (`Agent_pid_gains`). |
| 7 | `ftracker.c` `count_task_actors()` (~1051), `fxn.c` `apply_task_service()` (~1031) | Every agent that selects a task delivers full service. |

Two things follow from this:

1. **Trust needs a communication model first.** The feedback has to be
   assembled from agent reports, not read from an oracle.
2. **The shared integral is a single point of failure.** Any persistent bias
   in the feedback accumulates in `Task_error_integral`. With leak `λ` and
   bound `B`, a constant bias `δ` settles at about `min(δ / (1 − λ), B)`, which
   amplifies small, hard-to-detect lies by `1 / (1 − λ)`. This is the
   vulnerability the paper should demonstrate first.

The existing `Feedback_bias_mode` and `Feedback_noise_*` options already model
a *corrupted channel*. They are a useful baseline: "everyone sees the same lie"
is a different and easier problem than "some peers lie to you".

## 2. Model change: report-based feedback

Each step, for each task `j`:

1. **Private observation.** Agent `i` observes
   `o[i,j] = e[j] + σ·z[i,j]`, reusing the hash-based noise generator in
   `feedback_noise_uniform()` with the agent index folded into the salt.
2. **Reports.** Agent `i` samples `K` peers (or uses a fixed neighbour graph)
   and receives reports `m[k,j]`. Honest agents report `m[k,j] = o[k,j]`.
3. **Aggregation.** Agent `i` forms its own feedback estimate
   `ê[i,j] = (w[i,i]·o[i,j] + Σ_k w[i,k]·m[k,j]) / (w[i,i] + Σ_k w[i,k])`.
4. **Per-agent controller state.** The previous feedback value and the
   integral become per-agent: `ê[i,j]`, `ê_prev[i,j]` and `I[i,j]`, with the
   same leak and bound. The selector stimulus becomes `max(ê[i,j], 0)`.

**Reduction property (must be a regression test).** With `K` equal to all
agents, `σ = 0`, no adversaries and all weights equal to 1, `ê[i,j] = e[j]`
for every agent, so the model reproduces current PTA exactly. The golden smoke
output in `tests/golden_smoke_results.csv` must stay unchanged with the new
options switched off.

## 3. Adversary models

Add `Agent[n].role` (0 = honest) and a fraction parameter `Adversary_fraction`,
assigned with a seeded draw so that runs stay reproducible.

| ID | Type | Behaviour | Why it matters |
|---|---|---|---|
| A1 | Inverting liar | Reports `−o` | Crude and easy to detect. Sets a lower bar for the trust model. |
| A2 | Integral poisoner | Reports `o + δ` with small `δ` inside the honest noise band | Exploits the `1 / (1 − λ)` amplification of the I term. This is the headline attack. |
| A3 | Freeloader | Selects a task and is counted as working, but delivers no service | Attacks service rather than information. Needs audits to detect. |
| A4 | Colluders | A coordinated group of A2 agents with the same `δ` sign | Defeats median-style aggregation once the group is large enough. |
| S | Strategic | Picks its threshold to maximize its own utility (Section 5) | The game-theoretic case: self-interested rather than malicious. |

For A3, `count_task_actors()` needs to count *claimed* actors (what peers see)
separately from *effective* actors (what `apply_task_service()` multiplies by
the per-agent capacity).

## 4. Trust mechanism

**Beta reputation per peer.** Agent `i` keeps `(a[i,k], b[i,k])` for each peer
`k`, and the trust weight is `w[i,k] = a / (a + b)`.

- **Report consistency.** When `|m[k,j] − o[i,j]| ≤ τ·σ`, increment `a`;
  otherwise increment `b`.
- **Service audits.** With probability `p_audit`, agent `i` checks whether a
  peer that claimed task `j` actually delivered service. A delivery increments
  `a` and a missing one increments `b`. This is what catches freeloaders.
- **Forgetting.** Multiply `a` and `b` by `ρ < 1` each step so that trust can
  recover and adapt to churn. This connects to the existing `Kill_number`
  removal experiments.
- **Newcomer prior.** `(a0, b0)` sets how much new agents are trusted, which
  lets you study the trade-off between robustness and how fast newcomers
  become useful.

Store trust as one `Pop_size × Pop_size` pair of arrays allocated at
initialization rather than inside the `AGENT` struct, since `AGENT` already
holds several `MAX_TASKS` arrays.

**Baselines for the aggregation step** (needed to show that trust adds value
beyond robust statistics):

- unweighted mean of reports, which is report-based PTA with no defence;
- coordinate-wise median and trimmed mean, a standard Byzantine-robust
  aggregator with no memory;
- trust-weighted mean, the proposed method;
- trust-weighted mean with trimming, which is likely the most robust
  combination.

## 5. Game-theoretic layer

**Strategic agents.** A strategic agent pays effort cost `c` per step of work
and receives a credited reward `r · credit[i]`, where the credit comes from the
shared outcome. Its utility is `u[i] = r · credit[i] − c · work[i]`.

- **Free-riding equilibrium.** When credit is shared equally, the best response
  is to raise one's threshold and let others work. Measure the gap between the
  selfish equilibrium and cooperative PTA as an empirical price of anarchy:
  `PoA = R_selfish / R_cooperative`.
- **Trust as a mechanism.** Credit each agent in proportion to its trust weight
  times its audited contribution. A freeloader or liar who is caught with
  probability `p_audit` loses a reputation-weighted stake `L`. Honest work is a
  best response roughly when `p_audit · L ≥ c`. Deriving this
  incentive-compatibility condition properly is a clean theorem target.
- **Potential-game view.** Task choice with payoff that falls as more agents
  pile onto a task is a congestion game, and congestion games are potential
  games. A worthwhile question is whether threshold dynamics (CT at a minimum)
  are better-response dynamics in that game. If so, convergence follows from
  Monderer and Shapley.

## 6. Theory targets

| ID | Statement | Difficulty |
|---|---|---|
| T1 | Reduction: trust-aware PTA equals PTA when no adversaries are present, `K` covers all agents and `σ = 0`. | Easy; proof by construction |
| T2 | Integral amplification: without trust, an A2 coalition of fraction `f` shifts the integral by about `min(f·δ / (1 − λ), B)`. | Easy; linear recursion |
| T3 | Bounded influence: with beta trust, the expected weight on a liar decays geometrically after detection, so the integral shift is bounded independently of the attack duration. | Medium |
| T4 | Incentive compatibility: the `p_audit · L ≥ c` condition from Section 5. | Medium |
| T5 | Breakdown point of median aggregation versus trust under colluders (A4). | Medium to hard |

T2 alone, paired with a figure showing it in simulation, is a strong
motivating result.

## 7. Experiment grid

| Factor | Levels |
|---|---|
| Method | CT, LFTA, SBTA, SETA, PTA, report-PTA (mean), report-PTA (median), trust-PTA |
| Adversary type | A1, A2, A3, A4, S |
| Adversary fraction `f` | 0, 0.1, 0.2, 0.3, 0.4 |
| Sample size `K` | 3, 8, all |
| Demand class | Reuse the four existing classes in `ftarget.c` |
| Threshold mode | Clamped and latent (the latent mode may be more exposed to poisoning) |

Outcomes:

- `R` and `R_abs`, which already exist;
- detection time;
- false distrust rate (honest agents losing trust);
- breakdown fraction `f*` at which `R` exceeds the CT baseline;
- messages per step.

Headline figures:

1. `R` against `f` for each method and adversary type;
2. the integral trajectory under an A2 attack, with and without trust;
3. empirical price of anarchy against `p_audit`.

## 8. Implementation plan

The simulator source is hash-checked by `make verify` against
`data/provenance/simulator_source_sha256.csv`. To keep the published results
reproducible:

1. Tag the current paper commit (for example `paper-v1`) before touching
   `simulator/src`.
2. Add new parameters in `params.c` and `global.h`/`extern.h`, all off by
   default: `Report_feedback`, `Report_k`, `Obs_noise_alpha`,
   `Adversary_fraction`, `Adversary_type`, `Adversary_delta`, `Trust_mode`
   (0 = none, 1 = beta, 2 = median, 3 = trimmed), `Trust_tau`,
   `Trust_forget`, `Audit_prob`, `Strategic_cost`.
3. Add a feedback stage in `fxn.c` beside `update_pre_service_task_error()`
   that fills the per-agent `ê`, `ê_prev` and `I` arrays when
   `Report_feedback` is on.
4. Give `task_stimulus()` and `agent_task_threshold()` in `ftracker.c` an
   agent argument, and make them read per-agent feedback when the new mode is
   on.
5. In `adjust_agent_thresholds()`, read per-agent feedback and integral when
   the mode is on. Keep the global path untouched otherwise.
6. Split claimed and effective actors in `count_task_actors()` for A3.
7. Add the new outcomes in `output.c`.
8. Add tests:
   - the T1 reduction, checked bit-for-bit against the global path;
   - the golden smoke output unchanged with defaults;
   - an A2 attack test checking that the integral shift matches T2;
   - a test checking that trust weights fall for A1 within a set number of
     steps.
9. Regenerate the provenance hashes in a separate commit that clearly states
   the simulator was extended.

Suggested order of work:

1. Steps 2–5 and the T1 test, for report-based PTA with honest agents.
2. A2 attack plus T2, which gives the motivating figure.
3. Beta trust and the median baselines, which give the main result.
4. A3 audits and strategic agents, for the game-theoretic section.

Stop after the third step if time is short. That alone is a complete paper.

## 9. References to position against

- Gerkey and Matarić (2004): taxonomy of multi-robot task allocation.
- Bonabeau, Theraulaz and Deneubourg (1996): response threshold models.
- Huynh, Jennings and Shadbolt (FIRE); Teacy et al. (TRAVOS); Jøsang and
  Ismail (beta reputation): trust models.
- Ramchurn, Huynh and Jennings (2004): survey of trust in multi-agent systems.
- Monderer and Shapley (1996): potential games.
- Marden, Arslan and Shamma: game-theoretic distributed control.
- Blanchard et al. (2017) and Yin et al. (2018): Byzantine-robust aggregation
  (Krum, median, trimmed mean).
- Strobel, Castelló Ferrer and Dorigo: blockchain-based Byzantine swarms, the
  closest swarm-robotics prior work.

Before citing any of these, check the exact titles, years and venues.
