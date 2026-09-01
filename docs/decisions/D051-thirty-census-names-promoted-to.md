# D051 - Thirty census names promoted to behavioural checks, all of them settled by a document


Status: decided.

The census counts a symbol's presence and claims nothing about what it does. That is the
right trade for the vendor surface, where the functions are documented and their exact
behaviour for a given input generally is not - an expectation there is an assumption
however sensible.

The C library half is different: ISO C and POSIX settle it. A name sitting in the census
when a public document says exactly what it must return is coverage left on the table,
and every one promoted carries `OBS_FROM_SPEC` rather than `OBS_FROM_ASSUMED`. Eleven new
checks, and the ratio of settled expectations to assumed ones moves the right way.

**They are chosen for having exact answers.** Every value compared against is exactly
representable in binary floating point - `log2(8)` is 3, `exp(0)` is 1, `strtod("2.5")` is
2.5 - so the comparisons stay exact and need no tolerance. Where the true answer is not
representable, the function is left out: `asin(1)` is pi/2, and checking it would mean
picking an epsilon, which is inventing a specification nobody wrote and building the one
place a wrong answer can hide forever.

**They are chosen for having a wrong answer that looks right.** Each check aims at a
specific plausible failure rather than at the happy path:

- `round(2.5)` must be 3 and `round(-2.5)` must be -3. The hardware's default rounding
  mode is nearest-even and answers 2 and -2; a `round` written as "add a half and
  truncate" answers 3 and -2. Both are common, and no other input reveals either.
- `trunc(-2.7)` must be -2. An implementation that forwards to `floor` answers -3 and is
  correct about every positive input.
- `llabs(-4294967296)` and `atoll("4294967296")` are wider than an `int`. A version that
  forwards to the narrow variant is right about everything a smaller check would use.
- `strtoull("0x10", 0, 0)` must be 16 and `strtoull("010", 0, 0)` must be 8. Base zero
  means read the prefix, and an implementation that treats it as base ten answers 0.
- `strncasecmp("abc", "abd", 2)` must be 0. The bound is the whole difference from
  `strcasecmp`, and reading one character too far is invisible without it.
- `strdup` must return storage that can be *written*. A copy sharing storage with a
  string literal compares equal and faults on the first write, much later and elsewhere.
- The single-precision family gets its own check because an implementation that forwards
  to the double version is numerically right and returns in the wrong register class,
  which corrupts the caller rather than the answer.

**All eleven pass under `make host` against glibc.** That is not a formality: the rule
exists because a probe whose own expectation is wrong reports a working implementation as
broken, and it has caught exactly that twice - most memorably a responsiveness probe
built on `fmod(7, 4)` and `fmod(11, 4)`, which are both 3.

