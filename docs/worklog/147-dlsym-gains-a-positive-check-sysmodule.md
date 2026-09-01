# 2026-09-01 - dlsym gains a positive check; sysmodule positive left out (no citable id)


Coverage polish toward principle 7 (positive > negative). `060-module` had only the failure side of
`sceKernelDlsym` (`dlsym-rejects-bad-handle`). Added `dlsym-resolves-known-symbol`: opens `libScePad`,
resolves `scePadOpen` through a valid handle, requires `rc == 0` and `obs_address_is_callable(address)` -
callable, not merely non-null. Reuses the already-declared/imported `sceKernelDlsym`, so no surface,
import or census change; skips honestly where the platform resolves nothing by name. A positive
`sceSysmoduleIsLoaded` was deliberately NOT added - no citable `SCE_SYSMODULE_*` id exists in
`platform.h`, and inventing one violates "nothing is invented" (D282). `make host` builds clean (-Werror)
and runs; the new check takes the honest skip on the host stub. `make check`'s tool-based gates could not
run: sibling `selfish` is mid-rescaffold (empty `Cargo.toml`, `main` with no commits) and the Rust tool
path-depends on it - a blocker entirely outside this repo and this change.

