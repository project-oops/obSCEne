# 2026-08-27 (cont.) - the payload ABI, fully mapped by signal, and the real frontier


Forty-odd one-question payloads against 192.168.1.211, each answering through the signal it
died of. What was established:

- **16 KB pages were the D205 "no imports" blocker.** A `ud2` payload proved it: SIGSEGV at 4 KB
  (never ran), SIGILL at 16 KB (ran). Now fixed in the Makefile for both payload targets. (D208)
- **The bootstrap is ps5-payload-sdk `payload_args`, fully mapped** by a canonical readout
  channel (jump to a marker-tagged value, read the fault address): dlsym / rwpipe / rwpair /
  kpipe_addr / kdata_base_addr / payloadout, the last two being kernel addresses that prove what
  the struct is. (D208)
- **Word 0 is execute-only and does not resolve.** Callable, unreadable (`0xa0020328` fault),
  and called as a resolver it returns a pid-like counter ignoring its name - 96 handle x
  convention x name-form combinations, nothing in the libkernel band. The payload is sandboxed;
  the escape primitives are in the struct precisely because an escape is expected first.
- **The backtrace-echo channel corrupts non-canonical reads** - D206's `0x4000ab` was such an
  artefact; the real word 0 is `0x8000005b0`. Only canonical tags are trustworthy. (D207/D208)

**The frontier is precise and external:** the working `dlsym` invocation (and whether a
`ucred`/kernel-R-W escape precedes it) lives in the open-source ps5-payload-sdk / etaHEN elfldr
crt0, or in the 12.40 kernel offsets an escape needs. Reading that source is the next step;
guessing further handles is a D008 violation and was stopped.

**The console survived every send** - all five services up after each of ~40 deliberate faults,
no reboot. The loop's core bet is proven on metal.

