# The emulator toolkit, and what the first run through it found


The emulators now live in `<emulators>` - binaries and fourteen shallow source
clones - with the run scripts defaulting there (D054). They were in the session
scratchpad, which is transient by design; that they survived at all was luck.

`docs/EMULATORS.md` says what each is and what it is good for. Seven PS4 emulators, two
PS5, five format and symbol references.

### `ps4libdoc` gave the hash an independent confirmation

42,010 established names and 1,130,742 unrecovered identifiers. Feeding the names to
`crack` as candidates, with our own harvested corpus as the known set:

```
# candidates 42010
# generator  reproduced 388 of 389 known pairs
# recovered  0 of 1130742
```

**388 of 389 is agreement.** The single name we have and they do not is
`sceAgcCreateShader` - AGC is the PS5 graphics API and that is a PS4 database, so its
absence is the expected result rather than a disagreement (D055).

Worth sitting with: this chain began with one published pair. It now agrees with a large
independent corpus on every shared name.

`recovered 0` is also the expected answer - those NIDs are exactly the residue these
names could not crack. That file is a target list for a candidate *generator*, which is
still the piece deliberately not built.

**The database carries no library association**, which is what stops its 8,572 vendor
names from being a census expansion. An import with no library resolves to nothing.

### A check of mine that a stub could pass

`037-math/inverse-trigonometry` passed under shadPS4, and the pass was worthless. Every
value it asserts is zero - `tan(0)`, `asin(0)`, `acos(1)`, `atan(0)`, `atan2(0, 1)` -
because zero is the only exact answer those have. A function returning zero to everything
satisfies all five.

This is the exact failure `007-responsive` exists to prevent, committed inside a value
check where that section could not see it. The check now also requires that two differing
inputs produce differing answers, which is the responsiveness argument applied in place.

It still passes, and now that means something.

**Twenty-seven responsiveness probes added** for everything promoted in D051, split
across the two verdicts. All answer on glibc, which is what makes a silence under an
emulator evidence rather than a guess.

### What shadPS4 0.18.0 actually implements

574 records, complete. 60 pass, 5 partial, 34 fail, 7 skip.

**Thirty-three of fifty-five C library functions are silent** - they return the same
value to inputs whose answers must differ, which is a stub rather than a wrong
implementation:

> strspn toupper tolower isdigit isalpha abs labs atoi atol wcslen strstr strcspn
> strtol strtoul atoll strtoull llabs strncasecmp sprintf sqrt fabs floor ceil fmod
> round trunc log2 floorf ceilf fmodf strtod strtof

**Twenty-two answer:**

> strlen strcmp memcmp strncmp strchr pow sin cos exp log log10 tan asin acos atan
> atan2 powf expf logf sinf cosf tanf

The split is stranger than "the maths library is missing". `pow`, `sin`, `cos`, `exp`,
`log`, `log10` and every inverse trigonometric function work; `sqrt`, `fabs`, `floor`,
`ceil`, `fmod`, `round` and `trunc` do not. `log` and `log10` work and `log2` does not.
That is not a layer being absent, it is a list somebody has been working through.

**This is what the failures above were.** "`round(2.5)` is not away from zero", "`llabs`
is wrong", "`strtod("2.5")` is wrong" - every one of those functions is silent, so the
finding is *unimplemented*, not *incorrect*. They need opposite work, and without the
responsiveness verdict the report could not tell them apart. Reading the two together is
the whole design.

**All 262 census symbols resolved.** Every one. Presence and implementation are not the
same thing, and this run is the clearest demonstration of it the project has produced:
262 symbols present, 33 of the 55 tested behaving as stubs.

### A lead worth following: there is no heap

`malloc` returns null, so `calloc`, `realloc` and `strdup` all skip on a missing
capability. Four checks lost to one cause.

The toolchain we now have locally suggests why. `OpenOrbis-PS4-Toolchain` declares
`extern uint64_t sceLibcHeapSize` and its documentation calls it "maximum heap size libc
can use" - a global the runtime is expected to set, with a note that homebrew historically
had to because the default cap was small.

Not acted on yet, and it should not be guessed at: whether that global is what shadPS4
reads, and whether a module is meant to define or import it, are two different questions
and getting either wrong produces a module that loads and misbehaves. But this is the
first concrete lead on the heap, and it came from a repository cloned an hour earlier -
which is the argument for having them.

---

