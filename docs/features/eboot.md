# Full-Screen BIG_APP Mode

Executing conformance probes as a full retail `BIG_APP` in `/data/homebrew/`.

When probing RDNA2 GPU command buffers (PM4), video flip queues (`libSceVideoOut`), and DualSense controllers (`libScePad`), the probe must hold exclusive HDMI screen focus.

`make native` is the target that produces this: it builds the fake-signed `eboot.bin` (the same
one `make eboot`/`pkg` produce, generation defaulted for the native/BIG_APP flow) and lays it out
as a full PS5 title directory under `build/prospero/<TITLE_ID>/`, alongside `sce_sys/param.json`
and `sce_sys/icon0.png` - not the standalone `build/eboot.bin` that plain `make eboot` produces
for `pkg`. See `docs/ARTIFACTS.md`.

---

## How to Build & Run

```bash
# 1. Compile native title in WSL/Docker
cd obscene
make native

# 2. Upload title folder via Prosperous (or use ./bin/obscene native --deploy)
#    Default TITLE_ID is PPSA90000 unless TITLE_ID/CONTENT_ID is set (scripts/build-native.sh).
pros restore build/prospero/PPSA90000 /data/homebrew/PPSA90000

# 3. Launch title on target
pros launch PPSA90000
```

---

## What Renders on Screen
The console TV will display the **obSCEne HUD**, rendering:
- `GEN`: Detected hardware generation.
- `GPU`: Active graphics driver (`gnm` / `agc`).
- Live check execution counter and pass/fail summary.

