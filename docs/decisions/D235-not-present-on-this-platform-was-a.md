# D235 - "Not present on this platform" was a claim the harness could not support


A check whose symbol did not resolve reported:

```text
OBS|res|080-video/open|skip||the symbol is not present on this platform
```

That is a statement about the **platform**, made from an observation about the **loader**, and
on a console the two came apart badly.

A title is given far fewer libraries than it asks for. Of the eboot's twelve `DT_NEEDED`, five
were mapped; the other seven were silently not provided, and every check behind them said the
platform did not have those symbols. The same run's census, resolving the same libraries at run
time through `sceKernelLoadStartModule`, found `libScePad`, `libSceAudioOut`,
`libSceUserService`, `libSceGnmDriver`, `libSceNetCtl` and `libSceVideoOut` **all partly
present**. The symbols were there. Twenty-one checks reported otherwise, and the sentence read
as a finding about the console rather than about the build.

So the library is now asked before anything is claimed, and there are three outcomes where there
was one:

| | |
|---|---|
| the symbol is not present on this platform | genuinely absent, asked and answered |
| the symbol exists in its library but this build did not link it | the title was not given that library |
| the loader did not resolve this symbol, and this platform cannot be asked | no module resolution here (D232) |

Only the first is a finding about the platform. The second is a finding about *how a title is
loaded*, which is worth more - it is the difference between "the console cannot do this" and
"a title cannot reach this the way we asked for it".

### The fix was nearly worse than the fault

The first version answered the question by loading the library and resolving through it. That is
the wrong place to ask. A skip is the one path in the harness loop that is supposed to do
**nothing**, and putting a module load in it gave the inert path side effects - in a program
whose first principle is that a check announces before it acts.

It was caught on hardware within one sweep iteration: passing `105-record` loaded
`libSceVideoRecording`, which is one of the ten libraries that end the process, and a
thirteen-thousand-record run became three hundred. The census sweep found it because the sweep
rebuilds from source each iteration, so it picked the change up and the record count collapsed.

The skip now says only what was seen - `the loader did not resolve this symbol for this build` -
and whether the symbol exists is left to the census, which asks that question in the section
built for it, last, where loading modules is the point rather than a side effect.

### The same mistake, a third time

D232 was a census reporting `libkernel` absent while calling into it. D233 was a channel
reporting success into nothing. This is a check reporting a platform gap that was a link gap.
All three are the same shape: **an observation reported as the thing it was evidence for**, and
in every case the honest fix was to separate the two and say which one was seen.

`obs_address_is_callable` guards a call. It cannot answer why an address is null, and for three
sessions it was read as though it could.

