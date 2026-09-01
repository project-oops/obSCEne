# A console answered the process-parameter experiment, and named the field


`src/crt.c` has carried an explicit open question since it was written:

> does a loader want this segment to *exist*, or to *contain* something? A zeroed structure
> separates those two, and whichever way it comes out is worth more than a better guess would be.

Launching from `/app0`, a console answered it and said which field it was reading:

```text
[KERNEL] kern_get_sdk_compiled_version: m=0 sz=80
[Syscore App] Error: failed to get both Prospero and PS4 SDK version of /app0/eboot.bin, ...ignored
```

`sz=80` is `OBS_PROC_PARAM_SIZE` to the byte, so the structure was found, at the right place, the
right size. `m=0` is the magic, which this file never wrote. **The size was right and the contents
were not** - which is exactly the distinction the zeroed version was built to draw.

The magic and entry count now come from the OpenOrbis PS4 ELF specification, which documents the
layout and records the same `0x50` size this file had already picked independently. Both kernel
messages are gone: `kern_get_sdk_compiled_version` no longer appears at all.

The SDK version stays zero. It is a claim about which SDK built this, there is no SDK here, and
the console's own message says that particular failure is ignored - so writing a number to silence
a warning that already says it does not matter would be inventing a value to no end.

### It was a real defect and it was not the launch failure

The process still dies. What changed is that a message the console printed about our file is gone,
which is worth having on its own: a build that a console complains about is a build with a known
defect, whether or not that defect is the one being chased.

```text
[Syscore App] new pid=0xd1
pid 209 (SceSysCore.elf), uid 0: exited on signal 12      SIGSYS, during exec
[Syscore App] An error occured while creating new process. error=0x80aa001a errno=106
```

Eliminated so far, each measured against a real package's eboot rather than reasoned about:
container generation, `e_type`, image base, headers-in-segment, import count (352 against a
one-import build - identical failure), `paid`, `ptype`, and now the process parameters.

What is left is the segment list: six against a real eboot's eleven. Missing are `PT_TLS`,
`PT_GNU_EH_FRAME`, and three vendor segments `selfish-elf` already names - `SCE_RELRO`,
`SCE_COMMENT`, `SCE_VERSION`. The `PHDRS` block declares exactly what exists, so each is absent by
construction, which is the same reason `PT_INTERP` was missing before D097.

