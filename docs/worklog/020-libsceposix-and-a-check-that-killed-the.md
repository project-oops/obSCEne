# libScePosix, and a check that killed the process on the host


The gap analysis said obSCEne did not touch `libScePosix` at all - a whole library
exporting POSIX under a `posix_` prefix, 64 symbols, none of them named here. It is the
best provenance available without a console, because POSIX settles what these do: a check
written against them is answerable from a document rather than from this project's own
reasoning (D057).

That matters more than coverage. Fifty-three checks here are `assumed` and none is
`hardware`, and an emulator implemented to satisfy an assumed check has only been made to
agree with us - both wrong together, with a green report saying otherwise. The authority
for a spec check sits outside both projects, which is the only thing that breaks the
circle before hardware does.

Five checks in `017-posix`: page size, signal sets, a short sleep, the try-form read/write
lock, and the one this section exists for.

### The comparison check

`scePthreadRwlockTryrdlock` and `posix_pthread_rwlock_tryrdlock` should be one
implementation behind two names. `017-posix/spellings-agree` calls both and reports when
they differ.

It is the only check in the suite whose expectation comes from the platform rather than
from a document. It asks for self-consistency, not for a particular answer - a divergence
is a fault whatever the right answer turns out to be, and each path passes its own checks
while it happens. Marked `assumed`, because no document says the two libraries must be one
implementation.

### It segfaulted, and everything caught it correctly

First run on the host: segmentation fault, process gone. The `try` record with no `res`
named the check exactly (D058).

The harness skips a check whose declared symbol is null, because jumping to zero ends the
run. That guard covers **the one symbol in the check's table row** and nothing else the
body reaches for. This check calls two libraries by design, and on the host the vendor
spelling has no stub - so `scePthreadRwlockInit` was null and the check walked into it.

Two things worked exactly as designed and are worth naming.

**The announce-before-attempting invariant identified it precisely** rather than saying
"somewhere in this run". Third time it has paid for itself.

**It happened on the host build.** A null vendor symbol is normal on an ordinary machine
and unusual under an emulator, so the host met it first - which is the entire argument for
having a host build. Under an emulator this would have surfaced as an intermittent crash
in a section that usually works.

The rule is now in CLAUDE.md rather than in anyone's memory: a check that reaches outside
its own table row is responsible for the addresses it reaches for.

### Two smaller corrections

The section was numbered `030-posix`, which both collided with `030-thread` and sat out of
order after `015-sync`. The report's own self-check caught it - sections must ascend, and
it listed them to prove they did not. Renumbered `017-posix`, which is also where it
belongs: after the vendor locks it compares against. Free to rename because nothing had
ever shipped with the old identifier.

A comment claimed `posix_usleep` was the only deliberately-blocking call in the suite.
`050-time/usleep` already does the same thing. Corrected, and the note now points out that
`sceKernelUsleep` is the vendor spelling of the same call - another comparison candidate,
deliberately not taken, because two sleeps both returning zero would agree without either
having slept.

### The rest of the library is censused

Fifty-one symbols. Every one takes a `timespec`, a `sigaction`, a `sockaddr` or an `mmap`
argument, and a wrong layout produces a call that succeeds and does the wrong thing (D008).
Presence is the honest claim, and not a small one: this library is a second implementation
path onto the same kernel, so which half of it an emulator has bothered with is worth
knowing.

The signal-set checks show the layout rule can be worked around rather than broken. A
`sigset_t` is opaque and differently sized per system, so the pointer is declared `void *`
- identical in the ABI, since every pointer passes the same way - and the set is only ever
read back through the platform's own `ismember`. Nothing about the layout is assumed except
that it fits in a buffer sized 128 bytes against a target wanting 16.

Census is now 311 symbols across 15 libraries.

### The comparison check found something on its first run

Under shadPS4 0.18.0:

```
015-sync/rwlock            pass    the vendor spelling refuses a writer while readers hold it
017-posix/rwlock           fail    the POSIX spelling lets one in
017-posix/spellings-agree  fail    the two spellings disagree about admitting a writer
```

Two entry points onto what should be one implementation, disagreeing about the one
property a read/write lock exists to have. The vendor path passes its own check in
isolation and would have gone on passing it; nothing else in this program could have seen
this.

That is the argument for the section, made on the first run rather than in theory.

The rest of the section reads as stubs, and the responsiveness reasoning applies:
`posix_getpagesize` returns 0, and `posix_sigaddset` reports success and does not add the
signal. Both are "not implemented" rather than "implemented wrongly".

`posix_usleep` passes, and **a stub would pass it too** - which is worth stating because
it is the same defect D056 found elsewhere. It is documented in the check rather than
fixed, because both fixes available are worse: a responsiveness probe would compare two
return values that are both zero when the function is *correct*, and a clock comparison
would need a time source counting something other than process time, which a sleeping
thread may legitimately not accrue. A pass there means "accepted a valid request and
reported success", and the value is in the fail.

### State

639 records, complete. 62 pass, 5 partial, 38 fail, 7 skip. 313 census symbols, every one
present - including all 51 of `libScePosix`, which shadPS4 resolves in full while
implementing almost none of it. The gap between those two numbers is the whole reason the
census and the checks are separate things.

**A note on process.** The sweep that produced these numbers was started while files were
still being edited, and that was flagged as untrustworthy at the time. A clean run
afterwards reproduced 639 records and the same three rounds exactly. The caution was
right and the result happened to survive it.

---

