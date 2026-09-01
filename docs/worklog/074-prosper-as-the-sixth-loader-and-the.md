# 2026-08-26 - prosper as the sixth loader, and the sweep made to fit on a desk


### prosper

obSCEne boots under `boot_trace`, headless, and the resume mechanism converges on it
(118 → 181 records over six rounds). `hle_registry_dump` gave the thing no other loader can:
its own registry of what it implements, which turned `007-responsive` from a plausible
heuristic into a measured one - **54 of 54**, no false stubs and no missed ones. (D195)

It stops on a SIGSEGV inside prosper's own `real` semaphore implementation. The dangling `try`
named the call first, on a loader obSCEne had never met.

### The sweep

The user's complaint was concrete and correct: runs were long enough that they minimised the
windows to get on with work, and a minimised window cannot be captured - so the screenshot
failed on runs that had gone fine.

Most of the length was D194's deadlock, and fixing it took the whole sweep from ~850s of
emulator time to **87s**: shad 7s, cem 4s, fp 14s, kyty 60s, orbistoun 2s. What is left:

- **Frames are banked during the run**, not taken once at the end. The end of a run is the
  moment a capture is most likely to fail, and a failed capture now leaves the last good frame
  standing instead of truncating it. Failures are logged with their reason.
- **A second `grep -q` on a Windows program** was sitting in the success path - `screenshot.sh`
  drives `powershell.exe`. Same deadlock as D194, found by looking for the shape rather than
  the symptom.
- **The end record is looked for in both channels.** Kyty's records never reach the log.

### Two false completions on the way, both worth writing down

Reading the watched file unguarded declared every Kyty run **complete at zero seconds** - the
file is the resume state, so it still holds the *previous* run's `OBS|end` when a run starts.
The row said `complete` with a real record count, and both came from the run before. A detector
that succeeds instantly is worse than one that never fires.

And Kyty's screen is not its state: it presents at about **0.78 fps**, so sixty seconds after
the report read `OBS|end` the window was still drawing section 4 of 27. The screenshot was
right about what was on screen and wrong about what had happened. Its text channel works now,
so the screenshot is no longer load-bearing for it.

### The table work was half-finished twice

`verify.sh` caught it: `derive` reported "nothing here to derive from" and `imports` reported
**zero imports** - both about the module the same run had just built and verified nine
relations against. The switch had been given to the writer and not to the readers, and the
readers' default made every current-generation module invisible to this project's own tools.

`dynlib::detect` now reads the convention off the module, keyed on the high-range vendor tags
rather than `DT_STRTAB` so that a plain payload still has neither and `derive` still refuses
it. `verify: ok`, 24 gates. (D193)

Worth noting how close this came to being missed: I read a `verify.sh` result through `tail`
and took the exit code from the pipe rather than the script, so it reported success while the
last lines said `verify: FAILED`. The same mistake I made with `compat` earlier, and the reason
`verify.sh` prints a verdict line at all.

