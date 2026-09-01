# D057 - `libScePosix` is checked, and it is the best provenance available without a console


Status: decided.

A separate library exports POSIX under a `posix_` prefix. obSCEne did not touch it at
all, which the emulator gap analysis surfaced (BACKLOG §10). Five checks now do.

**The expectations are settled by a document.** That is the point. Fifty-three checks in
this suite are `OBS_FROM_ASSUMED` and none is `OBS_FROM_HARDWARE`, and an emulator
implemented to satisfy an assumed check has only been made to agree with this project.
Both can be wrong together and the report will say otherwise. A spec check cannot fail
that way, because the authority sits outside both projects.

**It is also a second spelling of functions already checked.**
`scePthreadRwlockTryrdlock` and `posix_pthread_rwlock_tryrdlock` should be one
implementation behind two names, so `017-posix/spellings-agree` calls both and reports
when they differ. It is the only check here whose expectation comes from the platform
rather than from a document: it asks for self-consistency, not for a particular answer,
and a divergence is a fault whatever the right answer turns out to be. Marked `assumed`,
because no document says the two libraries must be one implementation.

**What is left out is everything needing a struct layout.** `posix_nanosleep` and
`posix_clock_gettime` take a `timespec`; `posix_mmap` and the `sys_*` socket family take
more. D008 applies unchanged.

The signal-set checks are the exception that shows the rule can be worked around rather
than broken. A `sigset_t` is opaque and differently sized per system, so the pointer is
declared `void *` - identical in the ABI - and the set is only ever read back through the
platform's own `ismember`. Nothing about the layout is assumed except that it fits in the
buffer, which is 128 bytes against a target that wants 16.

