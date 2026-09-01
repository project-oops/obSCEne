# D186 - The module version is per library, and declaring one value for all of them was why nothing appeared on screen


`MODULE_VERSION_MAJOR/MINOR` were constants, `1.1`, for every library a module imports from.
The comment beside `LIB_VERSION` already stated the hazard exactly:

> A loader builds its lookup key from the version *this module declares* and matches it
> against the version the library was registered with, so declaring zero against a library
> registered as version one does not match - and every symbol from it silently fails to
> resolve.

It was right, and it was applied to one value shared by all 352 libraries.

### The question that found it

*"How do commercial games work on this loader when our homebrew does not? That sounds
backwards."* It is backwards, and the answer is that obSCEne was taking a path commercial
titles never take.

Kyty splits exactly two libraries by generation, and does it two different ways:

| library | previous generation | current generation |
|---|---|---|
| `GraphicsDriver` | name `GraphicsDriver`, v1.1 | **different name** `Graphics5`, v1.1 |
| `VideoOut` | name `VideoOut`, **v0.0** - twelve functions | same name, **v1.1** - three |

A different *name* needs nothing here; it resolves or it does not. `VideoOut` is the only case
where the **module version** is the whole distinction, and it is the library that decides
whether anything is drawn: the previous-generation set holds `sceVideoOutSubmitFlip` and the
current-generation set has no way to present a frame at all.

So a commercial previous-generation title declares `0.0`, binds twelve functions, and draws.
obSCEne declared `1.1` whatever it was built as, landed on the three-function set, registered
a framebuffer it had no way to submit, and drew a black window. **Nothing was backwards; we
were the only thing asking for a version almost nothing is registered under.**

### Measured

`tag::module_version(library, generation)` now returns `(0, 0)` for `libSceVideoOut` at
generation 4 and `(1, 1)` otherwise. On Kyty, at `GEN=4`:

| | before | after |
|---|---|---|
| symbols bound to real code | 305 | **509** |
| `sceVideoOutRegisterBuffers` | 0 | bound, and **called** |
| `sceVideoOutSetBufferAttribute` | 0 | bound, and **called** |
| `sceVideoOutSubmitFlip` | 0 | bound, and **called** |
| suite | died at `puts` | **36,524 records, complete** |

### What is still not right

The window presents frames and stays black. That is now inside the loader's renderer rather
than in what this module declares, and it is where this stops being obSCEne's problem.

The table has **one entry**, from **one** emulator's registration, which is the weakest kind of
evidence this project accepts. Every other library defaults to `1.1` because nothing here
contradicts it - not because it has been checked. `IMPLEMENTATIONS`, and hardware settles it.

Status: **assumed**.

