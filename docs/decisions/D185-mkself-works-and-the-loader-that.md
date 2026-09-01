# D185 - `mkself` works, and the loader that rejected it first is why


The container is built and a real loader takes it: shadPS4 accepts the magic, reads the entry
table, extracts the module, resolves its imports and runs the suite to its end record -
**36,257 records, tally 454/7/37/17**, the same shape as the same module run as a plain ELF.
The eboot chain is proven rather than asserted.

It did not work first time, and the failure is the useful part.

### A rejection said more than the table it came from

The first container was accepted as far as `LoadSegment`, then died on `UNREACHABLE()`.
shadPS4's SELF path does not walk entries in order; it looks for one whose **blocked** bit is
set and reads bits 20..31 as a program header index:

```cpp
if (seg.IsBlocked()) {
    auto phdr_id = seg.GetId();
    ...
}
UNREACHABLE();
```

Every entry had `props = 0`, so nothing was blocked, nothing matched, and it fell off the end.

**Both fields were already in `data/self-format.tsv`** - `segment_flag/blocked` and
`segment_flag/id`, recorded from the readers on the first pass. The table was right and the
builder wrote zeros into the field the table described. Having the fact written down is not
the same as using it, and nothing in the pipeline could have caught that: the container was
structurally valid, the sizes all computed, and it was wrong.

`mkmodule`'s header comment says *every fixup here came from a rejection, not a
specification*. This is the same sentence one layer out, and it arrived within an hour of the
table being written.

### What the rejection also settled

Cross-referencing the reader against the writer showed the two are describing one thing under
two vocabularies:

| shadPS4 reads | OpenOrbis writes | bit |
|---|---|---|
| `IsBlocked()` | `has_blocks` | 11 |
| `GetId()` mask `0xFFF` | `segment_index` mask `0xFFFF` | 20 |
| `IsOrdered/Encrypted/Signed/Compressed` | `order/encrypted/signed/compressed` | 0,1,2,3 |

So `segment_flag` in the table and `entry_prop` are the same field. That is recorded in the
rows rather than merged, because the two masks genuinely differ and neither can be shown wrong
from here.

One constant needed care: block size is stored as `ilog2(bytes) - 12`, so 16KiB is written as
**2**. Storing the byte count fits the four-bit field for small values and truncates for real
ones - a container a loader would accept and then read from the wrong offset.

### What this does and does not prove

It proves the container is well-formed enough for a **previous-generation** loader, which is
what the format table describes. It says nothing yet about the current console, and the whole
point of the shape is to reach that one. A build from this table remains a hypothesis until
something current accepts or rejects it - and a rejection will be worth as much as this one
was.

Status: **decided**. `eboot` is a real target, its CI job no longer tolerates failure, and the
job checks the emitted header against the constants the table names.

