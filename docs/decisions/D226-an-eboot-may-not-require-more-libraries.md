# D226 - An eboot may not require more libraries than a title can load


`DT_NEEDED` is an assertion - *this module requires that one* - and a system loader acts on it
**before a single instruction of ours runs**. The census names 352 libraries, so the eboot
required 352, and the console went down with no crash report at all: it died inside the loader,
before `EXEC`, so there was nothing to report from.

A title gets six. Measured, from the dynamic-library list of the minimal run that worked:
`libkernel`, `libSceLibcInternal`, `libSceSysmodule`, `libSceDiscMap`, `libSceNet`, `libSceIpmi`.

### The existing guard is at the wrong layer, and that is the whole lesson

Every platform symbol here is `OBS_WEAK` and address-checked before it is called. That is why
the probe survives a **missing function** - the guard runs, the check reports absent, the run
continues. It can do nothing about a **missing library**, because linkage happens before the
program exists. There is no instruction of ours to guard with.

Stated as the project's own first principle: **requiring a library and probing it are opposite
claims.** `DT_NEEDED` says "I need this"; a check asks "is this here?". Making the first claim
352 times means the question "does `libSceAbstractDailymotion` exist for a game process?" is
answered by the console dying rather than by a `try` with no `res`. Announce before attempting,
applied to linking.

### What was done, and what it is not

`EBOOT_LIBS` (default 16) fails the **eboot** build when it would require more, with
`EBOOT_LIBS=any` to opt out deliberately. `module` and `payload` are not gated: they run under a
homebrew loader with different privileges and under emulators that stub everything, where
requiring the whole census is exactly what is wanted.

This does **not** claim to know which libraries a title may load, and it must not pretend to.
`data/obscene-report.txt` says all 352 are "present", which is true of whatever loader produced
that report and is not evidence about a game process - using it as the gate would be the same
context error in a new place. The gate refuses to let a build *silently* require hundreds. That
is the difference between a failed build and an hour of re-running a jailbreak.

### The real fix, which is larger

The eboot should require only what the harness needs to run and report, and probe everything
else at run time through `sceKernelLoadStartModule` inside a guarded check that announces first.
Then a library a title cannot load is a finding with a record, which is what this program is
for. That is a design change rather than a gate, and it is the next substantial piece of work.

