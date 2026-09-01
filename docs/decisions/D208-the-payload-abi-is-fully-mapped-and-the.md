# D208 - The payload ABI is fully mapped, and the frontier is precise: word 0 is not a working resolver in the sandbox


Extending D206/D207 with a trustworthy readout: a **canonical** jump-tag (marker in bits 32-47,
data in 0-31) reports the fault address without the corruption the non-canonical tags of D206/D207
caused. Every value below was read that way, two sends per 64-bit word.

### The bootstrap is `payload_args`, confirmed by type

`rdi` at entry points at the ps5-payload-sdk / elfldr structure. The kernel pointers at words 3
and 4 clinch it - nothing else in a process holds two kernel addresses at fixed offsets:

| word | value | member |
|---|---|---|
| 0 | `0x8000005b0` | `sceKernelDlsym` - libkernel range, **execute-only** |
| 1 | `0x20000c100` | `rwpipe` |
| 2 | `0x20000c200` | `rwpair` |
| 3 | `0xfffff83074906640` | `kpipe_addr` (kernel) |
| 4 | `0xffffffffcd840000` | `kdata_base_addr` (kernel) |
| 5 | `0x20000c300` | `payloadout` |
| 6,7 | 0 | - |

`rsi`/`rdx`/`rsp` at entry are stack junk, so `rdi` is the only argument. D206's `0x4000ab` for
word 0 was a non-canonical-read artefact; the real value is `0x8000005b0`.

### Word 0 executes but does not resolve

`0x8000005b0` can be **called** (a jump there returns) but not **read** (`*0x8000005b0` faults
with `0xa0020328`) - execute-only protected memory. Called as a resolver it returns a small
value that **increments per process and ignores its name**: a real symbol and a garbage name
both returned it (`0x2f2` vs `0x2f6`, the delta being sends between them). That is `getpid`
behaviour, not `dlsym` behaviour.

An exhaustive hunt confirmed it: **96 combinations** - 24 handles (small ints, the 0x2001 band,
large module-handle-shaped values, the struct's own region pointers), each tried out-param and
direct-return, each with the plain name and the encoded NID `4wSze92BhLI` - and **not one**
produced an address in the libkernel band. `sceKernelWrite` is unresolvable from here.

### Why, and the frontier

The payload runs **sandboxed**: it cannot read libkernel (the `0xa0020328` fault) and its calls
into libkernel come back gated. elfldr provides exactly the primitives a sandbox escape needs -
`rwpipe`, `rwpair`, `kpipe_addr`, `kdata_base_addr` - which is why they are in the struct.
Established homebrew (BFpilot, CheatRunner) links the ps5-payload-sdk crt0, which performs that
escape before resolving anything.

**The remaining step is not brute-forceable and guessing it violates D008.** It needs one of two
citable, external things:

1. the ps5-payload-sdk / etaHEN elfldr crt0 (open source) - exactly how it invokes `dlsym` and
   whether it escapes first; or
2. the 12.40 kernel offsets a `ucred` patch needs, to escape using the provided primitives.

Reading the open-source loader is allowed with provenance, the way an emulator's source is.

### What holds regardless

obSCEne's payload must build with **16 KB pages** and take `rdi` as `payload_args`, resolving
through the escape-then-dlsym path rather than ELF relocations. `payload-min` and every hardware
payload need the page-size flags, which the module build already has and they do not.

### The console is unbreakable through this loader

Roughly forty payloads, every one faulting deliberately, and after each one all five services
answered and the next send went straight out. No reboot across the whole investigation.

Status: **hardware** - the full struct and the resolver's failure observed on the console,
2026-08-27.

