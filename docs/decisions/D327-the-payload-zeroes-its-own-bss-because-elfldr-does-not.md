# D327 - the payload zeroes its own .bss, because elfldr does not

**assumed** - 2026-09-08

The `payload` shape (plain ET_DYN through elfldr) had never completed on hardware. It emitted its
first ~9 records - meta, build, context, sink, guard, resume, display, the first section and its
first `try` - and then took SIGBUS at `000-boot/number-formatting`. Two things in those records
were wrong before the crash: `OBS|sink|<...>` was garbage that changed every run (`RKS0_`, then a
WebKit-mangled name), and the guard read `OBS|guard|on|not initialised` - `on` and yet its detail
never set. This records what that turned out to be, because the first fix was aimed at the wrong
cause.

## The wrong cause, ruled out

The garbled sink looked exactly like an un-applied relocation: a `const char *` holding a
link-time offset that, in the address space elfldr shares with the foreground app, dereferences
into someone else's memory. The payload has ~40k `R_X86_64_RELATIVE` relocations, so the theory
was that elfldr applied only some. A freestanding, idempotent self-relocation was added to
`obscene_start` to reapply them. It changed nothing: the records came back byte-identical.

## The actual cause, measured

A diagnostic in `obscene_start` reported the load base and tested .data and .bss directly, over
the boot-note channel that was already working. It settled it in one run:

    DIAG data init(exp 0x1111abcd)=0x1111abcd     .data initialiser loaded
    DIAG data wrote0x2222beef read=0x2222beef     .data writes persist
    DIAG bss  init(exp 0)=0x7645304445457749      .bss is NOT zero
    DIAG bss  wrote0x3333cafe read=0x3333cafe     .bss writes persist

Relocation was never the problem - .data is correct and the section-name pointers (relocated
`.data.rel.ro`) had always printed correctly. **elfldr does not zero the .bss** - the
`p_memsz`-beyond-`p_filesz` tail of the writable `PT_LOAD`. Every zero-initialised `static` comes
up as whatever was in the page, and the program assumes zero everywhere:

- `s_inited` in `fault.c` reads non-zero, so `obs_fault_init` takes its `if (s_inited) return;`
  and never installs the handler; `s_available` reads non-zero too, so the record says `on` while
  `s_detail` keeps its initialiser - `on|not initialised`, which had looked self-contradictory.
- `obs_sink_tried` reads non-zero, so `obs_sink_open` returns its early path -
  `obs_sink_reported_path`, itself uninitialised .bss - which is the garbage sink.
- The first check faults on the same class of uninitialised state.

The eboot never showed any of this because the system loader zeroes .bss for it. This is the
plain-ELF path's problem alone.

## The fix

`obs_zero_bss()` runs first in `obscene_start`, before any zero-initialised static is read. It
finds the load base from `__ehdr_start` - the ELF header, at link-time vaddr 0, so its
PC-relative `lea` address *is* the base, with no GOT and no relocated global involved - walks the
program headers, and for each `PT_LOAD` zeroes `[p_vaddr + p_filesz, p_vaddr + p_memsz)`. It
touches nothing but the mapped image and the stack, and is a harmless no-op where a loader
already zeroed. It is gated by `-DOBS_ZERO_BSS`, set on the Makefile `payload:` target only, so
the eboot and module keep their loader's own initialisation untouched. The self-relocation is
gone - relocation was never the issue.

## What the hardware showed

`OBS|sink|/mnt/usb0/obscene/report.txt` (a real path), `OBS|guard|on|installed (local setjmp)
sa=1 spm=1 pex=1`, no fault at the first check, `OBS|tally|62|10|27|165|0` and `OBS|end` - 3706
records, the payload leg completing the whole suite where it had never passed check one. With the
eboot (D326) and package legs already completing, all three delivery shapes now run end to end.

## The lesson, kept

`OBS|guard|on|not initialised` was the tell the whole time: `s_available` is .bss and read wrong,
`s_detail` is initialised .data and read right, and the two disagreeing is exactly "the loader
did not zero .bss" stated in the report. A contradiction in the output was a measurement, not a
glitch - the same reasoning `sink.c` and Principle 1 are built on.
