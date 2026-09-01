# D176 - A loader can be patched to let the probe run, and doing so measures the patch


Kyty ran the corrected module (D175), entered the guest, and died having written one record.
The question asked was whether obSCEne was at fault for calling too many unsupported
functions, whether it could be defensive instead, and whether patching Kyty locally was
reasonable. All three were tried. The answer to the first is no, to the second is mostly no,
and to the third is yes-with-a-caveat that turns out to be the whole finding.

### It was never "too many". It was one, before any check ran

obSCEne died at `puts` - the second channel of its own **output** chain, reached because
`sceKernelWrite(fd=1)` returns `EPERM` on Kyty. Not a check. Not the suite. The report writer.

Every platform declaration here is weak, so an unresolved import is a weak `Func` in the
jmprela table, and `relocate()` sends exactly that case to a JIT trampoline:

```cpp
value = reinterpret_cast<Jit::CallPlt*>(program->custom_call_plt_vaddr)->GetAddr(index);
```

Each trampoline pushes its relocation index and jumps to `RelocateHandler`, which prints a
stack trace and calls `EXIT`. **Calling one unimplemented function ends the process.**

### The patch is three bytes, and the reason it is clean is worth recording

The guest reaches a trampoline through an ordinary `call`, so `[rsp]` is already the guest's
return address. Replacing the body with `xor eax,eax; ret` returns a zero with correct stack
discipline and never enters the lazy-binding path at all.

Patching `RelocateHandler` instead would **not** have worked, and the reason is a good example
of why reading the mechanism beats editing the line that shows up in the error. `RelocateHandler`
is a PLT0 resolver: it is entered by `jmp`, after PLT0 and the PLT stub have pushed two words.
Deleting its `EXIT` and letting it return would have popped one word as though it were a return
address, leaving `rsp` two words out and `rax` undefined - a crash somewhere unrelated, with a
plausible-looking cause. The three-byte version cannot have that problem because it never
participates in the sequence.

### What two patches bought, and where it stops

| | records |
|---|---|
| stock | **1**, dies at `puts` |
| trampoline returns 0 | **14**, dies at `080-video/open` |
| + video-out user id relaxed | **102**, dies at `015-sync/mutexattr-round-trip` |

Then it stops being worth continuing, and the number that says so is this: Kyty contains
**1,970 `EXIT_NOT_IMPLEMENTED` sites and 207 `EXIT` calls**. Its design is to abort on anything
it does not handle, which is a defensible choice for running games - a title that reaches an
unimplemented function is not going to work, and failing loudly beats failing subtly. A probe
that deliberately sweeps input spaces meets those sites constantly.

Going further would mean disabling all 2,177, and the report from that build would be
worthless: every unhandled case continuing with default state is precisely the
stub-everything result `900-surface/control` exists to mark `(void)`.

### So it is a design mismatch, and neither side is wrong

This is the honest answer to the question. Kyty is not buggy for aborting and obSCEne is not
wrong for calling. **They want opposite things from an unimplemented function.** obSCEne cannot
be defensive about it in general either, because `obs_address_is_callable()` tests an address
and Kyty's trampoline *is* a valid address - there is no test that separates a stub from an
implementation, which is the argument for calling things and recording what came back.

### What the patched build was actually good for

Not measurement - bug-finding, and in about fifteen minutes it found three:

1. **`UserServiceGetInitialUser` returns `1`; `VideoOutOpen` refuses anything but `255` or `0`.**
   The two contradict each other inside one build, so any program doing the documented thing -
   ask for the initial user, open the display for that user - is killed. Of the two the
   returned `1` is the realistic half, so the check was the wrong one.
2. **`PthreadMutexattrSettype` calls `EXIT` on an unrecognised type.** POSIX says `EINVAL`.
   Terminating the process on a bad argument is not an implementation of that interface.
3. The unresolved-import behaviour above, which is a choice rather than a bug, but is worth
   stating as a compatibility fact about probes.

The third of those produced D177, which is a fact about the **platform** rather than about Kyty.

### The rule this sets

A patched loader may be run and its report kept, under two conditions: the patch is stored in
`patches/` so anyone can reproduce or refuse it, and its report never occupies the loader's row
in `COMPATIBILITY.md`. `reports/kyty.txt` holds the **stock** result (1 record) and
`reports/kyty-patched.txt` holds the other. A row labelled `kyty` that describes a build only
this machine has is the same lie as an invented constant, told at a larger scale.

Status: **assumed**.

