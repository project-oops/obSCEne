# D149 - A censused library is not a load-time dependency, and saying it is stops obSCEne loading on current shadPS4 at all


Status: defect, found by doing what D094 says to do.

shadPS4 built from its own source (`be21649`) refuses the corpus build outright:

```
LoadModule: Provided file /app0/sce_module/libc.prx does not exist
preloadModulesForLibkernel: Assertion Failed!
libc.prx cannot be loaded, but the guest attempted to use it.
```

The downloaded binary this project has been measuring with does not. Handed the *same
module*, it prints `Failed to preload libc, expect crashes` and carries on into the suite -
65,665 log lines against 33,044, and a run that reaches `040-file` instead of dying in the
loader. Current shadPS4 promoted that warning to an assertion.

**So every shadPS4 number in `COMPATIBILITY.md` was produced by a binary whose behaviour the
current source no longer has.** That is exactly the method error D094 names, and it stayed
invisible for as long as the source was only read and the binary only run.

### The cause is ours

A corpus-free module loads on the same emulator with no complaint at all - 284 records, no
assertion, not one missing-`.prx` line. The difference is `DT_NEEDED`.

`mkmodule` emits one per imported library, which was right when the imports were the curated
surface (D100 - fpPS4 keys its whole implementation table on `DT_NEEDED`, and without it
every symbol resolves to a stub). The corpus made it wrong: the module now names hundreds of
libraries, including `libSceFios2`, which obSCEne never calls and only ever asks about.

**A census asks "is this present?". That is not a dependency.** Declaring one for a library
you intend to probe is over-claiming in the module's own metadata - the same fault this
project refuses in a report - and a loader that takes the claim seriously is entitled to
demand the file. Current shadPS4 does exactly that, and firmwareless is the whole premise.

### Resolution: the declarations stay

The section above was written emulator-first, and reached the wrong conclusion from the right
evidence. It is kept because the evidence is the valuable part and because the reasoning is
worth being able to see.

**`DT_NEEDED` is not an over-claim.** It is the tag for declaring the platform libraries a
module works with, which is what a real title does, and on a console every one of those
modules exists - the firmware ships them, the loader loads them, and nothing fires. What
changed is shadPS4's tolerance for not having them, and a firmwareless loader meeting a
dependency it cannot satisfy is a gap in the loader's deployment, not a defect in the module.

The framing that produced the earlier conclusion had these emulators as the standard obSCEne
must satisfy. They are scaffolding: they exist to get this program running before hardware,
and the direction of travel is that obSCEne becomes the reference *they* are measured against.
Trimming the module to suit the least tolerant of them would be shaping the oracle to fit the
thing it is meant to outrank - and would reduce what obSCEne can reach on the hardware that is
the actual target.

So the declarations stay, and `COMPATIBILITY.md` records the shadPS4 gap as a firmware gap.

### What is still genuinely open, and it is not the emulator's half

A large set of hard dependencies is not obviously right for *hardware* either, for a reason
that has nothing to do with emulators: the corpus spans 23 firmware versions and no single
console carries every library in it, so a module declaring all of them could be refused by a
real loader for naming one the installed firmware does not have. Same failure, different
cause, and not yet measured.

The durable answer is probably that a census should not be a load-time declaration at all.
`sceKernelLoadStartModule` and `sceKernelDlsym` are already declared in `platform.h` and
`110-modules` already exists: asking the platform at runtime and reading the error code it
returns is strictly stronger evidence than a resolved address, which D140 established is
close to worthless on a loader that stubs whatever it cannot find. That is the shape of the
census obSCEne needs to be an oracle rather than a presence counter, and it is the work this
decision hands forward - not a `DT_NEEDED` trim.

