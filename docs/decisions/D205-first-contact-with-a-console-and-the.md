# D205 - First contact with a console, and the answer arrived in ninety seconds: elfldr does not resolve our imports


A real PS5 on firmware 12.40, 2026-08-27. `obscene-min.elf` - 3,848 bytes, the transport-proving
build the Makefile calls *"the one to reach for first on real hardware"* - sent through
`obscene-tool hw send`, which routes to `pros_link` and elfldr on :9021.

The send succeeded and nothing came back. `/data/obscene-report.txt` was never created, and
`/data` is `drwxrwxrwx`, so permissions were not it. Without a kernel log that is where the
investigation would have stopped, with three indistinguishable explanations and no way to
choose between them. klogsrv chose:

```text
# backtrace:
# 0000000000000000
mDBG: Sending signal(pid: 614, tid: 101304, signo: 0xb)
[Syscore App] App Crash : PID=0x266, reason=0xb
```

**Backtrace zero, signal 11.** The payload ran, got a pid, and jumped to address zero.

### The disassembly had already said so

```asm
1468: mov 0x1221(%rip),%rax   # 2690 <sceKernelWrite>
146f: test %rax,%rax
1472: jne  148b               ; resolved - carry on
1478: jmp  147d               ; null - fall through
147d: movq $0x0,-0x8(%rbp)
1489: call *%rax              ; call 0
```

`min.c` guards a weak symbol by testing it, and on the null branch the compiler emits a call
through a zeroed slot. The GOT entry for `sceKernelWrite` was still zero at entry, so **elfldr
loaded and ran the payload without resolving a single import**.

That is a property of the plain-ELF path, not of this payload: the full `obscene.elf` uses the
same weak-symbol mechanism and would fault identically. **There is no point sending it until
imports resolve** - which is the first thing this trip established and the whole reason for
sending 3,848 bytes before 9,246,520.

### What else the log gave away, unasked

Real module load addresses on 12.40, with fingerprints:

| module | text range |
|---|---|
| `/system/common/lib/libkernel_sys.sprx` | `0x800000000`-`0x80004c000`, 4 segments |
| `/system/common/lib/libSceLibcInternal.sprx` | `0x8000a8000`-`0x8001d8000`, 4 segments |
| `/system/common/lib/libSceSysmodule.sprx` | `0x800274000`-`0x800280000`, 4 segments |
| host app `/system/vsh/app/NPXS40112/eboot.bin` | `0x400000`-`0x454000`, 4 segments |

**These are the project's first `hardware` observations of anything.** Every number in every
report before today was `assumed`, `spec`, `derived` or `implementations`.

### Crashing is cheap, which is the finding that matters most

The console took a full coredump, terminated the process, and **every service was still up
afterwards**: elfldr, ftpsrv, klogsrv, shsrv, pldmgr all answering, immediately re-sendable. No
reboot, no re-jailbreak.

That was the open risk in the whole approach - D184 cost a reboot once, and the resume
mechanism only pays off if a crash costs one run rather than the session. **On this firmware,
through this loader, a faulting payload is survivable and repeatable.** The loop works here.

Status: **hardware** - observed on a console, which is the first time that word has been
usable in this repository.

