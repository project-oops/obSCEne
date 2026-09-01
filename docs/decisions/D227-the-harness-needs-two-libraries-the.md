# D227 - The harness needs two libraries; the other 350 are measurements


Measured rather than estimated, by taking every `sce*` name the harness's own files call -
`runtime.c`, `report.c`, `harness.c`, `status.c`, `sink*.c`, `net*.c`, `start.c`, `crt.c`,
`registry.c` - and looking each up in the manifest:

```text
23 symbols -> libkernel, libSceNet
```

Two. Both are among the six a title is given. **The eboot requires 352** (D226), so 350 of them
are linked for no reason other than that linking is how this program has always asked whether a
symbol is there.

That number is what makes the fix cheap and the flaw narrow. It is not that obSCEne probes too
much - probing 352 libraries is the entire point of a conformance probe, and nothing about that
should change. It is that **the import list and the probe list are the same list**, so a
question is asked in the form of a demand.

Under the loaders this was built for that distinction did not exist: `elfldr` loads what it is
told, and an emulator stubs every import, so a `DT_NEEDED` that cannot be satisfied is not a
category of failure either of them has. The eboot shape is where the two come apart, and it came
apart at the worst possible moment - before the program exists, where its own guards cannot run.

So the split is: **require the two, probe the three hundred and fifty.** A probe target moves to
`sceKernelLoadStartModule` inside a check that announces first, and a library a title cannot
load stops being a dead console and becomes a `res` line saying so. That is strictly more
information than today, on every loader, which is the test of whether a fix is the right one.

