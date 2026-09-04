# Contributing

This repository is the public companion to a research paper. The first priority
is that the published simulator, seeds, and processed results stay reproducible.

Questions about the paper results or reproduction steps are welcome through
GitHub issues. Please include the operating system, compiler, Python version,
exact command, and the relevant manifest or checksum.

Before opening a pull request:

```bash
make test
make verify
```

Changes to simulator behavior must include a focused test and must explain
whether they preserve or intentionally change the controller defined in the
paper. Do not replace paired seeds with independently generated method seeds.
Do not commit raw experiment output, compiled binaries, or machine-specific
paths. Do not reformat `simulator/src` unless the change is required for
correctness; those files are hash-checked against the final campaign.
