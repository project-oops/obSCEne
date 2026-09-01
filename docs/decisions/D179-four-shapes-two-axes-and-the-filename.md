# D179 - Four shapes, two axes, and the filename now says which


The probe ships in more than one shape and the names hid the only fact that matters about
them. Everything ended in `.elf`:

| was | actual shape | who takes it |
|---|---|---|
| `obscene-module.elf` | vendor (`e_type 0xFE10`, `DT_SCE_*`) | emulators |
| `obscene-payload.elf` | plain `ET_DYN` | a console's homebrew loader |
| `obscene-min.elf` | **vendor** | emulators |

Two entirely different formats for two entirely different consumers, distinguished by a word
in the middle of the name that reads like a synonym. The Makefile explains the difference
properly - *"no emulator will touch it… both exist solely to produce the shape this one must
not have"* - in a comment beside the rule, where it is found after the fact rather than
before.

Which is what happened the first time a console was available: 13MB of vendor module was sent
to a homebrew ELF loader, which exited without a word, taking the loader down with it. The
information needed to avoid that was in the repository, in the right file, written clearly.

So the published names carry the axis:

```
                  vendor shape (emulators)   plain ELF (console)
full suite        obscene.eboot.bin          obscene.elf
one import        obscene-min.eboot.bin      obscene-min.elf
```

### The grid found a missing build

Laying it out as two axes rather than a list of rules made the hole obvious: **all three
minimal builds were vendor-shaped**. There was no small thing to send a console, which is
exactly why the first artifact tried on hardware was the 13MB one. `payload-min` fills it, at
**3,848 bytes** against 9.2MB for the full plain build.

It needed one behavioural change to be useful there. The minimal builds report through
descriptors 1 and 2, which an emulator's host process catches and a console has nobody to
catch - a run that worked perfectly would leave no evidence, which is indistinguishable from
one that failed. `OBSCENE_MIN_FILE` writes the same line to `/data/obscene-report.txt`, the
first entry in the full build's sink candidate list and confirmed writable on hardware.

It also ends differently. `for (;;)` is right on an emulator, where reaching the loop *is* the
result and returning would fault in a way that reads as never having started - but that
reasoning assumes somebody is watching. On a console nobody is, the evidence is already on
disk, and spinning only pegs a core until someone kills the process.

### `.eboot.bin` overclaims slightly, and it is still the right name

A real `eboot.bin` is a signed SELF. This is a bare ELF in vendor shape, and `docs/BOOT.md`
is explicit that signing one is not possible here. The name is kept because it answers the
question a reader actually has - *where does this file go* - and it goes exactly where an
`eboot.bin` goes in an emulator's app directory. The inaccuracy is documented rather than
designed out, because every accurate alternative answers a question nobody asked.

### `HARDWARE=1` makes the blind prober a compile error

`910-bulk` calls every resolvable symbol with six zero arguments. Of the 67,053 entries it
walks, the named ones include `sceSystemServiceRequestPowerOff`, `sceLncUtilSystemShutdown`,
`sceShellCoreUtilRequestShutdown` and ten more of that kind. Calling one of those with zeros
is a plausible way to switch somebody's console off mid-run.

A blocklist cannot fix it, and the number that settles that is **50,344 of the 67,053 are
unnamed NIDs** - three quarters of the sweep cannot be screened because nobody knows what
those functions are. So the guard is total: `HARDWARE=1` with `BULK=1` is a `$(error)`.

The protection before this was that `BULK` defaults to empty, which is a convention, and a
convention is what fails at one in the morning when somebody reuses the command line already
in their shell.

Two other things hold independently of the guard, and both are structural rather than
remembered:

- **Census symbols are uncallable by type.** `#define OBS_DECLARE_SYMBOL(name) extern OBS_WEAK
  const char name;` - every censused name, `sceSystemServiceRequestPowerOff` included, is
  declared as data. Only `&name` is ever taken. Calling one does not compile. The single place
  that casts past it is the prober.
- **CI proves the guard by making it fail.** The step passes only when `make` does not.

Status: **decided** - the guard has a negative test, and all four shapes build.

