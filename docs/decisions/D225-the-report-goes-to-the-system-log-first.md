# D225 - The report goes to the system log first, because a title has no descriptor


Every output channel `runtime.c` had writes to a descriptor: `sceKernelWrite`, `puts`,
`write`, `putchar`. A title launched by the system has no parent holding the other end of one.

`sceKernelWrite` to standard output does not fail in that situation - it returns the byte count
and the bytes go nowhere. That is **precisely the undetectable success** the comment on the
channel order already warned about for one emulator, arriving from a direction nobody had
looked: not a loader that lies, but a process with nobody listening. A full run would report
itself sent and be unreadable.

So `OBS_CHANNEL_DEBUG_OUT` goes first: `sceKernelDebugOutText`, which is not a descriptor and
which a homebrew klog reader already tails. It is the channel that made the minimal build's run
legible - `obscene-min: the guest is running` arrived through it and through nothing else - and
`start.c` already used it for boot breadcrumbs on the same reasoning. On an emulator it resolves
to null, the guard skips it, and the order that was already right for emulators is unchanged.

**It is the third local copy of one signature**, after `start.c` and `min.c`, and this project
says three copies of a judgement is how one of them goes wrong. It cannot go in `platform.h`
yet: the census in `corpus.h` declares the name as `const char` so that it can only be probed,
`bulk.c` and `surface.c` include both headers, and a function declaration there is a conflict in
those two translation units. Converging them is the documented five-step census move and is
worth doing; it is written down here rather than left to be noticed.

