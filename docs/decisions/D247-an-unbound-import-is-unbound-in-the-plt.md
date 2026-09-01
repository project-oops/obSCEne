# D247 - An unbound import is unbound in the PLT too, so the gating is right


*status: measured*

Everything this program uses to decide "is this symbol available" reads a **data slot**.
`check->address` is a relocated entry in a `static const` table - an `R_X86_64_64` against an
undefined symbol. `&scePadClose` written in code is a GOT load, `R_GLOB_DAT`. A *call* goes
somewhere else entirely: the PLT, `R_JUMP_SLOT`, of which this module has 198.

Nothing established that a loader fills all three the same way, and if it bound the PLT and
left the data slots null then fourteen checks were skipping for no reason and the entire gating
mechanism of this program was reading the wrong thing.

So one call was made with the guard deliberately omitted - `scePadClose` on an invalid handle,
from the failure side, with its address slot null. **It did not return.**

```text
OBS|try|061-imports/unbound-import-is-callable|libkernel|sceKernelClose
                                                     (no res)
```

The three relocation forms are unfilled together. `obs_address_is_callable` is reading the
right thing, and there is no cheap repair hiding behind a wrong test.

**Two properties made this affordable to ask.** The `try` record is written before the call, so
a death names one check and nothing else - the run's other 527 results are attributable. And
the resume record gets past it, which is what let the console be handed the same build again
without a person deciding anything.

It is now behind `CALL_UNBOUND=1`, off by default. Not deleted: it is the only thing that would
notice a firmware where the answer changed. But leaving it on costs two runs every time, since
D181 retries a check that died before skipping it, and the answer is known.

**This is what deliberately breaking a rule looks like when it is worth it.** D058 forbids
exactly this call. The reason D058 exists - a run lost to an unguarded jump - was bounded here
by announce-before-attempt and by resume, and the thing bought was the elimination of a whole
class of repair. Both halves have to be true to break it; neither on its own is enough.

