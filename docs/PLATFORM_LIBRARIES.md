# Platform Libraries: The Prospero Retail SPRX Filesystem Layout & Privilege Model

This document specifies the retail filesystem organization of system shared libraries (`.sprx`),
the sandbox partition boundaries, and the four-tier privilege model governing access on the
Prospero-generation platform (firmware 12.40).

**Everything below is measured directly from retail hardware** via obSCEne runtime probes
and live filesystem inspection. The complete module list is captured in the companion
manifest [`data/hardware/ps5-sprx-manifest.tsv`](../data/hardware/ps5-sprx-manifest.tsv), which
records every module as a measured snapshot (firmware 12.40). That manifest is the source of
truth for the per-tier tallies quoted below; read them as measured rather than fixed, since
they move with firmware.

---

## 1. Architectural Divergence from the Orbis-generation Platform

On the Orbis-generation platform, system shared libraries resided almost exclusively in a single
flat directory: `/system/common/lib/`. Any library not shipped in `/system/common/lib/`
was bundled with the application inside `/app0/sce_module/` (such as `libc.prx` or
`libSceFios2.prx`).

On the Prospero-generation platform, the vendor compartmentalized the system filesystem into distinct
mount points with independent access controls, security tokens, and sandbox rules:

```
/ (Root)
├── /system/common/lib/            [Tier: app]        Standard Title Sandbox
├── /system_ex/common_ex/lib/      [Tier: system]     System Apps & WebKit
├── /system/priv/lib/              [Tier: root]       Privileged Daemons & Core Codecs
└── /app0/sce_module/              [Tier: app/title]  Application-Bundled Modules (*.prx)
```

Linking a module statically via `DT_NEEDED` forces the system loader (`rtld`) to resolve
it at launch. If that library does not exist in `/system/common/lib/` or is in a higher
privilege tier denied to the title, **the process is terminated before executing a single
instruction** (D295).

---

## 2. The Four Privilege Tiers

Access to system libraries is partitioned into four distinct permission tiers (D296):

| Tier | Category / Target | Typical Directory | Credentials (`paid`) | Scope & Capabilities |
| :--- | :--- | :--- | :--- | :--- |
| **`app`** | Game title (`"gd"`) | `/system/common/lib/` | `0x3800000000000000`<br>`0x3100000000000002` | Standard sandboxed game titles. Access to the standard application-tier SPRXs and `/app0/sce_module/*.prx`. |
| **`sysmodule`** | Title with sysmodules | Dynamic via IPC | Same as `app` | Dynamically activated into process address space via `sceSysmoduleLoadModule(id)` / `SceSysCore`. |
| **`system`** | System App (`"system_app"`) | `/system_ex/common_ex/lib/` | `0x3800000000000001` | Extended system applications (WebKit, Shell UI, Settings, Media Player). Access to WebKit, Mono, and POSIX runtimes. |
| **`root`** | System Service / Daemon | `/system/priv/lib/` | `0x8000000000000001` | Privileged system services and jailbroken payloads. Access to proprietary hardware encoders and kernel services. |

---

## 3. Directory Layout & Module Breakdown

### 3.1 `/system/common/lib/` (Standard Application Tier)

Mapped into every standard game title sandbox (`appCategory: "gd"`); the manifest lists these
SPRXs as a measured snapshot. These libraries provide
the public and semi-public APIs exposed to games:

* **Core Runtime**:
  * `libkernel.sprx`, `libkernel_sys.sprx`, `libkernel_web.sprx` - System call interfaces and thread primitives.
  * `libSceLibcInternal.sprx` - Standard C runtime internals.
  * `libSceSysmodule.sprx` - System module dynamic loader.
  * `libSceAmpr.sprx` - Application memory protection runtime (automatically loaded by `libSceSysmodule`).
  * `libSceFios2.sprx` - Fast I/O subsystem.
* **Next-Generation Graphics (Prospero Native)**:
  * `libSceAgc.sprx`, `libSceAgcDriver.sprx`, `libSceAgcVsh.sprx` - Native Prospero AGC graphics pipeline (`EI_ABIVERSION 2`).
* **Legacy Graphics (Orbis Backwards-Compatibility)**:
  * `libSceGnmDriver.sprx` - Orbis GNM driver.
* **Peripherals & Input**:
  * `libScePad.sprx` (DualSense / DualShock), `libSceKeyboard.sprx`, `libSceMouse.sprx`, `libSceMove.sprx`.
  * `libSceAjm.sprx`, `libSceAjm.native.sprx`, `libSceAjmi.sprx` - Audio Joint Manager.
* **Audio & Video**:
  * `libSceAudioOut.sprx`, `libSceAudioIn.sprx`, `libSceAudio3d.sprx`, `libSceAudioPropagation.sprx`.
  * `libSceVideoOut.sprx`, `libSceVideoRecording.sprx`.
  * `libSceVenc.sprx` - Video Encoder. *(On Orbis this was split across `libSceVenc.sprx` and `libSceVencCore.sprx`; on Prospero it is consolidated).*
