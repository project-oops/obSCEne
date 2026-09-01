# 2026-08-24 - the prober reached the end of the list


`reached the end of the list`, after four resumed sessions and 26 rounds on the final pass.

| | |
|---|---|
| entries | 32,466 |
| answered | **32,275** |
| `zero` | **31,111 - 96.4%** |
| `error-shaped` | 961 |
| `rejected` | 125 |
| `value` | 78 |
| never returned | 191 |

**282 of 342 libraries answered only zero** - 24,532 symbols, not one refusal among them. The
same loader's address census reports ~35,000 of ~35,000 present.

Every call passes nothing, so a function taking an out-pointer cannot succeed and one taking
a handle cannot operate. The correct answer for nearly all of them is a refusal, and refusing
is exactly what a generic stub cannot do. `COMPATIBILITY.md` argued presence-versus-behaviour
in prose from the day it was written; it now carries the measurement, with the caveats that
matter - one loader, no hardware, and a proportion rather than any individual record.

### The flag sweep answered orbistoun's first question, negatively and definitely

Four queries at offset 0 with `flags` 0, 1, 2, 4. All returned `0`, all wrote **byte-identical**
buffers, so no first-differing-byte record was emitted:

```
offset 0    00 00 00 00 00 00 00 00     start  = 0
offset 8    00 00 01 00 00 00 00 00     length = 0x10000
offset 16   03                          type   = 3
```

shadPS4 ignores the argument. That is worth having: their guest makes 87.6 million of these
calls, and anything inferred from agreeing with this emulator is inferred from a constant.

Two things noticed and not reconciled. `130-layout`'s own comment records "four experiments on
the emulator side went into establishing that it writes 24 bytes" - this writes **17**. Both
are emulator observations and neither is authority. And a 64 KiB length for the first extent
is not plausibly a console's.

### And a correction of framing rather than fact

The facility scheme was sent to orbistoun as a discovery. `OUTPUT.md` has described
`error-shaped` as "another facility, or an errno returned directly" since it was written, and
`classify()` has hardcoded `0x8002` as the privileged one just as long. **The two-tier
distinction was in our own vocabulary the whole time.** What the sweep produced that is new is
the mapping and the errno-correctness evidence, not the idea. Said so on the bridge, because
"they discovered a structure" and "they measured one their format already assumed" support
different confidence levels, and they are writing to a knowledge base off these entries.

