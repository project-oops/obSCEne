# D196 - fpPS4 does not crash on an unimplemented import - it sleeps forever, and that is one line


The question was whether fpPS4's stalling was ours or theirs. It is theirs, it is deliberate,
and the code says so plainly:

```pascal
procedure print_stub(nid:QWORD;lib:PLIBRARY); MS_ABI_Default;
begin
 Writeln(StdErr,SysLogPrefix,'nop nid:',lib^.strName,...);
 //DebugBreak;
 Sleep(INFINITE);
 //readln;
end;
```

Every unresolved function import is bound to this (`Stub.NewNopStub(...,@print_stub)`). The
commented-out `DebugBreak` and `readln` beside it show the intent: freeze the process so a
developer can attach and identify the missing function. Reasonable for that workflow. For an
unattended sweep it means one missing function costs the whole run **silently** - the process
stays alive and stops producing output, which is indistinguishable from a hang in the guest,
and is exactly what "fpPS4 seems stuck on 9/27" was.

Directly above it sits `_nop_stub` - `xor %rax,%rax; ret` - the returning alternative, unused.
The compiler says so on every build: *"Local proc `_nop_stub` is not used"*.

### Nothing on our side could have avoided it

obSCEne declared `strspn`, imported it, and announced it before calling. A call that never
returns cannot be detected by its caller; announce-before-attempting **is** the mitigation, and
it worked - the dangling `try` named `strspn` exactly. There is no defensive change available
to the probe, which is worth stating because "could we be more careful" was the open question.

### The patch, and why it returns a value rather than nothing

`patches/fpps4-probe-friendly.patch`. `print_stub` becomes a function returning `QWORD` zero.

Not a bare procedure, because the trampoline is `push rdx; push rcx; ...; call rax; pop; pop;
ret` - it hands the guest whatever the handler left in `rax`. Undefined there is not harmless:
a pointer-returning function given garbage crashes at the first dereference, far from the
cause. This is the same mistake that crashed Kyty at entry when its trampoline returned
`0x7FFFFFFF`.

### What it bought

| | stock | patched |
|---|---|---|
| records | 36 | **36,631** |
| outcome | hung in `007-responsive/libc` | **complete, 27/27 sections, 515/515 checks** |
| wall clock | ran until killed | **2s** |
| unimplemented imports survived | 0 | **181** |

181 is the number that settles "is fpPS4 worth continuing with". At two runs per blocker under
D191, converging the stock build would have taken roughly **362 runs**. Patched, it takes one,
and fpPS4 is now the fastest loader in the sweep.

It also makes the measurement *correct* rather than merely available: a stub that returns is
reported by `007-responsive` as `silent`, which is the true answer - the same verdict prosper's
registry independently confirmed 54 times over. (D195)

### Where the row goes

`reports/fpps4-patched.txt`, not the stock row. The behaviour was changed, so the result is
about a build only this machine has. (D176)

Status: **derived** - read from their source, measured against both builds.

