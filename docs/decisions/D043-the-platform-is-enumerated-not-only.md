# D043 - The platform is enumerated, not only asked about


Status: decided.

Every other section tests a list this program carries: someone wrote a name down and a
check asks whether it works. That is only ever as complete as whoever maintained the
list - and when an emulator's release notes were checked against the suite, two of the
functions it had just implemented were not in the census at all. Not broken, not
reported absent: never asked about.

`110-modules` asks the machine instead. `sceKernelGetModuleList` deals only in handles,
so it needs no struct knowledge and is safe outright; `sceKernelGetModuleInfo` is given
a large zeroed buffer with its leading size field set, and only the name at the front is
read back.

**On hardware this is the inventory.** Against an emulator it is what that emulator
claims to provide, and the difference between the two is the gap. That makes it the
foundation for coverage: a name missing from the inventory is a different problem from a
name present and broken, and until now this program could not tell them apart.

**A guessed offset reports itself as a guess.** When the assumed layout finds no name,
the section reports the front of the structure word by word rather than trying other
offsets until one produces something printable - a guess that lands on printable bytes
reads as a name and is not one. The layout gets derived from what a platform actually
wrote, which is the same argument that fixed the dynamic tags (D036).

First result, against an emulator: one module reported loaded, and `EINVAL` when asked
to describe it. Found without a user report and without reading anyone's source.

