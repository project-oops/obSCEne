# The call/response tracer - design

A companion instrument to obSCEne, sharing its report format and its provenance discipline.
Where obSCEne is **active** (we choose the inputs and can test failure deliberately), the tracer
is **passive**: it records what real titles actually call, in real sequences, including
functions nobody has named. The two cover each other's blind spots - the tracer finds the
coordinates a real game visits, obSCEne maps the territory around them.

Status: **design only.** Nothing here is built. The provenance fork in section 5 is a decision
for the repo owner and gates any injection code.

## 1. The injection question is answered, and we ship one self-contained ELF

The scene has already solved every primitive this needs, on-hardware and in the open. We read
those projects to **understand** the technique and reimplement it in obSCEne's own freestanding
C - we do not link them, and nothing they publish gets installed on the hardware. This is
**Option B** (decided; see section 5): the tracer is a single self-contained payload that needs
nothing on the box beyond what obSCEne already needs - a jailbroken hardware and a loader that
runs one ELF. Payload Manager is enough; etaHEN is not required.

Established by reading two GPL projects (`<clones>/CheatRunner`, `<clones>/etaHEN`) on 2026-08-27.
Each maps to a piece of C we write ourselves:

- **Inject code into a running game** - the technique in etaHEN's `libelfldr`
  (`elfldr_exec(pid, ...)`): `ptrace(PT_ATTACH)`, remote `pt_mmap` of `PROT_EXEC` pages *inside
  the target*, copy the ELF's `PT_LOAD` segments in, `pt_mprotect`, then hijack a thread
  (`r.r_rip = entry`; resume). About 200 lines, well understood, C-to-C. This is the part we
  reimplement.

- **Read a running game's memory** - CheatRunner's `cr_mdbg.c` shows the kernel R/W path
  (`mdbg_copyout`/`copyin`; a CR3 page-table walk to the physical page on FW 8.20+). We need it
  only if we ever go out-of-process; Option B works in-process and mostly does not.

