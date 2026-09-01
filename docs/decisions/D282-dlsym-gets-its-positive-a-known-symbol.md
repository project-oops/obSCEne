# D282 - dlsym gets its positive: a known symbol through a valid handle


`060-module/dlsym-rejects-bad-handle` proved argument validation and nothing else - by principle 7
that is the weaker half, because an implementation that fails everything passes it. Added
`060-module/dlsym-resolves-known-symbol`: open `libScePad`, resolve `scePadOpen` through the returned
handle, and require `rc == 0` **and** `obs_address_is_callable(address)`. Non-null is not enough - a
placeholder is non-null too, and the callable test is what separates "answered an address" from
"answered a usable one".

No surface or import change: the check reuses `sceKernelDlsym`, already declared and already imported by
its negative sibling, so the five-step admission list collapses to steps 4-5 (provenance `OBS_FROM_ASSUMED`,
run under `make host`). It skips - never fails - where the platform resolves nothing by name or cannot load
`libScePad`, so a skip reads as "not asked". The host stub takes the skip (no run-time resolution there);
a hardware run is where it earns its keep, naming the address the console binds so orbistoun's `dlsym`
success path can be diffed value-exact, the same way the rejection path already is.

The sysmodule counterpart was **left out on purpose**: a positive `sceSysmoduleIsLoaded` needs a real
`SCE_SYSMODULE_*` id, and there is no citable one in `platform.h` (the grep is empty). Inventing an id to
make a positive check compile is exactly what principle 2 forbids - a wrong constant makes the call succeed
and answer about the wrong module silently. The negative (`sysmoduleIsLoaded(0)` → unloaded) stays the
honest half until an id arrives from a nameable source.

Verified: `make host` builds clean (-Werror) and runs; the new check emits its `try`/`res` pair and takes
the honest skip on the host. The full `make check` could not run this session - the sibling `selfish`
checkout is mid-rescaffold (`main` has no commits yet, an empty `Cargo.toml` staged), and the Rust `tool`
gate path-depends on it through `prosperous`. That block is entirely in the sibling and untouched by this
change; the C addition is complete and host-verified.