* **Networking & Identity**:
  * `libSceNet.sprx`, `libSceNetCtl.sprx` - Sockets and network configuration.
  * `libSceUserService.sprx`, `libSceNpCommon.sprx`, `libSceSaveData.sprx`, `libwebrtc.sprx`.

### 3.2 `/system_ex/common_ex/lib/` (Extended System Tier)

Mounted only for applications flagged as system applications (`system_app` / `0x3800000000000001`).
Sandboxed game titles cannot access this directory. As measured, it holds native C/C++ libraries
alongside a larger set of Mono/.NET assemblies (`.dll.sprx`); the manifest carries the exact split:

* **POSIX Runtime**:
  * `libScePosixForWebKit.sprx` - Full POSIX threading, sockets, and memory functions.
    *(Orbis provided `libScePosix.sprx` in `/system/common/lib/`. On Prospero, `libScePosix.sprx` does
    not exist; POSIX is relegated to this WebKit support library).*
* **Web Engines**:
  * `libSceNKWebKit.sprx`, `libSceNKWeb.sprx`, `libSceNKWebKitRequirements.sprx` - WebKit runtime.
  * `libSceJsc.sprx` - JavaScriptCore engine.
  * `libhermes-0.11.sprx` - Facebook Hermes JavaScript engine.
* **Open Source System Ports**:
  * `libcurl.sprx` - HTTP/networking.
  * `libcairo.sprx`, `libfontconfig.sprx` - 2D graphics and font rasterization.
  * `libSceVshFreeType.sprx`, `libSceVshHarfBuzz.sprx`, `libSceVshPng.sprx`, `libSceVshJpeg.sprx`,
    `libSceVshWebP.sprx`, `libSceVshBrotli.sprx` - Media rendering and decompression libraries.
* **Managed Frameworks**:
  * `libmonosgen-2.0.sprx`, `libmono-btls-shared.sprx`, `libmono-profiler-log.sprx` - Mono CLR runtime.
  * The `.dll.sprx` managed assemblies (the manifest holds the full set as measured), e.g. `mscorlib.dll.sprx`, `Microsoft.CSharp.dll.sprx`, `Newtonsoft.Json.PlayStation.dll.sprx`.
* **Shell UI Components**:
  * `libSceShellUIUtil.sprx`, `libSceGLSlimVSH.sprx`, `libSceGLSlimClientVSH.sprx`, `libSceNotificationClient.sprx`.

### 3.3 `/system/priv/lib/` (Root / Privileged Tier)

Accessible only to processes with root/kernel daemon credentials (`0x8000000000000001` or specific
daemon entitlements); the manifest lists these privileged SPRXs as measured. Unprivileged titles
attempting to load these fail with permission denials:

* **Proprietary Hardware Codecs & DSPs**:
  * `libSceAc3Enc.sprx` - Dolby Digital AC-3 hardware encoder.
  * `libSceDtsEnc.sprx` - DTS surround encoder.
  * `libSceMat2Enc.sprx` - Dolby MAT 2.0 encoder (Dolby Atmos bitstreaming).
  * `libSceAudiodecCpuTrhd.sprx` (Dolby TrueHD), `libSceAudiodecCpuDts.sprx`, `libSceAudiodecCpuDtsHdMa.sprx`,
    `libSceAudiodecCpuOpus.sprx`, `libSceAudiodecCpuLpcm.sprx`.
* **System Core**:
  * `libSceAudioSystem.sprx` - Low-level kernel audio bus.
  * `libSceDipsw.sprx`, `libSceComposite.sprx` - Hardware dip switches and composition hardware.
  * `libSceFsInternalForVsh.sprx`, `libSceLoginMgrServer.sprx`, `libSceAppDbShellCoreClient.sprx`.
  * `libmdbg_syscore.sprx` - Debug monitor.
* **AI & Voice Services**:
  * `libSceVoiceCommand.sprx`, `libSceVisionManager.sprx`.
* **Remote Play & Cloud Transport**:
  * `libSceNpRemotePlaySessionSignaling.sprx`, `libSceWebTransport.sprx`, `libSceWtIpcClient.sprx`.

---

## 4. Key Differences Summary (Orbis vs Prospero)

| Library / Functionality | Orbis | Prospero | Impact on Homebrew |
| :--- | :--- | :--- | :--- |
| **`libScePosix`** | `/system/common/lib/libScePosix.sprx` | `/system_ex/common_ex/lib/libScePosixForWebKit.sprx` | Titles cannot statically link `libScePosix`. Must resolve dynamically via `obs_module_open` (D295). |
| **`libSceVencCore`** | Present in `/system/common/lib/` | Absent (merged into `libSceVenc.sprx`) | Static `DT_NEEDED` on `libSceVencCore` aborts launch with `val 2` (`ENOENT`). |
| **3D Rendering** | `libSceGnmDriver.sprx` | `libSceAgc.sprx` / `libSceAgcDriver.sprx` | Native titles use AGC; GnmDriver is retained for backwards compatibility. |
| **Audio Encoders** | Broadly accessible | Strictly isolated in `/system/priv/lib/` | Dolby/DTS encoders require `root` privilege. |
| **C Runtime** | Bundled in `/app0/sce_module/libc.prx` | Built into `/system/common/lib/libSceLibcInternal.sprx` | Requiring `libc.prx` via `.libc_param` causes `SCE_KERNEL_ERROR_ESDKVERSION` if versions mismatch. |

