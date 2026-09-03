# Provenance

This repository is a curated copy. The original development tree and raw
research results remain unchanged in the private archive.

## Verified simulator source

The final Linux campaign records the following SHA 256 value for `ftracker.c`:

```text
86ba010cdf4fe7d097fdfd3328331bddf54841efa4cdc547156cf809c54d35f9
```

The file at `simulator/src/ftracker.c` has the same value. This establishes that
the public controller implementation is the one used for the final campaign.
`make verify` checks this contract automatically.

The complete campaign hash record is preserved in
`data/manifests/source_hashes.json`. Hashes for a Linux binary or generated
parameter template are historical records; they are not expected to match a
binary rebuilt on another operating system or the annotated portable default
configuration in this repository.

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
