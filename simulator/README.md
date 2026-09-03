# Simulator

`src` contains the C11 simulator used by the final experiments. The source is
kept as the verified research implementation rather than reformatted in ways
that could silently change behavior. Its provenance is documented in
`docs/PROVENANCE.md` and checked by `make verify`.

Build from the repository root:

```bash
make build
```

Run a small verified example:

```bash
make smoke
```

The executable accepts an override parameter file and an output selection file:

```text
simulator/src/sim PARAMS_FILE OPFILES_FILE
```

It first reads `params.default` and `opfiles.default` from the current working
directory. The smoke wrapper creates these files in a temporary directory. For
custom runs, use `configs/params.default` as the documented base and apply only
the settings required by the experiment.

The important controller dispatch values are listed in `configs/methods.json`.
The output field definitions are summarized in `docs/DATA.md` and implemented
in `src/output.c`.
