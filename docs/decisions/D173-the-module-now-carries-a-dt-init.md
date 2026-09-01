# D173 - The module now carries a `DT_INIT`, because a loader may call one and ours pointed at the ELF header


obSCEne's module had no `DT_INIT` in 1,426 dynamic tags. It is linked with a stock `ld` and a
linker script, and neither has any reason to emit one.

Kyty calls it, and does not check it exists:

```cpp
// RuntimeLinker.cpp:1041 — StartModule
return run_ini_fini(program->dynamic_info->init_vaddr + program->base_vaddr, …);

// RuntimeLinker.cpp:93 — no guard
return reinterpret_cast<module_ini_fini_func_t>(addr)(args, argp, func);
```

With no tag, `init_vaddr` is zero, so Kyty calls `base_vaddr + 0` and executes the ELF header -
`\x7fELF` as machine code. That is the access violation at `base + 2` this module produced
there, and the reason obSCEne has never run on Kyty.

### Every other loader declines to call it, and each says why

| loader | calls it for an executable | guards absent |
|---|---|---|
| shadPS4 | no - `Execute()` goes straight to `RunMainEntry` | n/a |
| fpPS4 | no - the call site is commented out (`ps4_elf.pas:2814`) | only `-1` |
| PS5PCEM | no - deliberately | yes, must resolve to executable memory |
| Kyty | **yes** | **no** |
| orbistoun | not yet; parses them (their D235) | yes, with a test |

PS5PCEM's reason is the interesting one: *"The executable's PS5 CRT entry calls its own
`DT_INIT` routine. Calling that routine here as well runs global constructors twice."*

**So the console's own CRT is reported to call it.** That is the argument for emitting one -
not to satisfy Kyty, but because a module without it is a fault this project would otherwise
meet first on hardware, where an iteration costs a manual copy rather than a rebuild.

### Empty, and deliberately

`obs_module_init` returns zero and does nothing. The suite runs from the entry point, which is
what every working loader calls; an initialiser that did work as well would do it twice
wherever both are called. This exists so a loader that calls `DT_INIT` reaches a `ret` rather
than the ELF header.

### Omitted rather than zeroed

`mkmodule` looks the symbol up in `.symtab` and emits no tag when it is absent. **A `DT_INIT`
of zero is worse than none**, because it is exactly what a loader that does not check will
call - the fault this is fixing, reintroduced by the fix.

### Measured, not assumed

| | before | after |
|---|---|---|
| Kyty | access violation at `base + 2` | loads, runs `DT_INIT`, reaches `Execute: Main`, closes cleanly |
| shadPS4 | 33,597 records, ended | 33,599 records, ended |
| PS5PCEM | 36,434 records, ended | 36,435 records, ended |

Kyty still emits no report, which is a separate question and may finally be what D080
describes - but D080's reasoning could never have been tested before, because the module never
reached the point where an output channel mattered.

