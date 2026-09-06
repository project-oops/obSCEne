# D296 - Four-tier privilege model and module categorization

**Status**: [decided]
**Date**: 2026-09-02
**Context**: Expanding probe coverage, understanding sandbox boundaries, and generating privileged homebrew binaries.

---

## Context

On PlayStation 5, process execution is compartmentalized by sandbox boundaries and authorization identifiers (`paid` in the SELF header). Libraries are stored in distinct directories with different access rules, and certain modules can only be loaded through specific IPC services or under elevated privileges:

1. Standard game titles (`appCategory: "gd"`) can only access `/system/common/lib/*.sprx` and their own `/app0/sce_module/*.prx`.
2. Certain common services require explicit on-demand activation through `sceSysmoduleLoadModule(id)` IPC via `SceSysCore`.
3. Extended system applications (such as WebKit and shell tools) can access `/system_ex/common_ex/lib/*.sprx` when granted system app credentials.
4. Core system utilities and daemons execute with root/kernel privileges and access `/system/priv/lib/*.sprx`.

Previously, neither SELFish nor obSCEne had a structured way to specify or probe these authorization tiers. SELFish defaulted all binaries to a hardcoded title `paid`, and module loading failures could not distinguish between a missing library and a privilege denial.

## Decision

1. **Define a Four-Tier Privilege Model**:
   - **`app`**: Standard title sandbox (`/system/common/lib/`, `/app0/sce_module/`, `appCategory: "gd"`, default `paid` `0x3800000000000000`).
   - **`sysmodule`**: On-demand system module loaded dynamically through `sceSysmoduleLoadModule(id)`.
   - **`system`**: Extended system application (`/system_ex/common_ex/lib/`, `appCategory: "system_app"`, `paid` `0x3800000000000001`).
   - **`root`**: Root / kernel execution privilege (`/system/priv/lib/`, `paid` `0x8000000000000001`).

2. **Add Privilege Flag to Tooling**:
   - `selfish-container` provides `Privilege` enum and `build_with_privilege()`.
   - `selfish-cli` adds `--privilege <app|sysmodule|system|root>` to `wrap` and `native`. Setting `system` or `root` automatically assigns `appCategory: "system_app"`.
   - `obscene-tool mkself` accepts `--privilege <app|sysmodule|system|root>`.
   - The build pipeline (`Makefile`, `scripts/build-native.sh`) exposes `PRIVILEGE ?= app` for compiling and laying out titles at different privilege tiers.

3. **Report Module Privilege Tiers**:
   - `obs_module_open_tier()` tracks which search path or loader mechanism provided a library.
   - Probing emits structured `OBS|modtier|<library>|<status>|<tier>|<detail>` records to build a knowledge base of module restrictions on hardware.

## Consequences

* Enables building both unprivileged game-like native titles and elevated system utilities under kstuff.
* Distinguishes between absent libraries and permission-gated libraries in hardware reports.
* Establishes a common vocabulary across `obscene`, `selfish`, and future tooling.

