# Contributing

Bug reports and reproducibility questions are welcome through GitHub issues.
Please include the operating system, compiler, Python version, exact command,
and the relevant manifest or checksum.

Before opening a pull request:

```bash
make test
make verify
```

Changes to simulator behavior must include a focused test and must explain
whether they preserve or intentionally change the controller defined in the
paper. Do not replace paired seeds with independently generated method seeds.
Do not commit raw experiment output, compiled binaries, or machine specific
paths.

