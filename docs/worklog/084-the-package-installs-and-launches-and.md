# The package installs and launches, and the eboot shut the console down


The install and launch path is solved. What remained was the console's own state and one
build-time default, and the second is the more useful surprise.

Four things had to be true before `launch OBSC00001` got past the launcher, and each was found
by moving the error code forward one step at a time (`0x80020000 | errno`, so `...0002` is
`ENOENT` and `...0016` is `EINVAL`; the `sceSystemServiceLaunchApp: Resource temporarily
unavailable` string mislabels every one of them):

- The nested pfs had to mount. Four `selfish` fixes, each measured against real packages: the
  outer inodes carry `0x0C` (this crate wrote `0x10`), the inner ones `0x10` (it wrote nothing),
  the inner superblock's embedded inode had to be filled at all, and `pfs_image.dat` needed the
  compressed bit with `size_compressed` holding the *inner* length.
- **`kstuff` has to be loaded** or the fake licence is refused - `sceRifManagerStoreRif
  0x80870003` into an `AppPromoter` error into `preLaunchCheck 0x80a40086`. It is first in
  `/data/pldmgr/autoload.txt`, but a plugged-in USB with its own payloads takes precedence
  (`SCAN_USB_PAYLOADS=1`) and that chain omits it.
- **No other app may hold the display.** With everything else right the launcher gets through
  login, budget and directory checks and then fails `checkExistingApp: 0x80940010` while the
  resource dump shows another app `RUNNING` with `vcn[20] scanin[2K]`. One big app at a time.

### The surprise: `make pkg` builds the suite that CLAUDE.md warns about

On the one launch that the launcher accepted, the console **shut down** and showed
`CE-108262-9` on reboot - "an error occurred while reading the system software or application
data". The package was not the cause: it is self-consistent (13/13 entry digests, both manifest
digests, every size field agreeing) and its entry layout is byte-identical to a real package's.

The cause is a default. `make pkg` builds with `EXCLUDE` empty and `SERVE` unset, so the eboot
takes the `#else` branch in `start.c`: `obs_run_all()` over all 146 checks with **no exclusions**,
then `obs_screen_present()`. This file already says building without the exclusion list "has
twice handed a later step a module that walked into a known crash", and this is the third - the
first time in a foreground app that owns the display, where the cost is the whole console rather
than one process.

There is no `excludes.txt` on this machine to apply, and the sweep that generates one wants a
loader that survives the run - which is the thing being established. So the eboot is now built
`SERVE=1`, which is the mechanism `start.c` describes for exactly this: listen, run nothing, and
let a driver ask for the suite over the socket, so "a crash during it costs one session and a
reconnect rather than the whole endpoint".

`start.c` also announces itself on the kernel log now (`sceKernelDebugOutText`, guarded like every
platform call, import registered in `imports.c`) before the entry, the net check, the HUD and the
serve loop. Principle 1 applied to the boot sequence: the report channel does not survive a fault
and the log does. **None of these breadcrumbs has printed yet** - the eboot has not run since they
were added, so they are the instrument for the next launch and not evidence of anything.

The better instrument was already here and was overlooked for three crashes: `sink.c` writes the
report unbuffered, one syscall per record, to `/data/obscene-report.txt` and three fallbacks. That
file outlives the crash *and* the reboot, so whether the eboot ever ran is answerable after the
fact without watching the screen - `scripts/oops-recover.sh` reads it and looks for a crash dump.

