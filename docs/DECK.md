# Capturing a real RDNA2 GPU corpus on a Steam Deck

The GPU probe runs on a Steam Deck (RDNA2, gfx1033) and diffs what real silicon computes against
the reference and against emulation. This is the recipe for a corpus captured on a Deck.

## Why the Deck needs no platform GPU API

The Deck is x86-64 Linux with an RDNA2 GPU and no vendor libraries. obSCEne's GPU backend there
is ordinary public Vulkan, the same one llvmpipe uses in the build VM, so the Deck is reached
with the host build rather than the hardware module and needs nothing from `sceGnm`.

## 1. Build

```bash
make deck BUILD=/tmp/obs
```

`make deck` produces `/tmp/obs/obscene-deck`, the ordinary host build copied and renamed; there
is no `GPU=1` build flag. Copy that one file to the Deck.

## 2. Run and capture

On the Deck, either run it and keep stdout:

```
./obscene-deck > deck-report.txt
```

or serve it and drive from the dev machine:

```
# on the Deck
./obscene-deck --serve 9803
# on the dev machine, once, to pull the whole report over the socket
obscene-tool drive --address <deck-ip>:9803 --command 'hello|1' --command 'report' \
    --part target=deck --part gpu='AMD Custom GPU 0405 (gfx1033)' > deck-report.txt
```

Reduce the report to its GPU records - the corpus the analysis reads:

```bash
grep -E '^OBS\|(gpudev|gpu|gpuop)\|' deck-report.txt > deck-corpus.txt
```

The `gpudev` line names the real device (`... gfx1033 ... integrated`), which is how every tool
below knows this is silicon and not the `cpu` llvmpipe or the `reference` oracle.

## 3. Analyse

One command produces the reference for the corpus's inputs, the exact-match diff, and the ULP
approximation ranking:

```
scripts/gpu-analyze.sh deck-corpus.txt
```

Read it as three answers:

- **exact ops** are empty in the diff - `floor`, `divf`, `fma`, the bit ops. A divergence there
  is a device bug, on RDNA2 as much as anywhere.
- **transcendentals** appear in the diff and in the ULP ranking. The ranking is the finding: how
  far RDNA2's `rcp`, `sin`, `rsq` sit from correctly-rounded, worst first - the RDNA2
  approximation map.

## 4. Diff against emulation

To find where an emulator disagrees with the Deck - the shader gap to close - keep both corpora
and diff them directly:

```
obscene-tool gpudiff deck-corpus.txt emulator-corpus.txt   # exact divergences, by operand
obscene-tool gpustats deck-corpus.txt <(obscene-tool gpuref deck-corpus.txt)  # RDNA2's own error
```

`gpudiff` names the operands where the two disagree: each is a lane an emulator computes
differently from the hardware, which is a shader-recompiler gap.

## Regression

`reports/gpu-golden.txt` is the blessed llvmpipe snapshot (see `scripts/gpu-analyze.sh`). That golden is llvmpipe's, so the
check skips on a different device rather than failing. A Deck golden is blessed on the Deck once
one exists to bless.
