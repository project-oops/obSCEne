# D140 - A resolved address proves nothing on a loader that stubs what it cannot resolve. The display learned this the hard way


Status: derived - broke a working loader, and the loader said why.

D111 had the display take whichever video-out pair resolved, preferring the current
generation's. That broke fpPS4, which had been drawing the full report: the run stopped at
`display|opening` with four records.

fpPS4 said why in its own log:

```
nop nid:libSceVideoOut:3E34B9B804B0715F:sceVideoOutSetBufferAttribute2
```

**It installs a logging stub for every import it cannot resolve**, so the current
generation's entry point has a perfectly good non-null address there and does nothing. The
check asked "did this resolve" and got yes from a function that does not exist.

That is the thesis of `007-responsive` and `910-bulk` - presence is not behaviour - arriving
uninvited in code that had no business assuming otherwise. This program measures that
distinction in two sections and then depended on the opposite in a third.

### Preferring the older form settles it

The ambiguity only ever runs one way:

| | plain | `2` | chooses |
|---|---|---|---|
| fpPS4 | real | stub | plain, correctly |
| shadPS4 | real | absent | plain |
| PS5PCEM | absent | real | `2` |
| SharpEMU | real | real | plain, which it implements |

A platform that genuinely has the older form works with it, and a current-generation
platform does not offer it at all - so there is no tie to lose by taking it. The newer form
is reached only when the older is genuinely missing, which is the one case a stub cannot
manufacture.

### The other half of D111 was also wrong, and for a related reason

That entry called `sceUserServiceInitialize` before asking for a user, because a strict
loader refused an uninitialised service. Doing it unconditionally hung fpPS4 inside the
initialise. The order is now: **ask, and initialise only if refused** - which leaves every
platform that already answers untouched, and makes the smaller claim ("this needed
initialising") rather than the larger one.

Both mistakes have the same shape: a change made to satisfy one loader, applied
unconditionally, breaking another. The fix in both cases was to make the new behaviour
conditional on the old one having failed.

