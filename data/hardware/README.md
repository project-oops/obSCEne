# `data/hardware/` - what a real console answered

Reports from runs on real hardware, kept here rather than in `reports/` because **`reports/` is
gitignored**. Every emulator run this project has ever done is reproducible from source by
anyone; a hardware run is not. If these are not committed they do not exist, and a finding
nobody can cite is an opinion.

## What a consumer needs from these

Orbistoun implements what these files measure. It has to be able to say *"this is how that was
determined"* and have somebody else reach the same answer, so each file carries:

- **the artefact** - the exact build flags, so the same eboot can be produced again;
- **the console state** - which payloads were loaded, because a partly-loaded jailbreak silently
  changes what a title is allowed to do (see `docs/HARDWARE.md`);
- **the records** - `docs/OUTPUT.md`'s format, unedited, so a claim can be traced to a line.

A finding in `docs/HARDWARE.md` cites a record here. A record here without a run header is not
evidence and should be deleted rather than trusted.

## What is deliberately not here

The raw kernel log. It is thirteen thousand lines of console-wide noise per run, most of it
nothing to do with this program, and the parts that matter are quoted in `docs/HARDWARE.md`
where they are being reasoned about. The `OBS|` records are the measurement; the log is how they
got out.

## The files

| | |
|---|---|
| `ps5-full.txt` | the first complete suite this project ran on a console - 14,665 records, 521 checks, the census reaching 10,243 symbols |
| `ps5-imports.txt` | the run that separated **our** defect from the platform's. Adds `import` records: per symbol, whether the loader bound the import *and* whether a run-time lookup finds the same name in the same library |
| `crashers.txt` | the ten libraries whose load ends the process. Not a report - a **finding**, in machine-readable form, consumed by `scripts/sweep-build.sh` and `scripts/oops-rebuild-pkg.sh` |

`crashers.txt` is here rather than in a build directory for the reason this whole directory
exists: it cost an iterative sweep against a real console to obtain, one crash and one reboot
per entry, and a build directory does not survive `rm -rf` or reach a second machine. (D236)

**Emptying it and re-running the sweep is a valid experiment**, and the right one when the
firmware changes - `SEED=0`, or `EXCLUDE=` in the environment. Exclusions are how you see what
is behind a crash, never how you stop finding them.
