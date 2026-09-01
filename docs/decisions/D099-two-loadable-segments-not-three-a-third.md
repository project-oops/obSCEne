# D099 - Two loadable segments, not three. A third was silently not mapped, and that was the crash


Status: derived - the control build shows the correct shape and a loader demonstrated the
cost.

`link/module.ld` declared three `PT_LOAD` segments - read-execute, read-only, read-write -
on a note that "vendor modules have exactly text, rodata and data". The control build (D097)
disagrees: an OpenOrbis module has **two**, read-execute and read-write, with its read-only
data inside the first. The read-only region in the middle is `PT_SCE_RELRO`, which is not a
`PT_LOAD` at all.

fpPS4 maps segments by kind and listed only two of our three:

```
.CODE:0000000800000000..0000000800029000
.DATA:0000000800038000..0000000800040000
```

The middle segment is absent, with no error. So **every byte of `.rodata` was unmapped** -
the section tables, the check tables, and every string in the report. `obs_run_all` walks
`obs_sections[]` as its first act, which is why the process died with an access violation
immediately after `Entry:` and before a single import call.

Putting `.rodata` in the text segment fixes it: fpPS4 now maps `.CODE` across
`0x0..0x36000`, runs, and reaches its first import. shadPS4 is unchanged - runs to the end,
138 attempts and 138 results, the only diff being two timing-dependent `sleep-fidelity`
measurements.

### Why this took so long to find

Every earlier reading of the fpPS4 failure was wrong, and each was wrong in a way that
looked reasonable:

- "the module is malformed" - it is not; minimal modules run there;
- "it is missing program headers" - `PT_INTERP` was genuinely missing and adding it changed
  a silent exit into a *reported* fault, which was useful and was not the cause;
- "it is a problem of scale" - a minimal module survived and the full one did not, which is
  true and had the causation backwards. A minimal module has no separate read-only segment
  to lose.

What found it was reading the loader's own segment map and **counting the lines**. Two where
there should have been three. That output had been on screen since the first fpPS4 run.

