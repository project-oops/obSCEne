# D207 - The backtrace-echo readout is defeated by log caching, and the signal ladder is the trustworthy channel


D206 read word 0 of the bootstrap as `0x4000ab` by jumping to it and reading the fault address.
Extending that to words 1-4 returned `0x4000ab` every time - and the code genuinely differs per
word (`mov (%rax)` vs `mov 0x10(%rax)` in the disassembly), so identical outputs are not the
struct being uniform. They are the same historical crash re-read: `hw logs` opens a window on a
buffer that still holds the previous send's fault, and a fresh crash at the same site is
indistinguishable from a stale line.

**So the 64-bit backtrace channel is unreliable for a sequence of reads.** One value, once, with
a clean buffer - as word 0 was first taken - is trustworthy. A ladder of them is not, without a
freshness marker the reader can align on, which the raw kernel log does not provide.

The **signal number** does not have this problem: SIGSEGV / SIGILL / SIGTRAP / the PPRBUG
syscall message are each distinct enough that a payload choosing its death encodes a few bits
per send that cannot be confused with a stale SIGSEGV. Every reliable finding in D206 came that
way. The readout channel wants either a freshness marker or a move to a real output path - which
is a resolved `sceKernelDebugOutText`, i.e. the thing this whole investigation is trying to
reach.

So the order is: establish the resolver (signal ladder, a few bits at a time), get one real
import, and then output is a call rather than a crash. Recorded so the next session does not
re-run the multi-word backtrace dump expecting it to work.

Status: **hardware** - the caching was observed directly, five identical reads of five different
offsets.

