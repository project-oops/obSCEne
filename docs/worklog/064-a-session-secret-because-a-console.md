# 2026-08-24 - a session secret, because a console cannot be bound narrowly


Raised by the user, and the framing was the right one: not a wiretap, but **the other devices
on the network**. `PROTOCOL.md` was already honest that this is an unauthenticated socket whose
`call` verb invokes an arbitrary address with six arguments, and offered "bind narrowly" as the
mitigation - which a console cannot do. No shell to tunnel through, driver on another machine,
so the module binds every interface.

A random 128-bit token, generated fresh at every startup and displayed:

```
CMD|1|hello|1|deadbeef…    →  OBS|refused|1|unauthorised
CMD|2|report               →  OBS|refused|2|not-negotiated

CMD|1|hello|1|d98130d1…    →  OBS|hello|1|c0x1|call,read,report
```

Three consecutive startups gave `83b1becd…`, `d98130d1…`, `1c80d633…`. (D165)

The check sits at the top of the `hello` handler rather than beside `greeted`, because the
reply to `hello` names every capability the build has and an unauthenticated peer should not
learn it. Failing there never sets `greeted`, so the pre-existing rule refuses everything else
for free - one check, whole surface.

Entropy is a backend call, because the two targets can honestly offer very different things.
`/dev/urandom` on the host; on the console, the low bits of differences between successive
`sceKernelReadTsc` reads across work whose length varies with the pool, plus a stack address.
The jitter is the only real entropy in that list - the absolute TSC is roughly uptime times
frequency and can be estimated, the differences cannot. Called best-effort in the source and
described as cryptographic nowhere: tens of bits, not 128.

Read off a console the same way the port is: the HUD draws it beside it as `KEY`. When entropy
cannot be had the probe serves unauthenticated and **says so**, because "there is no secret" is
what an operator needs to see.

### Two things I got wrong first

**I answered a question by rebuilding the networking.** The user asked how to add a password;
I went and changed the bind to loopback, wrote a decision about SSH, and had to revert the lot.
The instruction afterwards was explicit - *"I am fine with the existing networking, I don't
want to change it"* - and the revert had to be done by hand, because there is no git here to
undo with.

**The revert missed a document.** `PROTOCOL.md` was left describing a loopback-by-default bind
that no longer existed in the code, from an edit that partly applied before it was rejected.
Caught while writing the real section. Reverting code and forgetting the prose that described
it is its own failure mode, and this repository has no gate that would have caught it: the
doccheck verifies that referenced things *exist*, not that described behaviour is *current*.

**And a build-time secret was the first design**, which is wrong for the obvious reason once
said out loud: a secret compiled into a module is shared by everyone who has that module. The
user caught it - "the password needs to be unique per startup".

