# The full probe takes the console down


The minimal build boots healthy (previous entry). The full one does not, and this is the first
time it has been through the install-and-launch path at all.

What was observed, and it is thin because the console stopped answering:

```text
[Syscore App] createApp OBSC00003
[Syscore App] processSpawn(pid, 0xa018, ...) appParam: appType = 0, titleId = OBSC00003
[Syscene App] new pid=0x1dc  attributeExe= 0
[Syscore App] Ready to exec
[Syscore App] wait for NOTE_EXIT|NOTE_EXEC
```

and then nothing. `ps` showed no `eboot.bin`; a second launch attempt found `shsrv` gone, and
`hw check` reports every service down. The jailbreak needs re-running before anything else can
be measured.

**No conclusion is available about *why*.** The log ends before `EXEC` and the crash-report
block never arrived, so it is not known whether the eboot loaded, whether it ran, or whether it
was the loading of 35,518 imports across 352 libraries that did it. That is the whole reason
`pkg-min` exists, and the Makefile comment beside it says so: an eleven-megabyte filesystem and
four hundred imports has too many possible causes to reason about.

### What is worth doing before the next attempt

The minimal build is the one that boots, and the distance between it and the full one is large
and unbisected: four imports against 35,518, one library against 352, one section against
thirty-four. The next step is not another full launch - it is something between the two.

`EXCLUDE` already exists for exactly this and names checks or whole sections to leave unrun.
A build with the census sections excluded, or with `BULK` provably off (it is, on `HARDWARE=1`,
by D179's compile error) and only `base` running, is the middle the bisect needs.

Also unresolved: `/data` is not writable from inside a game sandbox, so the report sink the full
probe uses has nowhere to write on a console. The kernel log is the only channel proven to work,
and the minimal build reached it through `sceKernelDebugOutText`.

