# Patches to other people's emulators

Local changes to loaders in `<emulators>`, kept here so they can be reproduced,
inspected, or refused. Nothing in this directory is applied automatically and nothing in
obSCEne depends on any of it.

**Why they are stored rather than just made.** A patched loader is not the loader. If a
result from one ever reached `docs/COMPATIBILITY.md` under the loader's own name, the table
would describe a build that exists on one machine - the same class of lie as an invented
constant, told at a larger scale. Storing the diff is what makes the claim checkable: anyone
can see exactly how far the thing measured is from the thing named. (D176)

## `kyty-buildable.patch`

Two build settings, no behaviour change. Needed to compile Kyty at all on this machine:

- `KYTY_WARNINGS_ARE_ERRORS` respects a `-D` override instead of being forced `ON`. The
  bundled SDL2 is from 2022 and declares `TF_PROFILE_DAYI`, which the 2026 Windows SDK now
  also declares in `msctf.h`; `-Wshadow` fires inside a third-party dependency and `/WX`
  turns it into a build failure.
- `KYTY_NO_LAUNCHER` skips the Qt launcher independently of `Build_Tools`. Setting
  `Build_Tools` also moves `KYTY_PROJECT` away from `KYTY_PROJECT_EMULATOR`, which compiles
  the emulator out via `KYTY_EMU_ENABLED` - `fc_script` then builds with no `kyty_*` Lua
  bindings at all.

Applying this changes no emulator behaviour, so results from a build carrying only this
patch are ordinary results.

## `kyty-probe-friendly.patch`

Two behaviour changes. **Results from a build carrying this are not results about Kyty**, and
belong in `reports/kyty-patched.txt` rather than `reports/kyty.txt`.

- `Jit::JmpWithIndex` - an unresolved import's trampoline becomes `xor eax,eax; ret` instead
  of jumping to `RelocateHandler`, which prints a stack trace and calls `EXIT`. Upstream,
  calling one unimplemented function ends the process.
- `VideoOutOpen` - accepts any user id. Upstream refuses anything but `255` or `0`, which
  contradicts the same build's `UserServiceGetInitialUser` returning `1`.

### What it is good for, and what it is not

Not measurement. Kyty holds **1,970 `EXIT_NOT_IMPLEMENTED` sites and 207 `EXIT` calls** - it
is designed to abort on anything it does not handle, which is a reasonable choice for running
games and an incompatible one with a probe that sweeps input spaces deliberately. Patching
past all of them would leave every unhandled case continuing with default state, which is
exactly the stub-everything result `900-surface/control` exists to mark `(void)`.

It is good for **finding bugs in the loader**. In one sitting it took obSCEne from 1 record to
102 and surfaced the `VideoOutOpen` contradiction above, plus `PthreadMutexattrSettype`
calling `EXIT` on an unrecognised type where POSIX specifies `EINVAL`. The second of those
produced D177, which is a finding about the platform rather than about Kyty.

## Applying and reverting

```sh
cd <emulators>/src/Kyty && git apply <OOPS>/obscene/patches/kyty-buildable.patch
```

The tree is left pristine after a session. `.orig` copies beside each edited file are the
faster route back if one is still there.

## `fpps4-probe-friendly.patch`

One hunk in `fpPS4.lpr`. `print_stub` - the handler bound to every unresolved function import -
calls `Sleep(INFINITE)` upstream, freezing the calling thread so a developer can attach a
debugger. The patch makes it a function returning zero instead, which is what the unused
`_nop_stub` beside it already does.

Behaviour-changing, so `reports/fpps4-patched.txt` and not the stock row. (D176)

36 records and a permanent hang become 36,631 records and a complete suite in two seconds,
surviving 181 unimplemented-import calls that would each have ended the run. (D196)

Rebuild with `lazbuild fpPS4.lpi`.
