# D089 - `150-memory-map`: the map walked, with the walk's own hypothesis checked as it goes


Status: decided, closing `docs/HARDWARE-PROBE.md` item 2 - which that document calls "the
single thing blocking a commercial title".

### The honest problem

Walking an enumeration means knowing where the next offset lives in the structure the query
returns, and that is a layout - which D008 forbids assuming. A wrong guess here does not
fail loudly. It loops forever on one region, or walks into nonsense while producing
plausible output, which is the worst failure available for a record whose purpose is to be
believed later by somebody who cannot check it.

### So the walk states its hypothesis and self-checks

- The hypothesis is that the **second** sixty-four-bit value is where the next query starts.
- **Every step must advance.** A value at or below the previous one stops the walk and is
  reported as `stalled`.
- The **raw bytes of every region are dumped anyway**, so the authoritative record survives
  a wrong hypothesis.
- The fields are `first` and `second`, named for their position. Never `start` and `end` -
  a reader who later learns the real layout can reinterpret a position; one who trusts a
  field called `end` cannot.

A wrong hypothesis therefore yields one region and a `partial` saying the walk stopped,
rather than sixty-four confident wrong lines.

### The two captures earned themselves on the first run

The document asks for the map at entry and again after an allocation, because the *shape of
a change* says what a snapshot cannot. Under shadPS4:

| | at entry | after allocating `0x4000` |
|---|---|---|
| region 0 | `0x0` → `0x10000` | `0x0` → `0x10000` |
| region 1 | `0x10000` → `0x7fc000` | `0x10000` → `0x800000` |

The second region's boundary moved by **exactly the allocation size**. That is the change
the second capture exists to show - and it independently corroborates the hypothesis, since
a field that moves by precisely the amount allocated is the field we guessed it was. The
walk did not need to assume that to work; it earned it.

Both walks terminated on a refusal rather than on the guard, and the refusal code went into
the error table: `0x8002000d`, which is `13`/`EACCES` under the scheme D088 found.

**The verdict claims nothing about what changed.** A check that interpreted the difference
would be inventing the layout the section spent its whole design avoiding.

