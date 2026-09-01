# D107 - The console socket backend is implemented, and proven inside shadPS4 with no hardware


Status: derived - obSCEne served the protocol over `sceNet*` inside shadPS4 and a client on
the host drove it end to end.

D126 left `net_target.c` refusing, because the vendor networking signatures were unconfirmed
and D008 forbids calling into an uncertain arity. The premise changed: the signatures are now
confirmed from two independent public sources that agree exactly - the OpenOrbis toolchain
headers and shadPS4's own `libSceNet` - so the refusal was replaced with a real
implementation. `sceNetSocket`'s unusual first argument (a name string) is confirmed by both,
which is the kind of detail D008 exists to pin down.

### Why this needs no Steam Deck

An emulator whose net layer maps guest sockets onto host sockets makes a guest listen open a
real host port. shadPS4 does exactly this - `PosixSocket::bind` calls the host `::bind`,
`::listen` calls `::listen`. So obSCEne built `GEN=4 SERVE=1`, run inside shadPS4, calls
`sceNetListen` and **host port 9803 opens**; a client on the host connects and drives the
protocol. The transport reports `scenet`, so this is the vendor path exercised, not a host
stand-in.

Observed:

```
OBS|end|sceKernelWrite
OBS|net|listening|9803
  TCP    0.0.0.0:9803    LISTENING    (shadPS4)
-> CMD|1|hello|1
OBS|hello|1|t0x13bd05e|report
OBS|part|t0x13bd05e|transport|scenet
OBS|done|1|ok||
```

### Two things this settled about the serving build

**It must be built with the exclusion list.** The serve loop runs *after* the suite, so a
module that hits a known crash mid-suite never reaches it - the first two attempts died at
`040-file/open-rejects-null` exactly as the sweep predicts. A serving module is a
sweep-excluded module with `SERVE=1`, not a plain one.

**The report goes first, the socket second.** `obscene_start` runs the suite (report to
stdout, file sink, and screen), then announces `net|listening` and serves - looped, so the
driver can reconnect after a faulting command ends a session. The static report is what
survives if the network never comes up; the socket is the live half on top of it.

The Deck is still the RDNA2 ISA oracle it was always going to be. It is no longer on the
critical path for *building and proving the networking*, which is now done.

