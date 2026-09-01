# 2026-08-25 - fpPS4, and the handle that did not fit


The fpPS4 sweep was set running with a **420-second timeout on something that hangs in
seconds** - 7 minutes plus a 14-minute confirmation retry per hang, against 44 exclusions.
Roughly fifteen hours if it had to find them all. The user stopped it, and was right to; a
screenshot of the window frozen at check 6 of 514 settled in one look what the log had been
saying all along.

Three changes: timeout 420 → 60, kill the stray guest (a hung window does not die with the
script), and **recover the exclusion list instead of rebuilding it**. `reports/fpps4.txt` still
records all 44 as `excluded at build time`, so that walk was already done on the 24th.
Extracted, handed to `--resume`, done in minutes.

It ran to the end: **447 pass, 8 partial, 5 fail, 54 skip**, and **zero new exclusions** - the
44 sufficed and today's twenty new checks introduced no new hangs.

### Four-way consensus, and D166 confirmed

`015-sync/event-flag-round-trip` is **gone from the outliers**: all four now agree, which is
the fix landing. `barrier-of-one-releases` too. Twenty behavioural outliers remain, most of
them the host being a PC.

### One of them was a check of ours, and the diagnosis was wrong (D171)

`018-relational/semaphore-state-is-per-object` failed on fpPS4 alone with *"a fresh semaphore
refused a signal"* and EINVAL. The signal was fine. fpPS4's `sem_enter` returns EINVAL for a
null handle, and the handle was null because creating the **second** semaphore had overwritten
the first.

| | out-parameter |
|---|---|
| shadPS4 | `SlotId` wrapping a `u32` |
| PS5PCEM | `?*u32` |
| fpPS4 | pointer to a struct - eight bytes |

Two against one, and `platform.h` follows the two, so this is an overrun. Whether the adjacent
`int`s sat in the order that exposed it was the compiler's choice - the single-semaphore check
on the same platform passes.

**A corrupted handle fails at whatever touches it next, and that is never where the fault is.**
It took reading three implementations to get from the symptom to the cause. So the question is
asked directly now: `018-relational/handle-fits-its-out-parameter` puts a guard word after the
destination and reports what landed there (`0x0` on fpPS4 - the upper half of a heap pointer
below 4 GiB). And the relation check checks its guards first, skipping with a pointer to the
width check rather than blaming per-object state for a width problem.

This is the mirror of the bug orbistoun found on their own side and fixed on the strength of
our signature. Sent, with the 2-of-3 count attached, because a third implementation doing the
wide thing is what reopens a settled decision later.

