# D280 - Porthole scaffolded, and the encoder reachability made a section (106-encoder)


**assumed** - 2026-09-01

Two pieces toward Porthole - the DIY remote-play stand-in whose design is prosperous/docs/VIDEO.md
part three (own video stream out, own controller state in, over our own payload, so a jailbroken
target is watched and played without the vendor's remote-play protocol).

**The payload is scaffolded, in `porthole/`, isolated the way `tracer/` is.** The bytes on the two
sockets (9805 encoded video out, 9806 the 24-byte PPAD input record) are decided and real - the wire
contract, with a host self-test that the record layout and its decoder agree. The three things the
payload actually does - reach the encoder, capture-and-encode, apply a pad - are stubs, each gated on
the one open question and marked, because nothing about them is established yet. `make -C porthole
check` proves the contract; `make -C porthole skeleton` proves it compiles freestanding for the
target. The payload lives in obSCEne (homebrew that runs on the target - VIDEO.md's own call); the
host client is prosperous's `pros-video`.

**The go/no-go is now a section: `106-encoder`.** VIDEO.md rests Porthole on "can an unsigned payload
reach the encoder?", and `libSceVencCore` is that encoder (`105-record`/libSceVideoRecording is the
high-level driver already probed). This section answers the *first half* - are `sceVencCoreCreateEncoder`,
`GetAuData` and `QueryMemorySize` present and callable from this build? - and only that half. It does
not call them: their arities are unconfirmed and the create/set-input calls take structures nobody here
has, so they are declared as data, uncallable, and the harness's own skip-if-unresolved does the
measuring (reaching a check body is the fact of presence). The second half - open a session, pull one
access unit - is protocol-harness work where a fault is `died` not lost, exactly as 105-record reserves
its own pointer-taking calls. On the host build all three skip (no PS5 encoder), which is correct; on a
console they pass if the encoder binds, skip if the import table does not (the payload-self-resolver
question, D277) - and the section header says to read a skip with that in mind.

Provenance: the `sceVencCore` names and their library are in the mined corpus from several public
sources (aerolib, ps4libdoc, fpPS4); no vendor header, no signature claimed beyond "it resolves".

Not done here, and not mine: the committed `make host` is currently blocked by the untracked
`src/common/` (freestd.c/h, the D278 injector's shared helpers) whose `#include "common/freestd.h"`
is not on the `-Iinclude` path. 106-encoder and the Porthole scaffold each build clean on their own
(verified in isolation and with an `-Isrc` override); the block is the injector WIP's to resolve.

