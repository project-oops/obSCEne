# 2026-08-27 - first payload execution on a real PS5, and the loader contract, by signal


A ladder of one-question payloads against 192.168.1.211 (fw 12.40), each reading its answer
out through the signal the kernel reports. What was learned, in order:

1. **16 KB pages.** The D205 "elfldr resolves no imports" conclusion was wrong - it was drawn
   from a 4 KB-aligned payload that never reached its entry. A `ud2` payload proved it: SIGSEGV
   at 4 KB alignment (never ran), SIGILL at 16 KB (ran). This platform maps with 16 KB pages;
   the module build sets `max-page-size=0x4000` and the payload builds did not. (D206)
2. **Direct syscalls are refused**, by name: `PPRBUG-22859: the process directly issued a
   syscall 4`. There is no import-free path to output.
3. **A bootstrap pointer is passed** in `rdi`: non-null, first word non-null (SIGTRAP from a
   probe that branched on exactly that). Word 0 = `0x4000ab`, a code address in the injected
   process. Measured, not assumed. (D206)
4. **The backtrace-echo readout (jump-to-value, read the fault address) is defeated by log
   caching** for a *sequence* of reads: five different struct offsets all reported `0x4000ab`
   because `hw logs` re-read one stale crash. Good for one clean value, not a ladder. The
   signal number is the trustworthy channel. (D207)
5. **The first bootstrap member is not a `(int,const char*,void**)` resolver** as called: every
   handle x nameform faulted (SIGSEGV), and `0x4000ab` is loader-space, not a libkernel
   address. What it *is* needs the elfldr source, which this project does not have and will not
   guess (D008).

**The console survived every one of ~20 deliberate crashes** with all five services up and
immediately re-sendable. The loop's core bet holds on real hardware.

Blocked here on the payload entry ABI - specifically what elfldr passes and how a payload is
meant to resolve imports through it. That is etaHEN's `libLoader` convention; established
homebrew (BFpilot, CheatRunner in the payload list) links against a known headers set that
encodes it. The honest next step is that convention, not more black-box probing.

