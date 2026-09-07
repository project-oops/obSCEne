# 2026-09-08 - The payload leg completes, and all three shapes run end to end

With the eboot leg fixed (161), the payload was the last shape that had never completed on
hardware: ~9 records then SIGBUS at the first check, a garbled `OBS|sink`, and
`guard|on|not initialised`. This session found why and fixed it. Documented in `D327`.

## A wrong turn, and the diagnostic that corrected it

The garbled, run-varying sink read like an un-applied relocation, so the first attempt added an
idempotent self-relocation to `obscene_start`. It changed nothing - the records came back
identical. A direct diagnostic (base, and .data vs .bss read/write tests, over the working
boot-note channel) settled it in one run: .data loads and writes fine, relocations are applied,
but **.bss comes up as garbage** (`bss init(exp 0)=0x7645304445457749`). elfldr does not zero the
`p_memsz`-beyond-`p_filesz` tail of the writable PT_LOAD, so every zero-initialised `static`
starts as whatever was in the page - `s_inited` (the guard never installs), `obs_sink_tried` (the
sink returns its uninitialised buffer), and the crash at check one, all the same cause.

## The fix

`obs_zero_bss()` runs first in `obscene_start`: it takes the load base from `__ehdr_start`
(link-time vaddr 0, so its PC-relative address is the base), walks the program headers, and zeroes
`[p_vaddr+p_filesz, p_vaddr+p_memsz)` for each PT_LOAD. Gated by `-DOBS_ZERO_BSS` on the payload
target only; the eboot and module keep their loader's own zeroing. The self-relocation was removed
- relocation was never the issue.

Hardware, payload leg: `OBS|sink|/mnt/usb0/obscene/report.txt`, `OBS|guard|on|installed (local
setjmp) sa=1 spm=1 pex=1`, no fault, `OBS|tally|62|10|27|165|0`, `OBS|end` - 3706 records, the
whole suite, where it had never passed check one. `make check` green.

## Where it stands

All three delivery shapes now run end to end: payload (D327), package, and native eboot (D326).
`./bin/obscene sweep` produces the six timestamped files, two per leg, in one command, and the
teardown no longer hangs when a leg finishes fast (161).

## Surprise worth keeping

`guard|on|not initialised` had been the answer from the start: `s_available` is .bss (read wrong)
and `s_detail` is initialised .data (read right), and the two disagreeing *is* "the loader did not
zero .bss", stated in the report. A contradiction in the output was a measurement, not noise.
