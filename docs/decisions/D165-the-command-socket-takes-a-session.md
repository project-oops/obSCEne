# D165 - The command socket takes a session secret, generated per startup and displayed


`docs/PROTOCOL.md` has always been accurate about what this is: an unauthenticated socket
whose `call` verb invokes an arbitrary address with six integer arguments. On a development
machine that is tolerable. **On a console it is not**, and the reason is structural: there is
no shell on it to tunnel through, the driver is on another machine, so the module binds every
interface and anything else on that network can connect and drive it.

### Per startup, not per build

A secret compiled into the module is shared by everyone who has that module, which is the
opposite of a secret. This one is generated when the probe starts listening, lasts that run,
and is replaced by a restart.

That forces the entropy problem onto the target rather than the build machine, and the two
backends can honestly offer very different things - so it is a backend call.
`net_posix.c` reads `/dev/urandom`. `net_target.c` has no such thing and mixes what a console
actually has: **the low bits of differences between successive `sceKernelReadTsc` reads**,
across work whose length varies with the pool so the samples do not settle into a pattern,
plus a stack address for whatever layout randomisation is applied, plus process time.

The jitter is the only real entropy in that list. The *absolute* TSC at startup is roughly
uptime times frequency and can be estimated; the differences depend on cache state and
interrupts and cannot. Mixed with splitmix64's finaliser, chosen because a weak mixer would
let the structure in a counter survive into the output.

**It is called best-effort in the source and not described as cryptographic anywhere**, which
is the honest position: tens of bits, not 128.

### Displayed, because a console has no other channel

The HUD already draws the port so a driver can be pointed at it - the "read the address off
the screen" this protocol assumes - and the secret goes beside it as `KEY`. A new
`OBS_SYS_SECRET` field, absent when there is none, because *"there is no secret"* is
something an operator needs to see: it means anything on the network can drive the probe.

### Where the check goes, and why not beside `greeted`

At the top of the `hello` handler, before the reply. That reply names every capability the
build has, and an unauthenticated peer should not learn it. Returning there never sets
`greeted`, so the pre-existing rule refuses every other verb with `not-negotiated` and one
check protects the whole surface:

```text
CMD|1|hello|1|deadbeef…      OBS|refused|1|unauthorised
CMD|2|report                 OBS|refused|2|not-negotiated
```

The comparison is constant-time. An ordinary one returns at the first differing byte, which
hands a timing adversary the secret a character at a time.

### Contract impact, which is small but not nil

The secret is a **fourth field appended to `hello`**, which `docs/OUTPUT.md` already permits -
new fields go on the end of a line. A driver written before this works unchanged against a
probe that generated no secret, and gets a clear refusal rather than a parse error against one
that did. `obscene-tool drive` needed no change at all: commands are caller-written strings,
so `--command 'hello|1|<secret>'` was already expressible.

`unauthorised` is a **new refusal reason**, added to the closed list in `PROTOCOL.md`. That is
the one part reaching `CLIENT.md`, which is shared with the sibling project, so it goes on the
bridge rather than changing under them.

### What was considered and rejected

**SSH inside the probe.** It cannot be freestanding - curve25519, a stream cipher, SHA-2,
bignums, a key-exchange state machine and key storage, with `libssh` and `wolfSSH` both
wanting malloc and libc against principle 8. Worse, a bug in it would be indistinguishable
from a platform bug, which is the exact failure `make host` exists to prevent (principle 4),
reintroduced inside the guest where every line can end the run and lose the report. And the
console has nowhere to keep a host key.

**A build-time secret.** Rejected on the sharing argument above; it is what the first version
of this decision proposed and it was wrong.

**Nothing, on the grounds that a token is theatre.** That was the previous position and it
holds only against an adversary on the path - who reads the secret off a cleartext wire and
never guesses. It does not hold against the threat actually being defended, which is another
device on the same network. Those are different adversaries and the old text conflated them.

