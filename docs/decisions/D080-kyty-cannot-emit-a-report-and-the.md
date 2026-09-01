# D080 - Kyty cannot emit a report, and the reason is structural rather than a missing flag


Status: closed as not worth doing, with evidence.

BACKLOG §12 named a second reporting loader as the prerequisite for consensus. Kyty was the
obvious candidate - it loads the module, relocates it, and since the generation marker was
corrected it reaches `Execute: Main` and opens a window. It emits no records, and the cause
is two independent things:

**Its `sceKernelWrite` refuses standard output by design.** `FileSystem::KernelWrite`
requires a descriptor at or above its own minimum and then looks up an opened file, so
writing to descriptor 1 returns `EPERM`. Not a bug - it is a file-system call in that
implementation, and console output is somebody else's problem there.

**And the fallback cannot resolve.** `puts` would work: Kyty routes it through the same
printf machinery that `--printf-direction File` captures, so it would land in a file this
project can read. But Kyty registers it under the library `LibcInternal` and this module
imports it from `libSceLibcInternal`, so it is one of the 238 imports Kyty leaves
unresolved.

**Closing it rather than fixing it.** The fix is to import the C library under the name one
loader expects, which would be a per-loader accommodation in the module's import table -
the category the README now names explicitly and the one most likely to break the loader
that works. Three accommodations already exist and each is argued; a fourth that changes
what the module *asks the platform for* is a different and worse kind.

**Kyty stays valuable for the thing it is uniquely good at.** It names every import it
cannot resolve, which shadPS4 never does because it stubs everything, and that list is what
`obscene-tool unresolved` turns into named missing functions - **252** against the current
module, including twelve `sceAgc*` entry points.

So consensus runs on the host and shadPS4 for now. The host is a real implementation of the
POSIX and C library surface, which makes it a better second opinion than its name suggests,
and craziiEmu remains the candidate for a third once it resolves imports at all.

