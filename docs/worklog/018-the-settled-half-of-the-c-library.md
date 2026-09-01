# The settled half of the C library, promoted from counting to checking


Thirty names moved out of the census and eleven behavioural checks written against them,
all `OBS_FROM_SPEC` (D051). Six in `037-math` - `round`, `trunc`, exponentials and
logarithms, the remaining inverse trigonometry, the whole single-precision family, and
text-to-float - and five in `035-libc`: wide integer text, wide absolute value, bounded
case comparison, `strdup`, `sprintf`.

Every one aims at a plausible wrong answer rather than the happy path. `round(-2.5)`
separates away-from-zero from nearest-even and from "add a half and truncate", which
disagree with each other and with the standard. `trunc(-2.7)` catches a forward to
`floor`. `llabs(-4294967296)` catches a forward to `abs`. `strtoull("010", 0, 0)` catches
a base-zero that does not read the prefix. `strdup` is checked by *writing* to the copy,
because one sharing storage with a string literal compares equal and faults later and
somewhere else.

**All eleven pass under `make host` against glibc**, which is the step that exists
because a probe with a wrong expectation reports a working implementation as broken. The
`fmod(7, 4)` and `fmod(11, 4)` embarrassment is recent enough to still be steering the
work: the new `fmodf` check uses 9 and 4.

### Two surprises, one of them old

**`div` and `ldiv` were written, passed, and then withdrawn** (D052). The struct-by-value
return is an ABI path nothing else here touches and it looked worth having. ISO C does not
specify the member order, so the check read the pair as an unordered set - honest, and it
said what it could not tell. What killed it was the host build: reaching the real `div_t`
means including `<stdlib.h>`, glibc defines several of `platform.h`'s other declarations
inline, and one conflict became a dozen. Keeping the check meant giving up running it
under `make host`, and that is the wrong trade - a check that has never seen a known-good
implementation is not evidence. Back to the census, with the reasoning written into
`platform.h` where the next person to have this idea will find it.

**The census had been carrying `X(FromAddr)`** (D053), and had been for some time. It is
the tail of `sceKernelGetModuleInfoFromAddr`, decapitated when `sceKernelGetModuleInfo`
was promoted out of the census by a match that ignored word boundaries.

It cost twice. `FromAddr` is not a symbol so it censused as absent forever, which the
header is right to call harmless. `sceKernelGetModuleInfoFromAddr` *is* a symbol, and it
silently stopped being counted while the list still appeared to contain it. Nothing would
have caught this: the build succeeds, the census runs, the report is well-formed. It was
found by reading the generated header for an unrelated reason.

`gen-surface.py` now refuses any name not shaped like a symbol - `sce` and a capital, two
leading underscores, or a lowercase first letter. Crude on purpose: it costs nothing and
it closes the only route this has.

Found, notably, by a scan written to check that *this* session's removals had not done the
same thing. They had not. The previous one had.

### State

`verify.sh` green: 86 tool tests, lints clean, three targets, derivation agrees, payload
refused. 404 imports, none unplaced. Census is 260 symbols, down from 290 and up one real
name.

**Not yet run on an emulator.** The shadPS4 install used for every previous run was in the
scratchpad and is gone, so these eleven checks have been validated against glibc and not
against the thing they were written to measure.

---

