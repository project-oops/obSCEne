# D127 - obSCEne's display targets the previous generation's video-out interface. That is why it draws nothing on a current-generation loader


Status: derived - from a NID comparison against a loader that implements the current
interface.

PS5PCEM runs the suite to completion and shows no picture. obSCEne says why in its own
report, and the answer was in the first ten records:

```
OBS|display|opening|the display is being opened
OBS|display|absent|the display symbols are not present
```

`src/display.c` requires seven symbols before it will open. Five resolve under PS5PCEM -
the direct-memory calls all pass, `sceVideoOutOpen` returns a handle. Two do not, and the
NIDs say exactly why:

| what the display calls | NID | what PS5PCEM implements | NID |
|---|---|---|---|
| `sceVideoOutSubmitFlip` | `U46NwOiJpys` | `sceVideoOutSubmitFlip` | `U46NwOiJpys` |
| `sceVideoOutRegisterBuffers` | `w3BY+tAEiQY` | `sceVideoOutRegisterBuffers2` | `rKBUtgRrtbk` |
| `sceVideoOutSetBufferAttribute` | `i6-sR91Wt-4` | `sceVideoOutSetBufferAttribute2` | `PjS5uASwcV8` |

The current generation replaced two of the three with `2`-suffixed entry points and kept
the third. **obSCEne knows nothing about either variant**, and shadPS4 does not implement
them either - which is consistent, because shadPS4 emulates the previous generation and
that is the interface obSCEne has been written against.

So the display works on the one loader whose generation matches the code, and the project's
stated target is the other one.

### What this says about the rest of the suite

The census already censuses 87 current-generation graphics symbols, and `005-generation`
exists to decide which generation a platform is. Neither helped here, because the display
is not a check - it runs before the sections, its symbols are in `platform.h` rather than
the census, and nothing compares them against a generation.

The fix is to import both variants weakly and prefer the `2` form when it resolves, which
is what a title targeting both would do. Worth doing carefully: the argument shapes may
differ, and D008 applies - a wrong layout here writes into a framebuffer descriptor.

**This is obSCEne's gap and not the loader's**, and it was found by a loader being more
current than the probe rather than less.

