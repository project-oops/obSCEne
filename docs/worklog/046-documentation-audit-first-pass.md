# Documentation audit, first pass


Asked for after a run of changes, and it found the kind of drift that matters rather than
typos. Six corrections, all in files that describe the program as it is now:

- **`README.md` and `docs/OUTPUT.md` both said "nothing in the census is ever called".**
  `910-bulk` calls censused symbols by design, and `OUTPUT.md` documents the record type it
  emits *ten lines below the sentence denying it exists*. A format contract contradicting
  itself on one page is the worst place for this. Both now state the rule and the deliberate
  exception, including why the exception is a cast rather than a redeclaration.
- **`README.md` status block** claimed shadPS4 executes the suite, Kyty relocates and
  craziiEmu loads - written when that was the whole story. Four loaders now reach
  `REPORT COMPLETE`, and one of them no longer loads the corpus module at all (D149).
- **`README.md` census figures** were from the 383-symbol era: "all 312 report present",
  "names the other 238", "several hundred symbols". The census is 39,549 across 372
  libraries, and the honest comparison is now shadPS4's 35,337-of-35,337 against PS5PCEM's
  31,601 absent - where the higher number is the less honest one.
- **`CLAUDE.md`** said "fourteen emulators"; there are eighteen and all are documented. The
  number was carrying no information and rotting, so it is gone rather than corrected -
  numbers that inform get gated, numbers that decorate get deleted.
- **Two source comments** still described the exclusion list as naming checks only, which
  stopped being true when section entries landed (D145).

Two things the audit did **not** flag, deliberately. `DECISIONS.md` and `WORKLOG.md` are
full of counts that no longer hold - 79 checks, 326 censused symbols - and those are dated
records whose numbers were true when written; `doccheck.py` exempts them for that reason and
"correcting" them would be falsifying a log. And `COMPATIBILITY.md`'s "499 checks" describes
the report it documents rather than the current build, which is also correct.

This is a first pass over the live documents. The per-file source comments are dense and
have not been swept end to end.

