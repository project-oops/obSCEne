# Hardware day: capturing a real RDNA2 GPU corpus

The GPU probe was built to run on a Steam Deck (RDNA2, gfx1033) and diff what real silicon
computes against the reference and against emulation. Everything below the capture step works
today against llvmpipe; this is the recipe for the day a Deck (or, later, a PS5) is in hand.

## Why the Deck needs no platform GPU API

The Deck is x86-64 Linux with an RDNA2 GPU and **no vendor libraries**. obSCEne's GPU backend
there is ordinary public Vulkan (`gpu_vulkan.c`, the same one llvmpipe uses in the build VM) - so
the Deck is reached with the host build, not the hardware module, and needs nothing from `sceGnm`.
That is the whole reason the Deck is the near-term target: the machinery that runs on it is
already built and proven on llvmpipe.

## 1. Build

```bash
make deck BUILD=/tmp/obs
```

Produces `/tmp/obs/obscene-deck` - the `GPU=1` host build, renamed. Copy that one file to the
Deck (scp, a USB stick, whatever reaches it); it is freestanding apart from `libvulkan`, which
the Deck has.

## 2. Run and capture

On the Deck, either run it and keep stdout:

```
./obscene-deck > deck-report.txt
```

or serve it and drive from the dev machine (the Deck has no keyboard convenient for this):

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

The `gpudev` line will name the real device (`... gfx1033 ... integrated`), which is how every
tool below knows this is silicon and not the `cpu` llvmpipe or the `reference` oracle.

## 3. Analyse

One command produces the reference for the corpus's inputs, the exact-match diff, and the ULP
approximation ranking:

```
scripts/gpu-analyze.sh deck-corpus.txt
```

Read it as three answers:

- **exact ops** must be empty in the diff - `floor`, `divf`, `fma`, the bit ops. A divergence
  there is a device bug, on RDNA2 as much as anywhere.
- **transcendentals** appear in the diff and in the ULP ranking. The ranking is the finding: how
  far RDNA2's `rcp`, `sin`, `rsq` sit from correctly-rounded, worst first. This is the RDNA2
  approximation map - the thing that was theoretical until the Deck produced it.

## 4. Diff against emulation

To find where an emulator disagrees with the Deck - the shader gap to close - keep both corpora
and diff them directly:

```
obscene-tool gpudiff deck-corpus.txt emulator-corpus.txt   # exact divergences, by operand
obscene-tool gpustats deck-corpus.txt <(obscene-tool gpuref deck-corpus.txt)  # RDNA2's own error
```

`gpudiff` naming the operands where the two disagree is the actionable output: each is a lane an
emulator computes differently from the hardware, which is exactly a shader-recompiler gap.

## Regression, meanwhile

`reports/gpu-golden.txt` is the blessed llvmpipe snapshot and `scripts/gpu-golden.sh --check`
(run by `verify.sh`) guards it. That golden is llvmpipe's, not the Deck's - the check skips on a
different device rather than failing. A Deck golden could be blessed with `--capture` on the
Deck, but only once one exists to bless.
