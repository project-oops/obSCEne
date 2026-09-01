# 13. Getting the report off the machine


Raised by the user, and it is the right question to ask before hardware rather than after.

### What exists now

Five channels, and every one of them is *live*:

| | |
|---|---|
| `sceKernelWrite(1, ...)` | the one that works under shadPS4 |
| `puts` | tried when the first is absent or refuses |
| POSIX `write` | tried after that |
| `putchar` | last resort, a byte at a time |
| the drawn screen | independent of all four, for when none of them work |

`src/probe/runtime.c` picks the first that moves a byte and sticks with it. So "is there hardware
out?" is answered - there is, four ways - and the gap is the sixth channel:

### What is missing: a file

**Nothing writes the report to disk.** On hardware that is the channel most likely to be
the only one available, and it is the one a person can retrieve afterwards. A drawn report
has to be photographed and transcribed; a file can be diffed against the last run, which is
what `tool/diff` exists for and cannot currently be pointed at real hardware's output.

The pieces are already imported - `sceKernelOpen`, `sceKernelWrite`, `sceKernelClose` are
all in `040-file` - so this is a sink in `runtime.c` and a decision about where to put it,
not new surface. Two things need settling: which path is writable to an unsigned module,
and whether to write incrementally (survives a crash, slow) or at the end (fast, loses
everything if a check kills the process). **Incrementally**, on principle 1 - a report that
stops mid-record names the call that ended the run, and that is the most valuable line in
the file.

`blargg`'s test ROMs and `pspautotests` are the right comparison and both settled the same
way: emit continuously through whatever the platform has, and make the last line meaningful.

### It is not what stops the other loaders

Worth stating plainly, because it is the obvious hypothesis and the evidence is against it.
Four of the five loaders fail *before any guest code runs*, so no output channel could help:

| | how far it gets | could output help? |
|---|---|---|
| SharpEMU | resolves 0 imports, stalls on the entry instruction | no - nothing runs |
| craziiEmu | same loader, same failure | no |
| fpPS4 | prints `Entry:` and the process ends | no |
| Kyty | **runs**, presents 3084 frames at 60fps, draws nothing | **yes, plausibly** |
| shadPS4 | 835 records through `sceKernelWrite` | already works |

Kyty is the one case where this is a live hypothesis, and it is already half-answered: its
`sceKernelWrite` refuses standard output by design, and `--printf-direction File
--printf-output-file` gets records out of it. A file sink inside the module would be a
second, independent route to the same place.
