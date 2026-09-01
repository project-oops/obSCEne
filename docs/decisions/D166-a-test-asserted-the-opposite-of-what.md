# D166 - A test asserted the opposite of what `sceKernelClearEventFlag` does, and the stub agreed


**`015-sync/event-flag-round-trip` asserted the opposite of what `sceKernelClearEventFlag`
does, and the host build agreed with it because the host stub had been written to the same
misreading.**

Two implementations failed it with the same message - *"a cleared bit still polls as set"* -
while the host alone passed. They were right.

```cpp
// shadPS4, event_flag.cpp
void Clear(u64 bits) { m_bits &= bits; }
```

```zig
// PS5PCEM, kernel_runtime.zig
// The PS5 ABI supplies the bits to retain, not the bits to remove.
object.bits &= mask;
```

**The argument is a mask of what to keep.** `clear(BIT_A)` retains BIT_A and clears everything
else, so a bit that was set and then "cleared" is correctly still set. The check called that a
failure.

### Why the host build did not catch it

Because the host stub was written the same day, to the same misreading, and its comment
*named the correct semantics before implementing the opposite*:

> The vendor call clears the bits **not** named - it is a mask of what to keep […] Here it
> does the reading a caller would expect from the name, because a host stub is a known-good
> implementation of the *obvious* semantics.

That reasoning is backwards. Principle 5 says a check that has not passed a known-good
implementation is not evidence - and its force comes entirely from the implementation being
*correct*. A known-good implementation of the **wrong contract** is worse than none: it
manufactures exactly the confidence the rule exists to supply. The check passed on the host,
looked validated, and was wrong.

Fixed to `&= bits`, and all three now agree.

### The check now distinguishes the two readings, which it could not before

Clearing a single bit cannot tell them apart. Set A, clear A: under keep-mask semantics A
survives; under clear-mask semantics A goes. **One bit changes state either way**, so a check
watching one bit sees a plausible answer whichever contract is true. That is how this survived.

So the sequence sets **two** bits and keeps one: A stays, B goes. Then an empty retain mask
clears everything, which is the strongest single statement of what the argument means and is
nonsense under the other reading.

### The provenance was the highest in the suite, and it was the wrong one

This was the **only** check of 146 carrying `OBS_FROM_DOCUMENTED` - "vendor interface
documentation describes this behaviour specifically". The one check claiming a document
described it had it backwards.

Downgraded to `OBS_FROM_ASSUMED`, and that is not a comfortable fit either. What supports it
now is two independent open-source implementations agreeing, one commenting explicitly on the
convention. **The ladder has no rung for that**, and it is the same gap the sibling project
raised about `measured` - a value that covers "somebody actually established this, just not
from the authority the grade names". We declined to add one for measurement on the argument
that the origin field carries it as data; there is no equivalent escape hatch on the
*documentation* axis, and this is the second time that has cost something. Worth reopening
rather than settling here.

ASSUMED understates the evidence. DOCUMENTED overstates it by claiming a citation nobody here
can produce. Overstating is what went wrong, so it understates, with both sources named in the
comment.

### What actually found it

Three-way consensus, and it only became legible today. The host stubs for event-flag
set/clear/poll were added this morning (D-relational-host-stubs work); before that the host
**skipped** this check, so consensus saw two opinions rather than three and had no majority to
report. Adding a third implementation turned an unreadable comparison into `host alone says
pass`.

That is the substitute-oracle argument working exactly as BACKLOG §12 describes it - with the
uncomfortable detail that the outlier was us.

