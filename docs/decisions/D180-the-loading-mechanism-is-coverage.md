# D180 - The loading mechanism is coverage surface, so each one is an artifact


obSCEne measures a platform by calling it. Everything built so far treats *getting there* as
plumbing - a packaging detail on the way to the checks. It is not. **How the program was
loaded is part of what there is to measure**, and the paths available until now all skipped
the piece of the platform this project has written the most about.

Four shapes, four loaders:

| artifact | shape | loaded by |
|---|---|---|
| `obscene.elf` | plain `ET_DYN` | a homebrew ELF loader, which maps the segments itself |
| `obscene.module.elf` | vendor ELF, unsigned | emulators, via their "not a SELF" path |
| `obscene.eboot.bin` | fSELF | **the system loader**, from an app directory |
| `obscene.pkg` | package | **the installer**, then the system loader |

### Why this is not packaging

`docs/BOOT.md` is entirely about the system loader: who calls `DT_INIT`, what `e_type`
decides, how the vendor dynamic table is read, what happens to an import that cannot be
resolved. Its closing section lists what is still unknown, and **every item there is a
question about the loader**.

Neither shipped shape can answer one of them. A homebrew ELF loader maps segments itself, so a
run through it reports what the *libraries* do and nothing else. An emulator's loader is
somebody's reading of the platform, which is the thing being checked rather than the check.
The first hardware run this project ever attempted would have measured elfldr and reported it
as a console.

Only an `eboot.bin` in an app directory is loaded by the real thing. That makes `mkself` the
difference between measuring a console and measuring a homebrew loader that runs on one - not
a convenience, and not a distribution question.

### Separate CI jobs, not one

The interesting failure is *which mechanism broke*. One job stops at the first and hides the
rest, which is the same shape as a summary that reports a number nobody can trace.

The two unimplemented shapes are `continue-on-error` so the gap stays visible without a
permanently red pipeline teaching everybody to ignore it. **That line comes out when they are
implemented.** A tolerated failure nobody ever un-tolerates is a silent one with extra steps.

### The naming was wrong and is corrected here

`obscene.eboot.bin` was briefly the name for the bare vendor ELF. That was an overclaim then
and a collision now: the real eboot is the fSELF. The vendor-shaped bare ELF is
`obscene.module.elf`, which says what it is - the shape emulators accept because none of them
require a SELF.

### What is already known, and what has to be found lawfully

`mkmodule` builds the payload that goes inside a SELF; the container is what is missing.
A retail `eboot.bin` is signed with keys nobody outside the vendor has, which is a wall. A
*fake* SELF is a different object - the same container with a known dummy where the signature
goes - and involves no signing at all. `BOOT.md` said "it cannot sign one, and that is not
going to change", which was true of retail and wrong about this.

The title layout was **measured on hardware** rather than recalled:

```
app0/eboot.bin
app0/sce_sys/param.json
```

`param.json`, not the previous console's `param.sfo`, and four fields:
`applicationCategoryType`, `titleId`, and a `localizedParameters` block carrying
`defaultLanguage` and a `titleName`. A console running a title-registration service picks a
directory of that shape up from `/data/homebrew` and registers it, so a package is not
required to reach the system loader - it is a second mechanism worth covering in its own
right, because the installer is a loader too.

**The provenance boundary is the hard part, not the format.** The container must be built from
public documentation and open-source tooling. Working it out by dumping and reading a console's
own `eboot.bin` is precisely what principle 6 forbids: reimplementation from a binary converges
on the original, and that convergence is what makes work unshareable. `mkmodule` was written
under exactly this constraint, and its header comment records the result - *every fixup here
came from a rejection, not a specification*.

Status: **decided** for the taxonomy and the artifact split; `mkself` and `pkg` are scoped and
unimplemented, and both fail loudly rather than producing nothing.

