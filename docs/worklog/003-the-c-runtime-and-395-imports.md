# 2026-08-19 - The C runtime, and 395 imports


**Done.** Scanned public sources for what the first two passes had missed, then acted
on the largest finding.

- New `035-libc` section: ten **positive** behavioural checks against the C runtime.
- 105 more censused symbols; 14 groups, 353 names, 395 imports.
- `docs/BACKLOG.md` written - the gaps found and deliberately *not* filled.
- `ACKNOWLEDGEMENTS.md` position hardened into D018.

**Verified.** 13 sections, 60 checks. Host run: 27 pass, 5 partial, 20 fail, 8 skip;
report well-formed. `035-libc` scores **10 of 10** against real glibc, and the census
reports `libc` as **partial (71 present)** - so all three census states are now
demonstrated by a real run rather than argued for.

**Surprises.**

- **The biggest gap was the one that needed no research.** Every other library needed
  looking up; the C runtime needed only remembering, and it was the one thing missed.
  A freestanding probe was written and the guest was then assumed to be freestanding
  too. A title imports more of libc than of any single vendor subsystem.

- **Declaring standard C names as data does not work - clang knows them as
  builtins.** `strcpy` as a `const char` is "redefinition as a different kind of
  symbol". The D014 trick holds for vendor names and collides head-on with anything
  the compiler has an opinion about. `-fno-builtin` on the host build fixes it, and is
  the honest flag rather than a workaround: the host build stands in for a guest
  platform, so clang genuinely should not assume it knows what these functions do. The
  target build already had it, which is why the failure appeared only on the host.

- **The community SDK for the current-generation console builds its symbol stubs from
  decrypted vendor libraries.** The names are correct and the chain of custody is not
  one this repository can adopt under rule 1. That rules out the easy route to the
  current graphics interface, and the honest answer is that the project does not have
  those names yet rather than that it has guessed them. (D018, backlog item 1)

- **The host build finally produces meaningful passes.** Until now every green was a
  negative check agreeing that a stub fails. The libc section passes because real
  implementations do the right thing - which also proves the positive checks can tell
  the difference, something no amount of red ever demonstrated.

**Not done.** The current generation graphics interface is still absent, and
deliberately so. Struct-taking functions remain out of reach. Nothing verifies a NID
mapping, which is the fault an emulator is most likely to have and this program least
able to see. Twelve more libraries unlisted. Never run on hardware or under an
emulator.

