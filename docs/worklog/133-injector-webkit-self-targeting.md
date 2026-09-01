# 2026-09-01 (Injector WebKit Self-Targeting Elimination & Robust Target Resolution) (D301)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` lines 17-116 and 178-181):
1. **Critical Discovery - Injector was Targeting Its Own WebKit Container (`NPXS40112`)**:
   - Path telemetry revealed that PIDs 109, 110, 112, 118, 133, 139, 141, 178 were all `/system/vsh/app/NPXS40112/eboot.bin`!
   - `NPXS40112` is the system WebKit / notification helper container used by `elfldr` to host the injector.
   - The injector was previously selecting its own WebKit container/siblings as the candidate game and attempting to attach to itself, which caused FreeBSD `sys_ptrace(PT_ATTACH)` to reject with `errno=37` (`EALREADY`).
2. **Why the Real Retail Game was Skipped**:
   - The candidate loop previously called `is_system_daemon(comm)` before reading metadata.
   - For procs with empty names at legacy offset `0x61E`, `is_system_daemon` returned true and skipped the process.
   - PIDs 298 and 297 (the foreground game) were skipped as `(empty)` before their Title ID at `0x470` or path at `0x604` could be examined.
3. **Remediation**:
   - **Explicit WebKit Container Exclusion (`src/injector/target.c`)**: Added check ensuring any process with `path`, `comm`, or `titleid` matching `NPXS40112` is skipped immediately.
   - **Metadata-First Evaluation (`src/injector/target.c`)**: `titleid` (`0x470`), `path` (`0x604`), and `name` (`0x5E4`) are read first before any daemon heuristics.
   - **Immediate Retail Game Match**: Any process with Title ID matching `PPSA*` or `CUSA*`, or app path matching `/app0/` or `/user/app/`, is immediately returned as the target game PID without being filtered by empty comm checks.

Verified: `make payload injector HARDWARE=1` (`9,410,856` bytes) compiles 100% clean with zero warnings and zero errors.

