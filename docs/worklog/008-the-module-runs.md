# The module runs


obSCEne loads, resolves its imports, and executes guest code under shadPS4. It calls
platform functions by NID and the emulator dispatches them. That is the thing this
repository has been trying to do.

**What it took, in order.** Each of these was hiding the next, which is why the count
matters more than any one of them:

| # | Symptom | Cause |
|---|---------|-------|
| 1 | `Unable to find library and module` | Every symbol encoded `<nid>#A#A` - a library id nothing declared. No import tables at all. |
| 2 | `Attempting to add too many segments!` | Four `PT_LOAD` segments. Three is the limit (D029). |
| 3 | Fault in the loader, empty log | Identity tags emitted before `DT_SCE_STRTAB` (D030). |
| 4 | Guest calls address zero | Our own functions going through the PLT and being "resolved" by NID (D031). |
| 5 | Nothing readable | Every output channel stubbed (D032). |

Number 3 was the expensive one. It presents identically to every other malformed
module: a fault inside the loader, before any guest instruction, with nothing logged.
It was found by cutting the dynamic table down one entry at a time and running each
truncation - an empty table ran, one entry did not, and that entry was the module's
own name.

**Where it stands.** 774 imports resolve. The probe runs its checks, calling
`sceKernelWrite`, `scePadOpen`, `sceKernelGetProcessTime` and the rest, and the
emulator answers. It stops partway through on a bad call - a real defect, and now one
that can be debugged, because everything before it works.

**Nothing can be read yet.** This emulator stubs `sceKernelWrite`, `write` and
`putchar` alike, so all three channels return zero and the report is discarded. The
channel selection is behaving correctly; the platform genuinely has no way out. That
is itself a finding, and the reason the chosen channel is now in the `end` record.

**Surprises**

*The reference module was not needed after all.* Removing it looked like a loss. What
replaced it - running truncations of our own module and reading which ones survive -
found in one pass what comparing against a reference had not found in a day.

*Two emulators disagree about `-fno-plt`, and the disagreement is load-bearing.* With
it, one runs the probe and the other refuses the module. Without it, exactly the
reverse. Kept as it is because one of those is running.

*The correct `e_type` was wrong.* It was 0xFE10 on the reasoning that a reference
module which runs is 0xFE10, which was true and untestable with a single emulator. A
second loader refuses 0xFE10 outright and accepts 0xFE18, and the first does not care.
One emulator could not have settled this.

