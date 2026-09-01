# 2026-08-19 - Symbol census: 30 imports to 277


**Done.** Split the coverage problem in two. The behavioural sections keep asking
whether a function *works*; a new `900-surface` section asks only whether it
*exists*.

- 248 censused symbols across 13 libraries, from one X-macro list per library, so a
  name still appears exactly once.
- Every platform declaration is now weak. An unresolved import is a null address
  rather than a link failure, and the harness **skips** a check whose symbol is absent
  instead of calling it - no more losing every remaining check to one missing
  function.
- New `sym` report record, `verify.py` validates it, `census-summary.sh` reports the
  ratio.
- `ACKNOWLEDGEMENTS.md` written, with the position on names versus signatures versus
  layouts versus implementation set out explicitly.

**Verified.** 49 checks, 12 sections, **277 imports** (was 30). Report well-formed.
Census control passes: 1 present, 247 absent on a host implementing none of the
platform.

**Surprises.**

- **The census could not tell "everything is absent" from "the test is broken".** The
  first run reported all 246 symbols absent and every library red. That is the correct
  answer on this host - and byte-for-byte what a completely non-functional presence
  test would print. Worse, that ambiguity is at its worst exactly when the tool is
  most needed: early, when an emulator implements nothing. Fixed with a control that
  probes one symbol that must resolve and one that must not, defined in a different
  translation unit so it goes through real weak resolution. (D015)

- **Declaring censused names as `const char` rather than as functions turned a rule
  into a compiler error.** The rule "never call a function whose signature you do not
  know" was going to be enforced by 248 opportunities to remember it. Declaring them as
  data means the type system rejects the call outright. The safer design was also the
  shorter one.

- **Two census names collided with real declarations in `platform.h`** - a name cannot
  be both a function and a `const char`. Worth having a check for rather than
  discovering at the next expansion, since the census is expected to keep growing.

- **clang-format mangled the census lists, and the damage was not idempotent.** It
  reads `X(name) X(name)` as chained calls and reflows them into a staircase of
  continuation indents; formatting and then re-checking *still* reported violations,
  so the CI gate could never have passed. Fenced with `clang-format off` and rewritten
  one symbol per line. (D016)

- **Trying to recover the list by parsing the mangled file lost a third of it.** A
  regex over the wrecked header recovered 213 of 246 names and dropped a whole library
  - and would have passed review, because the file still compiled and the census still
  produced a plausible number. Regenerated from the authored list instead. The lesson
  is narrow and worth keeping: when a mechanical transform corrupts a source of truth,
  reconstruct it from the source, not from the wreckage.

- **`clang-format` over the mount is slow enough to matter** - several minutes for 20
  files, because every read and write crosses the share. Worth moving builds to a
  VM-local copy if this grows much further.

**Not done.** The census is presence only; it says nothing about behaviour, and a
platform stubbing every function to return success would score 100%. Struct-taking
functions are still absent from the behavioural checks (D008), so the positive-check
ratio has not moved. Never run on real hardware or under an emulator.

