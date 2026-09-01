# 2026-09-01 (memory allocator & online walls) - added probes for VirtualQuery, AllocateMainDirectMemory, and NpCppWebApi (D284)


Added hardware measurement probes in obSCEne to unblock sibling emulator `orbistoun` on commercial title walls (recorded in orbistoun D436: PPSA25872, PPSA02664, PPSA03416, PPSA28061):
- `include/obscene/platform.h` & `src/probe/imports.c`: Declared and registered `sceKernelVirtualQuery` and `sceKernelAllocateMainDirectMemory` under `libkernel`.
- `src/probe/sections/memory.c`:
  - `020-memory/virtual-query-mapped`: Queries mapped direct memory, dumps the raw `SceKernelVirtualQueryInfo` buffer via `obs_report_written` against `0xAA` poison pattern to reveal the true struct layout (protection, memoryType, flags, name).
  - `020-memory/virtual-query-text`: Queries executable code segment, dumping the layout for RX permissions.
  - `020-memory/virtual-query-stack`: Queries thread stack address, dumping the layout for stack mappings.
  - `020-memory/virtual-query-unmapped`: Queries `0x720000240000` (the exact address from D436) to record the exact return code and error semantics on unmapped pages.
  - `020-memory/allocate-main`: Calls `sceKernelAllocateMainDirectMemory`, records the reported physical base address, maps it, verifies read/write, and releases it.
- `src/probe/sections/layoutmap.c`:
  - `138-layout/np-cpp-webapi`: Probes `libSceNpCppWebApi.sprx`, measures `Common::initialize` address and queries the unnamed online leaderboard NID `0xa9721c01ca796f63`.

Verified: `make check`, `make payload injector HARDWARE=1`, and `bash scripts/verify.sh` pass 100% clean.