- **NID to address, and the call-site detour, we already have.** Once our payload is executing
  *inside* the target, hooking is memory writes in our own address space. Walking the game's
  import tables (`DT_JMPREL`/`Elf64_Sym`) to find each function pointer is the D193 work;
  `libNidResolver`'s runtime resolve is the same NID hashing obSCEne already does; a `rel32`
  detour over a resolved slot (etaHEN's `install_raw_hook`) is ten lines. No external code.

And the method itself is already practice: etaHEN's own PKG writeup describes *"Tracing BGFT IPC
calls within shellcore"* to reverse an error-code change. We are industrialising an existing
technique in our own code, not inventing one and not borrowing a binary.

## 2. What the corpus is for, and what a record holds

The mineable facts, ranked by what they unblock that nothing else can:

1. **Arity and argument shapes** - which registers hold non-garbage across thousands of calls.
   Turns D008's *assumed* arities into *derived* ones. The ladder the whole project climbs.
2. **The valid constant space** - every distinct flags/type value a real title passes is a
   *known-good* constant. How you learn "memory type 3 exists" without inventing it.
   Unobtainable from static analysis.
3. **Struct layouts from real data** - out-param buffer before/after, diffed. The poison-pattern
   dump, but on real structures with real values.
4. **Call sequences** - which functions must precede which; init orders; "X before Y or Y
   returns Z". Nearly impossible to derive from a census; exactly what breaks emulators.
5. **Errors in context** - the returned code *and what the caller did next* (retry / fall back /
   abort). The difference between knowing a code and failing the way a caller expects.
6. **Criticality ranking** - which NIDs every title calls. An evidence-based implementation
   order for emulator authors, replacing a guessed one.

## 3. The hot path is a volume problem, not an interception one

A title makes millions of calls a second. A complete trace fills storage in seconds and
perturbs timing enough to trip frame pacing / watchdogs - then you are measuring a broken game.
You do not want a complete trace. You want **first-N-per-NID plus distinct argument shapes**.

The stub must decide in a few ns whether a call is interesting:

```
stub:  inc per-NID counter
       if counter > N: jmp real            <- the 99.99% path
       else: append fixed-size binary record to per-thread ring; jmp real
```

Rules, all of them load-bearing:

- **Fixed-size binary records on-hardware. Formatting happens off-hardware.** Text in the hot path
  is the expense.
- **Per-thread lock-free ring buffers.** No locks, no syscalls, no allocation on the hot path.
- **Nothing hooked may be called by the logger.** Reentrancy otherwise. obSCEne's freestanding,
  no-libc discipline is exactly the right posture and the code transfers.
- **A separate low-priority drain thread** moves rings to disk/network, decoupling IO from the
  game threads.
- **Two hook tiers.** Entry-only is cheap and can cover everything. Entry+exit (needed for
  return values and out-param bytes) hijacks the return address, costs more, and risks stack
  unwinding / exception handling - so it goes on a whitelist of functions actually cared about.
- **Sampling**: full detail for the first ~64 calls per NID, counts after, plus ~1-in-1000 to
  catch variety that appears late (a function used differently in level 3 than level 1). Skip
  libc/math entirely - the 400,000th `memcpy` teaches nothing.

## 4. Retrieval: binary on-hardware, OBS records off-hardware

```
hardware:  ring -> drain thread -> /data/trace-<title>-<n>.bin
   ftp:   pull the .bin (etaHEN/CheatRunner already serve one)
   host:  obscene-tool trace <bin>  ->  OBS| records  ->  the corpus
```

The decoder emitting obSCEne's existing format is the point: `diff`, `pretty`, `consensus`,
`derive` all already operate on it, so trace data and probe results land in one comparable
corpus. The decoder is also the single place the provenance rule is enforced (section 5).

Proposed record types (shapes matter more than names; align with `docs/OUTPUT.md` and the
measurement records proposed in `docs/HARDWARE-PROBE.md`):

| record | fields | for |
|---|---|---|
| `call`   | nid, lib, seq, arg0..argN (scalar or shape) | arity, constants |
| `ret`    | nid, seq, value, errno-ish | error table, return shapes |
| `outbuf` | nid, seq, offset, hex OR (len, hash) | struct layouts |
| `seq`    | thread, nid-before, nid-after | call ordering |
| `count`  | nid, lib, total | criticality ranking |

## 5. Provenance - decided: Option B, learn-not-link

The scene's projects are **clean**: reverse-engineered interoperability tools, not decompiled
firmware, so reading them is the same class of activity as everything else obSCEne does under
principle 6 (no vendor headers, no SDK, no decrypted material - these are none of those). The
question was only whether to **link** their GPL code or reimplement the technique.

**Decided: reimplement (Option B).** The two considerations both point the same way:

- **It is cheap here.** The valuable piece - inject-into-a-running-process - is ~200 lines of
  well-understood C, and it reimplements *into a C codebase that already owns the other half*:
  obSCEne parses the executable format and the import tables (`../selfish`, D193, D200), so the
  find-the-function-pointer step is code we have. This is not the weeks-of-Rust that the same
  choice would cost orbistoun.
- **It keeps every repo clean.** Linking `libelfldr`/`libNidResolver` (GPLv3 / GPLv2) would make
  the linking binary GPL - a one-way door. Reimplementing keeps obSCEne's licence its own
  choice and keeps GPL nowhere near orbistoun, which never sees this code regardless (it
  consumes only the corpus; see section 4).

The tracer is therefore a **self-contained payload**. Its only hardware-side requirement is a
loader that runs one ELF with the jailbreak's privileges - Payload Manager, which the hardware
already has. It does its own attach, injection, and hooking; it depends on no resident daemon
and no installed library.

Independent of all that, one rule is absolute and lives in the decoder: **log scalars and shapes
freely; for any buffer over a small threshold, record `(length, hash)`, never contents.** A
trace of facts is publishable and is the whole point of the corpus. A trace of buffer contents
redistributes chunks of a copyrighted title (shaders, audio, textures) and possibly firmware
structures. Enforced in one place, mechanically, so the corpus is publishable by construction.

## 6. Build order

The staging exists so there is a useful result long before the whole thing is built, and so the
risky part (injection) is attempted only once the cheap parts are known good. Every stage runs
off Payload Manager alone.

1. **Pipeline against obSCEne itself, no injection.** obSCEne hooks its *own* imports in its own
   address space - no attach, no other process - and traces its own calls. We own that binary,
   it makes a known set of calls, it is already on five loaders and shortly on hardware. This
   validates stub -> ring -> drain -> decode -> mine against a known-correct oracle: if the
   decoder says obSCEne called `sceKernelWrite` 400 times and obSCEne's own report agrees, the
   whole pipeline works, and the only thing still unproven is getting *into another process*.

2. **The go/no-go, five minutes, do this before writing injection code.** Does a payload run
   from Payload Manager, launched *after* a retail game is already running, have the privilege
   to `ptrace(PT_ATTACH)` that game? That single question decides whether Option B's in-process
   plan holds on this hardware. It needs no new code - an existing ftp/klog payload attempting an
   attach is enough to answer it.

3. **Attach-first injection.** Reimplement the ~200-line inject-into-a-running-process technique
   (section 1). Flow: launch the game, then fire the tracer payload at it; it attaches, injects
   the hooking code built in stage 1, and traces. Simplest path, needs nothing but Payload
   Manager, and catches everything except very-early boot.

4. **Resident watcher (optional, later).** Fork-and-watch for a target process spawning so the
   payload can inject at launch and catch early-boot calls - the one thing a resident daemon
   like etaHEN gives for free and we would otherwise miss. Only worth it if early-boot calls
   turn out to matter.

5. **Mining.** Patterns over the accumulated corpus, feeding orbistoun and any emulator that
   consumes the format.

Scope note: done properly this is a project comparable in size to obSCEne. The staging is what
keeps that from being one long stretch before the first useful output - stage 1 alone produces
a working, verifiable trace of real hardware payload.
