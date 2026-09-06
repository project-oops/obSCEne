# D295 - Dynamic module resolution for non-essential libraries

**Status**: [decided]
**Date**: 2026-09-02
**Context**: Investigating native PS5 title launch aborts where processes exit before entry point execution.

---

## Context

On PlayStation 5 (Prospero), the system dynamic linker (`rtld` / `self_load_shared_object`) inspects all `DT_SCE_NEEDED` / `DT_NEEDED` entries declared in an executable's dynamic section during process creation. This module resolution takes place before jumping to the process entry point (`obscene_start`) or running `DT_INIT`.

If even a single library in `DT_NEEDED` fails to resolve on disk, `rtld` aborts process initialization with `val 2` (`ENOENT`). The process terminates immediately without executing a single instruction, preventing any diagnostics, logs, or reports from being emitted.

Inspection of `/system/common/lib/` on retail PS5 firmware revealed that certain libraries commonly present on PS4 or assumed by the probe—specifically `libScePosix.sprx` and `libSceVencCore.sprx`—do not exist in the standard PS5 application library path (their equivalents exist in other locations, such as `/system_ex/common_ex/lib/libScePosixForWebKit.sprx` and `/system/common/lib/libSceVenc.sprx`). Linking them statically forced `DT_NEEDED` entries that caused immediate launch aborts for native titles.

## Decision

1. **Remove non-essential modules from static platform imports**:
   - `libScePosix` and `libSceVencCore` are removed from `src/probe/imports.c`.
   - `symbols-no-census.txt` generated for `eboot` builds contains only the 12 core libraries confirmed to exist in `/system/common/lib/`.
2. **Pivot non-essential library checks to dynamic runtime resolution**:
   - Sections such as `017-posix` and `106-encoder` resolve their target functions dynamically via `obs_module_open()` and `obs_module_symbol()`.
   - If a library or symbol cannot be resolved within the process's current sandbox, the check records an honest `obs_skip("libScePosix is not available in current sandbox")` instead of halting the loader.
3. **Preserve host build verification**:
   - The host test harness (`OBSCENE_HOST_BUILD`) retains local libc implementations for verification in CI without requiring the console libraries.

## Consequences

* The native `obscene.eboot.elf` dynamic section only requires verified base libraries (`libkernel`, `libSceLibcInternal`, `libSceAudioOut`, `libSceGnmDriver`, `libSceKeyboard`, `libSceNet`, `libSceNetCtl`, `libScePad`, `libSceSysmodule`, `libSceUserService`, `libSceVideoOut`, `libSceVideoRecording`).
* Native titles boot cleanly past `rtld` directly into `obscene_start`.
* Non-essential modules can be probed conditionally across different firmware versions, jailbreak configurations, and privilege levels.

