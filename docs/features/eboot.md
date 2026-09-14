# Full-Screen BIG_APP Mode

Executing conformance probes as a full retail `BIG_APP` in `/data/homebrew/`.

When probing RDNA2 GPU command buffers (PM4), video flip queues (`libSceVideoOut`), and DualSense controllers (`libScePad`), the probe must hold exclusive HDMI screen focus.

---

## How to Build & Run

```bash
# 1. Compile native title in WSL/Docker
cd obscene
make native

# 2. Upload title folder via Prosperous (or use ./bin/obscene native --deploy)
pros restore build/prospero/PROO00001 /data/homebrew/PROO00001

# 3. Launch title on target
pros launch PROO00001
```

---

## What Renders on Screen
The console TV will display the **obSCEne HUD**, rendering:
- `GEN`: Detected hardware generation.
- `GPU`: Active graphics driver (`gnm` / `agc`).
- Live check execution counter and pass/fail summary.

