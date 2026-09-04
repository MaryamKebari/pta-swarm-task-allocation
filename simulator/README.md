# Simulator

`src` contains the C11 simulator used by the final experiments. The source is
kept as the verified research implementation. Do not reformat it in ways that
could silently change behavior: `make verify` checks file hashes against
[`docs/PROVENANCE.md`](../docs/PROVENANCE.md).

## Source files and paper roles

| File | Role |
|---|---|
| `main.c` | program entry |
| `sim.c` | time loop; initializes target-path RNG after parameters are read |
| `params.c`, `params.h` | parameter file I/O |
| `ftracker.c`, `ftracker.h` | threshold updates for CT, LFTA, SBTA, SETA, and PTA |
| `ftarget.c`, `ftarget.h` | demand / target paths, including the four demand classes |
| `fxn.c`, `fxn.h` | service delivery, leaky integral, agent-specific gains |
| `output.c`, `output.h` | metrics `R`, `R_abs`, switching, and post-removal windows |
| `random.c`, `random.h` | random-number streams |
| `types.h`, `global.h`, `extern.h`, `sim.h` | shared types and declarations |
| `animate.c`, `gnu.c` | optional visualization helpers, not used for paper statistics |

Method dispatch values (`Pid`) are listed in
[`configs/methods.json`](../configs/methods.json):

| `Pid` | Method |
|---:|---|
| 0 | LFTA |
| 1 | PTA |
| 2 | SETA |
| 3 | SBTA |
| 4 | CT |

## Build and run

From the repository root:

```bash
make build
make smoke
```

The executable accepts an override parameter file and an output selection file:

```text
simulator/src/sim PARAMS_FILE OPFILES_FILE
```

It first reads `params.default` and `opfiles.default` from the current working
directory. The smoke wrapper creates those files in a temporary directory. For
custom runs, start from `configs/params.default` and override only the settings
required by the experiment.

Paper campaigns should be launched through
[`experiments/run.py`](../experiments/run.py) rather than by
calling `sim` by hand, so that seeds, parameter snapshots, and output columns
stay matched.
