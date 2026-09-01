# The complete sweep


Fifteen sections, seventy-nine checks, 513 records, a clean terminator, and
`obscene-tool verify` calls it well-formed.

    54 pass   3 partial   18 fail   4 skip

Getting there took three runs, because two checks end the emulator process rather than
returning and each one hides whatever is behind it. Build-time exclusion (D040) made the
sweep iterative rather than blocked: run, see which call did not return, exclude it, run
again. `scripts/sweep-build.sh` turns the accumulated list into the build flag.

**What the run says about the emulator.** `sqrt(4)` is wrong, `fabs` is wrong, rounding a
positive value is wrong, and twelve string and conversion checks fail. Memory, threads,
timers, dynamic linking, user service, audio and input all pass cleanly. Two calls take
the process down.

**The census invalidated itself, correctly.** `900-surface/control` fails with "a symbol
that does not exist reported present" - this emulator resolves every unrecognised import
to a stub, so a presence test cannot tell a real function from a placeholder. The
fourteen library counts are left in the report and the control says what they are worth.
That behaviour was predicted from reading the loader's source, and the control caught it
without being told.

