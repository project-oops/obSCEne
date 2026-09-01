# D236 - What a sweep proved crashes belongs in the repository, not in a build directory


*status: decided*

Ten libraries end the process when loaded on real hardware. Establishing that took an iterative
sweep against a console: run, find the `try` record with no matching `res`, exclude it, run
again - ten crashes and ten reboots' worth of patience, for a list of ten names.

That list lived in `$BUILD/excludes.txt`. A build directory. `rm -rf $BUILD` loses it, a second
machine never had it, and nothing in the history records it. The prose in `docs/HARDWARE.md`
named the ten, but prose is not something a build reads.

It is now `data/hardware/crashers.txt`, and both build paths consult it:

- `scripts/sweep-build.sh` seeds a fresh list from it, so a rebuilt build directory does not
  walk back into ten known process kills one at a time.
- `scripts/oops-rebuild-pkg.sh` passes it to `make pkg` as `EXCLUDE`. It did not pass anything
  before - `make pkg` has no exclusions of its own - so every hardware package built through
  that script had all ten back in it unless somebody remembered to type them.

**The file is not a list of libraries this program refuses to probe.** It is what one console
did on one day, and re-running the sweep with it emptied is a valid experiment - the right one
to make when the firmware changes. `SEED=0`, or `EXCLUDE=` in the environment, does that
without editing anything. Exclusions are how you see what is behind a crash, never how you stop
finding them.

The general form: a measurement that cost hardware time to obtain is repository content. Where
it happens to be *used* is a build directory, and the two are not the same file.

