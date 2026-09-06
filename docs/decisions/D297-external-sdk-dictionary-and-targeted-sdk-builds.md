# D297 - External SDK Version Dictionary and Targeted SDK Builds in SELFish and obSCEne

**Status**: [decided]
**Date**: 2026-09-02
**Context**: Eliminating `0x80020063 (SCE_KERNEL_ERROR_ESDKVERSION)` crashes, replacing magic hex constants with user-friendly string identifiers, and decoupling version tables from source code.

---

## Context

When launching a native PlayStation 5 application on real hardware (FW 12.40), the kernel dynamic loader enforces a minimum SDK version check during module loading. Declaring `sdk_version_second = 0` (`PPR:00000000`) caused `sceKernelLoadStartModule` to abort process startup immediately with:

```text
[rtld] ERROR self_load_shared_object:2826: B: res 0 (libc.prx) val 2
PRX_SCE_MODULE_LOAD_ERROR (0xa0020102), r12: 0x80020063 (SCE_KERNEL_ERROR_ESDKVERSION)
```

Arbitrary or hardcoded magic numbers like `0x02000009` are opaque, non-intuitive to users, and inflexible when target platforms or firmware baselines change over time. Furthermore, if a user targets a Gen-4 package (`ps4_compat`), using a Gen-5 Prospero SDK version is invalid and must be rejected at container build time.

## Decision

1. **Decouple SDK Tables into an External Dictionary**:
   - Created `selfish/data/sdk-versions.toml` containing known SDK version mappings, aliases (`ps5-native`, `ps5-current`, `ps4-compat`, `zero`), target console generations, and descriptions.
   - Users and tools can update or extend this table without recompiling source binaries.

2. **Parse and Validate Target SDKs in SELFish**:
   - Implemented `selfish-container::sdk::SdkDictionary`, supporting loading from custom paths (`--sdk-table`) or embedded fallbacks.
   - String versions accept dotted notation (e.g. `"2.000.009"`, `"8.050.001"`, `"11.600.005"`), hex literals (e.g. `"0x02000009"`), or aliases.
   - Enforce generation compatibility: Gen-5 Prospero versions are rejected when targeting `Generation::Previous`, and Gen-4-only versions without PPR are rejected for `Generation::Current`.

3. **ELF `PT_SCE_PROCPARAM` Segment Patching**:
   - `patch_elf_procparam()` in `selfish-container` locates the `PT_SCE_PROCPARAM` segment (`p_type == 0x61000001`) in uncompressed ELF binaries and stamps both `sdk_version` (PS4) and `sdk_version_second` (PPR) before container packing.

4. **Pass `--sdk` through the Toolchain**:
   - Added `--sdk <version>` to `selfish-cli wrap`.
   - Added `--sdk <version>` to `obscene-tool mkself`.
   - Updated `Makefile` with:
     ```makefile
     PRIVILEGE ?= app
     SDK ?= $(if $(filter 5,$(EBOOT_GEN)),ps5-native,ps4-compat)
     ```
   - Updated `native:` target to default to `SDK = ps5-native` (`0x08050001` / `0x02000009`).

## Consequences

* Hardware execution on PS5 FW 12.40 confirms kernel acceptance:
  ```text
  [KERNEL] INFO: Application Category Type: 00010000
  [KERNEL] INFO: SDK vesion: PS4:08050001 PPR:02000009
  ```
* Completely eliminates `0x80020063 (SCE_KERNEL_ERROR_ESDKVERSION)`.
* Provides clean, user-friendly command-line ergonomics without cryptic magic numbers.

