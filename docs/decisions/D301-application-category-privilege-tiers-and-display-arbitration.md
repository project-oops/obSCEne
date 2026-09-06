# D301 - Application Category, Privilege Tiers, and Display Arbitration on PS5 Native

**Status**: [measured]
**Date**: 2026-09-03
**Context**: Investigating why obSCEne native runs completed 10,112 checks headlessly over klog while leaving the HDMI display black (0x80290001 on sceVideoOutOpen).

---

## Context

On PlayStation OS (Prospero / Orbis), process execution permissions and resources are partitioned across two independent axes:

1. **Privilege Tier (`paid` / Authority ID in SELF header)**:
   - `app` (`0x3800000000000000` or `0x3100000000000002`): Standard sandboxed execution.
   - `sysmodule` (`0x3800000000000001`): Dynamic system modules.
   - `system` (`0x3800000000000001`): Extended system application privileges.
   - `root` (`0x8000000000000001`): Root kernel authority (raw device access, sysctls, jailbreak access).

2. **Application Category (`param.json` -> `applicationCategoryType`)**:
   - `0` (`SCE_APP_CATEGORY_TYPE_BIG_APP` / `gd` / `native_game`): Primary foreground game.
   - `65536` (`0x00010000`, `SCE_APP_CATEGORY_TYPE_SYSTEM_APP`): System application or background utility.
   - `131072` (`0x00020000`, `SCE_APP_CATEGORY_TYPE_MINI_APP`): Mini app (quick menu overlay, etc.).

Historically, homebrew tooling assumed `system` or `root` eboots should be registered as `system_app` (`applicationCategoryType: 65536`). Hardware measurements on PS5 FW 12.40 refuted this assumption when attempting video output.

---

## Hardware Findings

### 1. HDMI Display Access and Direct Memory (`DMEM`) are Governed by Category, NOT Privilege

When obSCEne ran natively with `applicationCategoryType: 65536` and `privilege: root`:
- `SceSysAvControl` denied HDMI scanout ownership: calling `sceVideoOutOpen(user, OBS_VIDEO_BUS_MAIN, 0, 0)` returned `0x80290001 (SCE_VIDEO_OUT_ERROR_INVALID_VALUE)`. Bus 0 is reserved exclusively for Big Apps (`category 0`).
- `ResourceArbitrator` granted **0 bytes of direct memory** (`DMEM`), causing `020-memory/allocate` (`sceKernelAllocateDirectMemory`) to fail with `-1`.
- obSCEne's display subsystem recognized the refusal, returned `OBS_DISPLAY_FAILED`, and continued headlessly, executing all 26 test sections and 10,112 checks over klog while leaving the screen black.

When `applicationCategoryType` was set to `0` (Big App) with `privilege: root`:
- The kernel immediately granted full Big App resources:
```text
[KERNEL] INFO: launch bigapp: category 0x00000000, container 1
[KERNEL] INFO: DMEM size: 0x300000000 (12 GB Direct Memory allocated)
[KERNEL] INFO: FMEM size: 0x1c000000 (448 MB Flexible Memory allocated)
[AvControl] VideoOut: shared (pid=0x1b9 appId=0xa019)
[AvControl] video owner is switched to registered app(appId=0xa019)
```
- **Conclusion**: A `root` eboot CAN be launched as a Big App! Privilege level and Big App category are orthogonal. You do not need to launch unprivileged and escalate; a root eboot launched as `category 0` receives both root kernel privileges and the full GPU/display pipeline.

---

### 2. The Big App PRX Loader Contract (`category=native_game`)

When launched as `category 0`, the PS5 dynamic linker (`rtld`) activates the native game loader contract:
- `rtld` unconditionally attempts to load `/app0/sce_module/libc.prx` before jumping to the eboot entry point.
- The kernel's `self_pager_activate` invokes `sceSblAuthMgrAuthHeader` to authenticate the PRX container:
  - **Raw ELFs**: An unencrypted ELF (`\x7fELF`) lacks a SELF header, causing `sceSblAuthMgrAuthHeader` to fail with `unexpected error 46`, aborting with `PRX_SCE_MODULE_LOAD_ERROR` (`0xa0020102`).
  - **Gen-5 Containers (`54 14 F5 EE`)**: Current `kstuff` fSELF hooks on FW 12.40 only validate Gen-4 container magic (`4F 15 3D 1D`). Passing a Gen-5 container results in `=== Lack of a .prx file in /app0/sce_module is detected!!! ===`.
  - **Gen-4 Containers (`4F 15 3D 1D`)**: Bundled PRXs must be wrapped in `4F 15 3D 1D` containers so that `kstuff` validates them successfully.

---

## Guide for Future Homebrew: Which Settings to Choose

| Goal | Application Category | Privilege (`paid`) | PRX Contract (`sce_module/`) | Use Cases |
|---|---|---|---|---|
| **Full GUI / Game / Probe with Screen Output** | `0` (Big App) | `root` or `app` | **Required**: `libc.prx` wrapped in Gen-4 (`4F 15 3D 1D`) container matching eboot authority. | Emulators, games, graphical homebrew, full obSCEne GUI sweep. |
| **Headless Daemon / Background Tool** | `65536` (`0x10000`) | `root` or `system` | Not enforced by `rtld`. | FTP servers, web servers, telnet/shsrv daemons, headless klog dumpers. |
| **System Overlay / Quick Menu** | `131072` (`0x20000`) | `system` | Standard system app requirements. | System HUDs, overlay menus, notification hooks. |

---

## Consequences

* Fixes the native black screen defect: obSCEne native builds targeting display output must declare `applicationCategoryType: 0` in `param.json`.
* Fixes the PRX loading requirement: bundled native PRXs (`libc.prx`) must be wrapped in Gen-4 fake-signed containers under kstuff.
* Reassures homebrew development that root access and graphical HDMI output are fully compatible without requiring runtime privilege escalation hacks.
