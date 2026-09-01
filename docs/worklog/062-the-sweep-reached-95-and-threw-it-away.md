# 2026-08-24 - the sweep reached 95% and threw it away over one bad run


The prober with a proper round budget got to **index 351 in 31 rounds** - far better
than the one-fault-in-five the dense early region suggested, because the faults cluster at
the front. 321 answered calls:

| outcome | count |
|---|---|
| `zero` | 220 |
| `error-shaped` | 52 |
| `rejected` | 46 |
| `value` | 3 |

Then it exited on `the run produced no bulk records and no unfinished check; it did not
start`, discarding an hour of accumulation.

**Re-running that exact index by hand produced 916 records immediately.** The round was a bad
run - shadPS4 crashing before it opened a window - and nothing else. The script enumerated
two causes for an empty round when it was written, and the third is the common one: the run
just fails. Worse, the message names the *module*, so the natural next move was to go hunting
for a fault in a build that did not have one.

Retried once now, which is exactly the discipline `sweep.sh` already applies to a suspected
hang (D144): a single observation of nothing is not evidence of nothing. Only a second empty
round at the same index concludes, the flag resets whenever a round produces records, and the
summary reports how many retries were needed - six of them describes an unstable loader,
which is a finding about the loader and should not be silent. (D162)

`--resume` then did what it was built for on its first real use: *resuming at index 351,
keeping 672 records*. No hand-concatenation, no restart.

### The error returns are a facility scheme, and we had told orbistoun otherwise

321 answered calls from the blind prober, and the error returns are not one convention. They
are `0x8` + a **16-bit facility** + a facility-local code, with the facility tracking the
library: `0x8026` libSceAudioOut, `0x8029` libSceVideoOut, `0x8041` libSceNet, `0x8055`
libSceNpTrophy, `0x805a` libSceSysmodule, `0x8092` and `0x809b` libScePad, `0x8093`
libSceAjm, `0x8096` libSceUserService, `0x809f` libSceSaveData, `0x80a1` libSceSystemService,
`0x80d1` libSceGnmDriver.

**Only `0x8002` carries errno in its low half**, and every code we saw with that facility is
one - ESRCH, EBADF, EACCES, EFAULT, EINVAL. Decoding any other facility as errno produces a
plausible wrong answer, which is the worst kind.

We sent orbistoun "`0x8002_0000 | errno` is the vendor scheme" and they recorded it as an
assumption on their hardware worklist. It is one facility of thirteen. Corrected on the
bridge, with two riders: **libkernel is not one facility** (`sceKernelGetProcParam` returns
`0x8076c000`), and `0xffffffff` is a plain `-1` from six Gnm entry points and `fclose` - our
own classifier calls it `error-shaped` on the high bit, conflating it with a facility-coded
error.

### And a correction to ourselves, of the same class

This sweep was recorded above as "index 351 of 371 - 95% of the census". **The prober walks
the corpus as well as the curated census**: 32,466 symbols in all (D114), against `surface.h`'s
371. It was at **1.1%**.

Two counts, both correct, both called "the census" in different parts of this repository, and
only one of them is ever printed. The prober reports a bare index with nothing beside it, so
there was no denominator on screen to disagree with the one already in mind. `910-bulk`
should report its own size, and D163 is the reason.

The corrected picture is actually encouraging. The dense region is at the front - 31 rounds
to cross the first 351 indices, then **one round covering 367 to 914**. Faults cluster in
libkernel and the pthread family, which is where a loader implements most and so has most to
get wrong. Rounds-per-index is not a constant and a budget cannot be estimated from an
average.

### The size record earned itself immediately

D163 said `910-bulk` should report its own list length. It does now, as a `measure` record
with quantity `list-length`, summed from the group table at run time rather than written as a
constant.

**It caught the second wrong denominator within a minute of existing.** Writing D163 up, the
figure was recomputed by hand - summing the `N callable` comments in `corpus.h` - and came to
32,095. That went into the decision entry, this worklog, `bulk-sweep.sh` and a message to
orbistoun. The first run of the new record printed `list-length 0x7ed2`: **32,466**.

The difference is 371. The hand count summed the corpus and forgot that the same loop also
walks the curated census - which is the exact conflation D163 exists to document, committed a
second time while documenting the first.

A denominator worked out by hand from the sources can be wrong in the same way twice. One
taken from the table the loop iterates cannot. Corrected in all four places.

### Where the sweep got to

| | |
|---|---|
| index reached | 25,602 of 32,466 (**79%**) |
| answered calls | 25,534 |
| did not return | 39 |
| blocked ahead of the section | 1 (`015-sync/condvar-wakes-a-waiter`) |
| empty runs retried | 0 |

Resumed again from 25,603 rather than restarted, which is the second real use of `--resume`
and the first where the alternative would have been an hour.

### D162 earned itself in one run, like the size record

The transient retry fired once, at index 25,710. The sweep then ran another 35 rounds and
reached 30,104.

So without it that run would have ended at **79%** instead of **92.7%**, and it would have
reported `it did not start` - words that send the reader to look for a fault in the module.
Two mechanisms added this session, both of which caught something real within a single run of
existing. That is not luck; it is what the absence of a check looks like from the outside,
which is nothing at all.

