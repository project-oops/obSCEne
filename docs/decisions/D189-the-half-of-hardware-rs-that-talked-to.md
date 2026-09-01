# D189 - The half of `hardware.rs` that talked to a console now lives in a shared crate, and this project depends on it by path


`src/hardware.rs` was two things wearing one name: a registry of consoles somebody has told
this tool about, and a client for the five services a jailbroken console runs. The emulator
needs the second half - it can only settle some questions by asking real hardware - and had
begun writing its own. A second copy of a transport is a second set of the same mistakes,
found separately, at different times, by whoever is unlucky.

So the transport moved to `pros-link`, in the `prosperous` repository beside this one, and
this crate takes it as a path dependency. The registry stayed: it is this project's own file
in this project's own place, and the shared version of that belongs a layer up in a crate
this tool deliberately does not take.

### Why a path dependency rather than a published version

Publishing would trade a checkout convention for a release process, and a release process is
the worse thing to owe for two consumers in adjacent directories. **The accepted cost is that
`obscene-tool` no longer builds from a standalone clone of this repository** - the
repositories are checked out as a set. Recorded here rather than discovered by whoever hits it.

### Why it does not breach the dependency policy

`Cargo.toml` argues for each of its three dependencies individually and this adds no fourth.
`pros-link` is std-only with an empty dependency table, kept empty *because* this project
takes it: hashing and manifest reading live in a crate above it that never reaches here. It
also forbids unsafe code, which this crate does too.

### What changed at the call sites

`hw check`, `hw logs`, `hw send` and `hw sh` call the crate. The `e_type` guard before a send
is now the crate's, which is the right home for it - it is knowledge about a loader, not about
this project. **The check at the call site stayed anyway**, and it is not a duplicate: it is
where the advice specific to this project's two build outputs can be given, and a shared
library has no business knowing that `obscene.elf` and `obscene.eboot.bin` are both produced
here.

### How it was proved, since a console was not to hand

Against a stand-in written in a different language from the crate's own fake, on the grounds
that two halves of a proof sharing an implementation only ever prove it agrees with itself.
Register, check, logs, sh and send were all run end to end against it, in an isolated home
directory so no real registration was touched: the check reported four services up and the
closed one down with its timing, the log window ended on its own clock, the shell banner did
not leak into the answer, a vendor module was refused before a byte left the machine, and a
payload arrived whole.

### The one thing this breaks, which is not fixed here

**Continuous integration will not build this crate until it checks out `prosperous` too.**
The workflow clones one repository and then runs `cd tool && cargo build`, and the path
dependency points outside it. The fix is a second checkout step before the build; it is not
written yet because the sibling repository has no remote to check out from. Local builds,
tests, clippy and format all pass with the repositories side by side, which is the whole of
what has been established.

Status: **decided** - built and proved locally; the CI checkout is a known open end.

