# Provenance

This repository is a curated copy. The original development tree and raw
research results remain unchanged in the private archive.

## Verified simulator source

The final Linux campaign records the following SHA 256 value for `ftracker.c`:

```text
86ba010cdf4fe7d097fdfd3328331bddf54841efa4cdc547156cf809c54d35f9
```

The file at `simulator/src/ftracker.c` has the same value. In addition, the
complete source tree was reconstructed from the archived campaign base and the
four recorded final corrections: the selector stimulus derivative, required
metrics, switch counting, and independent random demand stream. Every compiled
C source file, header, and the Makefile in the reconstructed tree matches the
curated simulator byte for byte. Their hashes are recorded in
`data/provenance/simulator_source_sha256.csv` and checked by `make verify`.

The complete campaign hash record is preserved in
`data/provenance/source_hashes.json`. The public `configs/params.default` is the
exact final campaign base parameter snapshot and is also checked by
`make verify`. The 27 rows in
`data/parameters/reference_tuned_parameters.csv` are also byte for byte
identical to the archived selected parameter table. The historical Linux
binary hash is retained for provenance but is not expected to match a binary
rebuilt on another operating system.

## Preserved implementation behavior

The verified simulator retains:

- signed paired post service feedback;
- PTA derivative `max(e[t], 0) - max(e[t-1], 0)`;
- bounded leaky integral accumulation;
- clamped and latent stored threshold modes;
- Global and reproducible Agent gain schemes;
- independent target path random numbers;
- capacity loss after removal;
- unclipped imperfect feedback;
- paired imbalance, raw residual, Euclidean, switching, and post removal metrics.

The public wrappers and documentation do not silently alter these choices.

## Report-feedback extension

After the paper campaign, the simulator was extended with optional
report-based feedback (`simulator/src/freport.c`, see
[`proposals/TRUST_AWARE_PTA.md`](proposals/TRUST_AWARE_PTA.md)). The extension
touches `ftracker.c`, `fxn.c`, `params.c`, `output.c`, `global.h`, `extern.h`
and the Makefile, so their hashes in
`data/provenance/simulator_source_sha256.csv` now describe the extended tree.
The paper campaign `ftracker.c` value above is unchanged as a historical
record. The last commit whose simulator source matches the campaign byte for
byte is `b5b0e1d` (“Align folders with the paper experiments”).

Every new parameter is off by default and the extension never draws from the
agent random stream. Before the extension was committed, all 27 method
configurations, with and without agent removal, were run with the campaign
binary and the extended binary, and the final statistics matched exactly.
`tests/test_report_feedback.py` keeps checking that report mode with honest,
noise-free peers reproduces shared-feedback runs exactly, and
`tests/test_campaign_reproduction.py` still checks the archived values.
