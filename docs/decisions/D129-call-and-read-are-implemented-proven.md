# D129 - `call` and `read` are implemented, proven live over a socket


Status: derived - driven end to end against a serving build over a real TCP connection.

The two primitives that turn "run the fixed suite remotely" into "ask the target anything",
which is what makes obSCEne an oracle for CPU/NID work. Announced now as capabilities
(`call,read,report`); `write`/`blob`/`reset` stay off (a read or call costs a crash at
worst, a write costs a crash *and* built state).

**`call <addr> <arg>…`** invokes an address with up to six integer arguments through the same
variadic-prototype mechanism `910-bulk` uses (D096): on this ABI the caller cleans up, so a
mismatched arity cannot corrupt the stack, and D008 is about expectations - a `call` asserts
nothing about what it invokes. A malformed address is refused; a *valid but fatal* one is
not, because a faulting call is the designed, normal outcome and the `ack` is already on the
wire, so the death is legible as a lone `ack`.

**`read <addr> <len>`** dumps memory as `bytes` records in the exact shape `obs_report_bytes`
uses, straight to the socket, then a `done` with the length. An unmapped address faults (the
`died` path) rather than being pre-validated - "not readable" and "asking killed it" are
different facts and this reports the second by dying.

Proven on the host serve build (same `net.c`, real socket):

```
read  0x…b000 0x8  -> bytes 7f454c4602010100 (the ELF magic), done returned 0x8
call  obs_display_width -> done returned 0x0   (a real function, its value back)
call  0x0          -> ack, then SIGSEGV, connection reset, no done, seq 5 unanswered
```

The last line is the whole design working: a fatal call leaves a lone `ack` the driver
records as `died`, never `returned 0`.

**Two bugs the live run caught, both wrong-record rather than crash:**
- the `read` id was built with a hand-written `0x` on top of the one `obs_format_hex` already
  writes, producing `read/0x0x…` and breaking the transcript's `read/<addr>` contract;
- `call` special-cased a null address as `bad-argument`, which both contradicted the spec and
  hid the death path a consumer must handle. Removed - `call` invokes what it is told.

**Not yet:** the records from `report` still do not stream down the socket (only the summary),
and the shadPS4 serving path currently dies after the new `160-gpu` section on that loader, so
the live shadPS4 proof used hello/report (D107) while call/read were proven on the host
socket. The mechanism is identical on both; the shadPS4 GPU-section crash is a separate issue.

