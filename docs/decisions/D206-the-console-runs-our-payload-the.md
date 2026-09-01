# D206 - The console runs our payload - the blocker was 16 KB pages, and the loader blocks direct syscalls and passes a bootstrap


Continuing from D205, where the minimal probe faulted at address zero. A ladder of one-question
payloads, each read out through the signal number the kernel reports, established the real
contract - and overturned the D205 conclusion in the process.

### It was never import resolution. Execution never reached us.

D205 concluded elfldr resolves no imports. That was measured on a 4 KB-aligned payload, and it
was the wrong conclusion from a real fault. A payload whose entire body was `ud2` (raises
SIGILL, signal 4) died of **SIGSEGV (11)** instead - so the illegal instruction never ran, so
execution never reached the entry at all. The imports were never the question yet.

**This platform maps with 16 KB pages.** Rebuilt with `-z max-page-size=0x4000
-z common-page-size=0x4000`, the same `ud2` payload died of **SIGILL**. We arrived. The module
build already sets this; the payload builds did not, and `payload-min` still does not.

### The kernel refuses a direct syscall, by name

The syscall-only probe - no imports, no relocations, a raw `syscall` instruction - faulted
with the kernel writing:

```text
PPRBUG-22859: the process pid=658 directly issued a syscall 4 at 0x200004406
```

So the FreeBSD-syscall shortcut is closed on purpose. Every system call must go through
libkernel, which means through resolved imports. There is no import-free path to output.

### The loader passes a live bootstrap pointer

A probe that chose its death by what it found in `rdi` - SIGSEGV if zero, SIGILL if the pointer
was null, SIGTRAP if the word behind it was non-null - died of **SIGTRAP**. So `rdi` holds a
non-null pointer and the first word behind it is itself non-null. Word 0 read out as
`0x4000ab`, a code address in the injected process's range.

That is the payload-args structure the platform convention describes, **measured rather than
assumed**: something is passed, and its first member points at code. Whether that member is the
by-name resolver is not yet settled - a call through it with signature
`int(int, const char*, void**)` returned without faulting but resolved nothing, across four
candidate handles and three name spellings (plain, encoded NID, `nid#lib#mod`).

### A readout channel that needs nothing resolved

Jumping to a value makes the crash backtrace report it: the fault address **is** the value.
Sixty-four bits per send, no import, no syscall. Word 0 of the bootstrap came back this way.
It is race-prone against the log reader and wants a cleaner capture, but it is a real
measurement channel on a target with no output.

### Every fault was survivable

Ten-plus payloads, every one faulting, and after each the console had all five services up and
was immediately re-sendable. No reboot. The loop's core assumption - a crash costs one send,
not the session - holds across a long night of deliberately crashing a hijacked system process.

### What this changes

`payload-min` and every hardware payload must build with 16 KB pages. The import mechanism is
the bootstrap resolver, not ELF relocations - obSCEne's payload entry takes no arguments and
must be changed to take `rdi` and resolve through it. The resolver's exact signature, handle
and name form are the next measurements, and the backtrace channel plus the signal ladder are
how they get taken.

Status: **hardware** - every signal in this entry observed on the console, 2026-08-27.