---

## 5. obSCEne Integration & Probing Strategy

obSCEne treats module presence as an empirical question, probed dynamically rather than assumed:

1. **Static Dynamic Section (`DT_NEEDED`)**:
   `obscene.eboot.elf` statically links only a small, deliberately-bounded set of core libraries
   guaranteed to exist in `/system/common/lib/` on every Prospero firmware - gated by
   `EBOOT_LIBS` (default 18, see `Makefile`) precisely because a system loader resolves every
   named library before any of our code runs (D226). The exact count moves with the behavioural
   imports in [`src/probe/imports.c`](../src/probe/imports.c) and per-target exclusions there;
   read that file for the current figure rather than a number fixed on this page.
2. **On-Demand Loader (`obs_module_open`)**:
   All non-essential libraries (such as POSIX, video encoders, or system tools) are resolved at
   runtime using `obs_module_open()`, which searches the tier paths in order of accessibility.
3. **Privilege Flag (`--privilege`)**:
   `selfish` container builds can declare target privilege (`app`, `sysmodule`, `system`, `root`),
   generating the matching `paid` auth token and `appCategory` so probes can test higher tiers
   when executing under kstuff (D296).
4. **Census Logging (`OBS|modtier|...`)**:
   The runtime module probe outputs the verified permission tier of every reachable module directly
   into the test report for automated comparison.

---

## 6. Application Category, Display Arbitration, and Memory Budgets

Process authority and runtime hardware resources are governed across two orthogonal axes:

1. **Privilege Tier (`paid` in SELF header)**:
   Determines kernel authority, raw device access, sysctls, and directory partition traversal (`/system/priv/lib/`).
2. **Application Category (`applicationCategoryType` in `param.json`)**:
   Determines display bus ownership (`SceSysAvControl`), Direct Memory budget (`ResourceArbitrator`), and UI focus:
   - `0` (`SCE_APP_CATEGORY_TYPE_BIG_APP` / `gd` / `native_game`): Primary foreground application.
   - `65536` (`0x00010000`, `SCE_APP_CATEGORY_TYPE_SYSTEM_APP`): Background or system application.
   - `131072` (`0x00020000`, `SCE_APP_CATEGORY_TYPE_MINI_APP`): Mini app (quick menu overlay).

### 6.1 Hardware Display & Memory Arbitration Rules

* **HDMI Video Scanout**: `SceSysAvControl` grants ownership of the primary HDMI video bus (`OBS_VIDEO_BUS_MAIN = 0`) **exclusively to Big Apps (`category 0`)**. Calling `sceVideoOutOpen(..., OBS_VIDEO_BUS_MAIN, ...)` from a `system_app` (`category 65536`) fails with `0x80290001 (SCE_VIDEO_OUT_ERROR_INVALID_VALUE)`.
* **Direct Memory (`DMEM`) Budget**: `ResourceArbitrator` only grants direct physical memory pools (required for GPU framebuffers and DMA buffers) to Big Apps (`0x300000000` / 12 GB). For `system_app`, direct memory budget is 0 bytes (`sceKernelAllocateDirectMemory` returns `-1`).
* **Orthogonality**: A `root` eboot (`paid: 0x8000000000000001`) CAN be launched as a Big App (`category 0`). When launched with both, the kernel grants full 12 GB DMEM, switches HDMI video ownership to the app, and retains root kernel capabilities.

### 6.2 The Big App PRX Loader Contract

When launched as `category 0` (`native_game`), the Prospero runtime dynamic linker (`rtld`) activates the retail game startup contract:
* `rtld` unconditionally attempts to load `/app0/sce_module/libc.prx` before jumping to the eboot entry point.
* Under `kstuff` on current firmware (FW 12.40), `sceSblAuthMgrAuthHeader` intercepts and authenticates fSELF files using **Gen-4 container magic (`4F 15 3D 1D`)**.
* Bundled PRX modules in `/app0/sce_module/` must be wrapped in Gen-4 containers with matching privilege authority. Raw unencrypted ELFs (`\x7fELF`) or Gen-5 containers (`54 14 F5 EE`) cause `rtld` to abort launch with `PRX_SCE_MODULE_LOAD_ERROR` (`0xa0020102`).

### 6.3 Homebrew Configuration Matrix

| Target Goal | Application Category | Privilege (`paid`) | Bundled `sce_module/` | Suitable Use Cases |
|---|---|---|---|---|
| **GUI / Game / Emulator / Screen Output** | `0` (Big App) | `root` or `app` | **Required**: `libc.prx` (Gen-4 container, `4F 15 3D 1D`) | Emulators, games, graphical homebrew, full obSCEne visual tests. |
| **Headless Daemon / Background Service** | `65536` (`0x10000`) | `root` or `system` | Not required | FTP servers, telnet/shsrv daemons, headless klog dumpers. |
| **System Overlay / HUD** | `131072` (`0x20000`) | `system` | System app standard | On-screen displays, quick menus, notification overlays. |
