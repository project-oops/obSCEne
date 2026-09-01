# D114 - The blind prober covered 1% of the census, and pointing it at the rest immediately called a variable


Status: derived - both halves observed.

`910-bulk` walked `OBS_SURFACE_LIBRARIES` only: **383 targets against a census of 39,555**.
The one section that actually calls anything was covering one per cent of what the program
knows about, and the tables it needed were already generated in the right shape. Nothing
pointed at them.

### And the first thing the wider list did was crash

```
OBS|call|libSceImageUtil|__dso_handle|0x1608|attempt|0x0
```

No answer after it. `__dso_handle` is **data**, and calling data jumps into a variable.

The census never had to care - counting a data symbol is perfectly sensible - so nothing in
the corpus recorded which symbols were functions. The firmware descriptions do record it,
in a field `mine-nids.py` was discarding:

| type | count |
|---|---|
| absent, meaning Function | 361,999 |
| `Object` | 8,614 |
| `TLS` | 17 |
| `Unknown11` | 34 |

**5,048 of the corpus as ingested are data.** On an emulator that is a crash and a wasted
sweep round; on hardware it is a jump into a variable with the argument registers zeroed,
which is a worse thing to do to a console than any of the null-argument calls this section
was designed around.

### Two lists, not a field

The corpus now carries a fifth column, `fn` or `data`, and `gen-corpus.py` emits a
`CALLABLE` twin of every group. The census walks the full list; the prober walks the
narrow one.

Two lists rather than a kind on every row because the census macro shape is shared with the
hand-written `surface.h`, and widening it there to carry a field only one consumer reads is
the worse trade. Where sources disagree, **`data` wins**: calling a function recorded as
data costs a skipped probe, and calling data recorded as a function costs the run.

With the filter the prober steps past index 5,640 and reaches 6,467 - the same mechanism,
no longer aimed at variables.

