# Why the full probe took the console down, and the guard that was missing


Measured offline while the console was being re-jailbroken, which is the right place for it:

```text
$ selfish-container --example tag_diff -- build/eboot.bin
   import libs 352, needed modules 352, DT_NEEDED 352
```

**352 `DT_NEEDED`.** A system loader finds and loads every one of them before a single
instruction of ours runs. A title gets six, measured from the dynamic-library list of the
minimal run that worked. That is why there was no crash report: it died inside the loader,
before `EXEC`, so there was nothing left to report from. (D226)

### The guard existed and was at the wrong layer

Every platform symbol is `OBS_WEAK` and address-checked before it is called, and that is exactly
why the probe survives a missing *function*. It cannot survive a missing *library*, because
linkage happens before the program exists. Nothing had noticed that the whole guarding strategy
has a floor under it.

`EBOOT_LIBS` now fails the eboot build rather than the console. It does not claim to know what a
title can load - `data/obscene-report.txt` says all 352 are "present", which is true of whatever
loader wrote it and is not evidence about a game process. The gate refuses to *silently* require
hundreds, and that is all it should do.

### Two hypotheses killed offline before spending hardware on them

- **The identifier encoding at scale.** 352 libraries needs two characters where one used to do.
  It rolls over correctly: `#+#-` is library 62 module 63, `#-#BA` is 63 and 64, and 352 distinct
  library ids appear across 35,519 symbols. Not the cause.
- **`EXCLUDE` as the bisect.** It stops checks *running*; `symbols.txt` comes from the census
  walk, so the eboot links all 352 regardless. It cannot bisect a load-time failure, which is
  what this was. Worth knowing before a jailbreak window was spent on it.

### And the report had nowhere to go anyway

Every output channel wrote to a descriptor, and a title has no parent holding one.
`sceKernelWrite` to standard output returns the byte count and discards - the undetectable
success the file's own comment warned about, arriving from an unexpected direction. The report
now goes to the system log first. (D225)

So a run that dies is now legible, and a build that would die does not get made.

