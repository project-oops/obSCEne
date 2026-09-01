# The emulators earned their keep while the console was down


Three changes went in without hardware: the run-time census (D229), the generation probe moved
off linking (D230), and the twelve-library eboot (D228). All three were validated on emulators
first, and one of them was wrong.

**PS5PCEM found it.** The run-time census reported

```text
OBS|res|900-surface/kernel|fail||this library could not be loaded
```

from a process making calls into `libkernel` at the time - every library failed, and the
generation probe called an emulator that implements current-generation graphics "neither
generation". `obs_module_open` failing has two meanings and only one of them is a finding; the
control that separates them is D232.

**`bin/obscene check` passes with that bug present.** The host build links the census, so the
run-time path is never exercised there. No gate in this repository could have found it, and the
next thing that would have exercised it was the console.

Coverage of the change, after the fix:

| loader | previous generation | current generation | verdict |
|---|---|---|---|
| shadPS4 | resolves, linked | cannot be looked for | `4 (gnm)`, with the caveat stated |
| PS5PCEM | absent | cannot be looked for | neither established |

Both branches, both honest. PS5PCEM runs the twelve-library eboot to the end - `tally 73/4/5/439`,
28 sections, `OBS|end|write` - and its five failures are its own behaviour (mutex recursion,
`sceKernelIsStack`, memory-map queries), not ours.

### Two things that made this possible and were not free

`EBOOT_KIND` (D231). The eboot's `e_type` is the one byte a console and every emulator disagree
about, and it had made the shape being changed the only shape testable nowhere but on hardware.

And PS5PCEM's `module-info`, an independent parser, reads our eboot as `[A]..[L]` libraries and
`[B]..[M]` modules - **D217's numbering confirmed by something that is neither selfish nor
obSCEne.**

