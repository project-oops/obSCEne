# The call/response tracer

A companion instrument to obSCEne, sharing its report format and its provenance discipline.
Where obSCEne is active - it chooses the inputs and tests failure deliberately - the tracer is
passive: it records what real titles call, in real sequences, including functions nobody has
named. The tracer finds the coordinates a real game visits; obSCEne maps the territory around
them.

## Injection

The tracer is a single self-contained payload. Its only hardware-side requirement is a
jailbroken console and a loader that runs one ELF - Payload Manager is enough; a resident daemon
is not required. It does its own attach, injection and hooking, and depends on no resident
service and no installed library.

The primitives come from reading two open GPLv3/GPLv2 projects (`<clones>/CheatRunner`,
`<clones>/etaHEN`) to understand the technique and reimplementing it in obSCEne's own
freestanding C. Their code is not linked and nothing they publish is installed. Each primitive
maps to C the tracer owns:

- **Inject code into a running game** - the `libelfldr` technique (`elfldr_exec(pid, ...)`):
  `ptrace(PT_ATTACH)`, a remote `pt_mmap` of `PROT_EXEC` pages inside the target, copy the ELF's
  `PT_LOAD` segments in, `pt_mprotect`, then redirect a thread (`r.r_rip = entry`; resume). About
  200 lines of C-to-C.
- **Read a running game's memory** - CheatRunner's `cr_mdbg.c` shows the kernel read/write path
  (`mdbg_copyout` / `copyin`; a CR3 page-table walk to the physical page). It is needed only
  out-of-process; the in-process path mostly does not.
- **NID to address, and the call-site detour** - once the payload executes inside the target,
  hooking is memory writes in its own address space. Walking the game's import tables
  (`DT_JMPREL` / `Elf64_Sym`) to find each function pointer is work `../selfish` already does; runtime NID resolve
  is the same hashing obSCEne already does; a `rel32` detour over a resolved slot is ten lines.

Reimplementing rather than linking keeps every repository's licence its own: linking GPL code
would make the linking binary GPL. obSCEne already parses the executable format and the import
tables (`../selfish`, D200), so the find-the-function-pointer half is code it owns.

## What the corpus holds

The mineable facts, ranked by what they unblock that nothing else can:

1. **Arity and argument shapes** - which registers hold non-garbage across thousands of calls.
   Turns D008's assumed arities into derived ones.
2. **The valid constant space** - every distinct flags or type value a real title passes is a
   known-good constant, unobtainable from static analysis.
3. **Struct layouts from real data** - out-param buffer before and after, diffed, on real
   structures with real values.
4. **Call sequences** - which functions must precede which; init orders; X before Y or Y returns
   Z. Hard to derive from a census, and exactly what breaks emulators.
5. **Errors in context** - the returned code and what the caller did next (retry, fall back,
   abort).
6. **Criticality ranking** - which NIDs every title calls, an evidence-based implementation order
   for emulator authors.

## The hot path is a volume problem

A title makes millions of calls a second. A complete trace fills storage in seconds and perturbs
timing enough to trip frame pacing and watchdogs, so it measures a broken game. The trace keeps
first-N-per-NID plus distinct argument shapes. The stub decides in a few ns whether a call is
interesting:

```
stub:  inc per-NID counter
       if counter > N: jmp real            <- the 99.99% path
       else: append fixed-size binary record to per-thread ring; jmp real
```

Rules, all load-bearing:

- **Fixed-size binary records on-hardware. Formatting happens off-hardware.** Text in the hot
  path is the expense.
- **Per-thread lock-free ring buffers.** No locks, syscalls or allocation on the hot path.
- **Nothing hooked may be called by the logger**, or it reenters. obSCEne's freestanding, no-libc
  discipline is the right posture and the code transfers.
- **A separate low-priority drain thread** moves rings to disk or network, decoupling IO from the
  game threads.
- **Two hook tiers.** Entry-only is cheap and covers everything. Entry-plus-exit, needed for
  return values and out-param bytes, redirects the return address, costs more, and risks stack
  unwinding, so it goes on a whitelist of functions cared about.
- **Sampling.** Full detail for the first ~64 calls per NID, counts after, plus ~1-in-1000 to
  catch variety that appears late. Skip libc and math: the 400,000th `memcpy` teaches nothing.

## Retrieval

Binary on-hardware, `OBS|` records off-hardware:

```
hardware:  ring -> drain thread -> /data/trace-<title>-<n>.bin
   ftp:   pull the .bin
   host:  decode the .bin  ->  OBS| records  ->  the corpus
```

A host-side decoder reads the binary log and emits obSCEne's existing format, so `diff`,
`pretty`, `consensus` and `derive` all operate on trace data and probe results in one comparable
corpus. The decoder is the single place the provenance rule is enforced. Record types align with
[`docs/OUTPUT.md`](OUTPUT.md):

| record | fields | for |
|---|---|---|
| `call`   | nid, lib, seq, arg0..argN (scalar or shape) | arity, constants |
| `ret`    | nid, seq, value, errno-ish | error table, return shapes |
| `outbuf` | nid, seq, offset, hex OR (len, hash) | struct layouts |
| `seq`    | thread, nid-before, nid-after | call ordering |
| `count`  | nid, lib, total | criticality ranking |

## Provenance

Reading the scene's reverse-engineered interoperability tools is the same class of activity as
everything else obSCEne does under principle 6: no vendor headers, no SDK, no decrypted material.
One rule is absolute and lives in the decoder: **log scalars and shapes freely; for any buffer
over a small threshold, record `(length, hash)`, never contents.** A trace of facts is
publishable and is the point of the corpus. A trace of buffer contents redistributes chunks of a
copyrighted title - shaders, audio, textures - and possibly firmware structures. Enforcing it in
one place makes the corpus publishable by construction.

## Pipeline layers

Each layer runs off Payload Manager alone, and each produces a useful result on its own:

1. **Self-hook, no injection.** obSCEne hooks its own imports in its own address space and traces
   its own calls - no attach, no other process. This validates stub, ring, drain, decode and mine
   against a known-correct oracle: if the decoder says obSCEne called `sceKernelWrite` 400 times
   and obSCEne's own report agrees, the pipeline works and only cross-process injection is
   unproven.
2. **Attach-first injection.** Launch the game, then fire the tracer payload at it; it attaches,
   injects the stage-1 hooking code, and traces. This catches everything except very-early boot.
3. **Resident watcher.** Fork-and-watch for a target process spawning, so the payload injects at
   launch and catches early-boot calls - the one thing a resident daemon gives for free.
4. **Mining.** Patterns over the accumulated corpus, feeding orbistoun and any emulator that
   consumes the format.

A prerequisite gates layer 2: a payload run from Payload Manager, launched after a retail game is
already running, must have the privilege to `ptrace(PT_ATTACH)` that game. An existing ftp or
klog payload attempting an attach answers it with no new code.
