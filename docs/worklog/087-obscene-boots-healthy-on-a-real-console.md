# obSCEne boots healthy on a real console


```text
461   55  55  55  1   RUN   8018  OBSC00002   41.3/41.4  eboot.bin
```

and in the console's own kernel log, written by this program:

```text
obscene-min: the guest is running
```

No fatal signal, no exception, no `EXECFAIL`. The process is in state **RUN**, its two bundled
modules are in the loaded-library list beside the six system ones, and the GPU context came up
(`### GC is active`).

### What stood between executing and booting

The eboot had already loaded and executed (previous entry). Everything after that was the
platform's startup contract, and there were five parts to it, each found by one build and one
launch because the loader named the field: the process parameters and their three structures
(D219), the bundled modules a title must have (D220), and what one of those has to look like -
header placement, ABI version, `DT_SCE_ORIGINAL_FILENAME`, and the procedure-linkage tables
(D222, selfish D078/D079).

### The two surprises

**A stopped process holds a title id forever.** After one crash, `checkExistingApp` began
failing and every install fetched the package header and stopped - one range request, no body,
which reads exactly like a malformed package. `ps` showed the culprit: a crashed `eboot.bin` in
state `STOP` that `kill` cannot touch, held by the crash-reporting machinery. Two builds were
spent looking at a package that was fine. `CONTENT_ID` now carries the title id with it, so a
fresh one routes around it without a reboot. (D223)

**The loader's message can come from a layer above the failure.** `Unsupported ELF e_type ...
fe18` on our bundled library, where `0xfe18` is exactly what a real one carries - measured, from
a real package. The actual failure was the header check two lines above. This is the second time
in two sessions (D075 was the first), and the rule that comes out of it is worth keeping: read
the whole log, in order, before believing the line that names something.

### What this is not yet

The minimal build. One import, one library, and a spin loop that proves guest instructions are
executing. The full probe - four hundred imports across twelve libraries - has not been through
this path yet, and the `sce-module` guard exists because that is when the stub names could start
mattering.

`/data` is not writable from inside a game sandbox, so the file sink the full probe uses will
need somewhere else on a console; the kernel log is what works today.

### Console cost

Zero crashes and zero reboots across this whole stretch. Every failure was a refused load or a
killed process, and the jailbreak survived all of them.

