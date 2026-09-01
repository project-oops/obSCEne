# D210 - Video is reachable from the payload, but the display is owned by the system


**Video is fully reachable from the payload, but the display is owned by the system - rendering needs a foreground context, not this background hijack.**

Having produced a socket report (D209), the remaining goal was the on-screen video report
(`display.c`). Every piece resolved:

- `sceKernelLoadStartModule("/system/common/lib/libSceVideoOut.sprx")` returns handle `0x12`,
  `res=0` - the module loads and starts. The short name and `.native` forms fail `0x80020002`;
  the full path is required.
- `sceKernelGetModuleInfo(0x12, &info)` returns 0 and its segment table gives the runtime base
  `0x8002a0000` (found by dumping the struct and taking the first `0x8xxxxxxxx` word - hexdump
  discipline, no guessed offset).
- Every `sceVideoOut*` export resolves as `vo_base + vaddr`, vaddrs from the pulled
  `libSceVideoOut.sprx` via selfish.

**Then `sceVideoOutOpen` refuses:** `0x80290009` for the system user (`0xFF`), `0x80290001`
(invalid value) for users `0` and `1`. `0x80290000` is the `SCE_VIDEO_OUT_ERROR` family. The
main video out is already held by the shell/compositor, and the payload runs inside a hijacked
**background** system app - `/system/vsh/app/NPXS40112/eboot.bin` - which is not granted the
display.

### What this means for the video report

`display.c` assumes it can open a video out and flip. On this delivery path it cannot, and that
is not a bug to fix in obSCEne - it is a property of where elfldr puts the payload. The
ecosystem agrees: the GUI payloads on this console render through an existing app (BFpilot is
"browser based"), not by opening the main video out from an injected process.

**The socket/file report is the viable output on the injection path; the on-screen render needs
a foreground, display-owning context** (replacing an app, or a display takeover that would
contend with the shell - invasive, and a stability risk on a console the operator wants kept
up). Not attempted without a decision to make it.

Status: **hardware** - every code observed on the console, 2026-08-27.

