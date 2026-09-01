# D101 - obSCEne runs on a second loader. `DT_NEEDED` was the whole of what was missing


Status: hardware-adjacent - observed, on a loader, from a build that reproduces.

`mkmodule` now emits one `DT_NEEDED` per imported library, naming it `<library>.prx`, and
adds that string to the table beside the bare name. Sixteen tags, sixteen strings, 66 tags
to 82. fpPS4 reads every one:

```
DT_NEEDED               :libkernel.prx
DT_NEEDED               :libScePosix.prx
DT_NEEDED               :libSceLibcInternal.prx
...
```

and then loads its implementations, and obSCEne reports:

```
[TTY]:OBS|meta|1|25|146
[TTY]:OBS|display|ready|1920x1080 framebuffer
[TTY]:OBS|res|000-boot/write-returns-count|pass|||derived
[TTY]:OBS|res|000-boot/write-rejects-bad-fd|pass|0xffffffff80020016||derived
[TTY]:OBS|responsive|libSceLibcInternal|strlen|responds|0x6
```

**Every check in `000-boot` passes**, the generation probe answers, `007-responsive` finds
`strlen` and `strcmp` responding, and the display opens at 1920x1080. From a loader that
produced nothing at all for the entire life of this project.

### The three faults, in the order they had to be fixed

Each was invisible behind the one before it, which is why this took four sessions of wrong
answers rather than one:

1. **`-g` instead of `-e`** - the runner never handed fpPS4 the module (D101, above). Every
   "fpPS4 produces no records" result before this was measuring nothing.
2. **Three `PT_LOAD` segments** - the middle one was silently not mapped, so `.rodata` was
   absent and the harness died reading its own tables (D099).
3. **No `DT_NEEDED`** - the library objects existed and stayed empty, so every import
   resolved to a logging stub (D100).

None of them is the ELF-shape work the control experiment's header table pointed at, and
`PT_INTERP` - the one header from that list that was added - fixed nothing on its own. It is
kept because it turned a silent exit into a reported fault, which is what made (2) findable.

### What is next on this loader

The run stops after 24 records, in `007-responsive/libc`, immediately after fpPS4 installs a
stub for `strspn` - the one function it does not implement. It does not crash; it hangs.
That is a finding about fpPS4's stub path and the exclusion list is the way past it, exactly
as with shadPS4's known crashes.

