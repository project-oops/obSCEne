# 2026-09-01 (Daemon Disqualification: Requiring eboot.bin & Filtering .self) (D304)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` lines 17-72):
1. **Target Resolver Matched Auxiliary Daemon**:
   - In Run #17, the game had not fully launched or was below PID 78.
   - The path heuristic (`/app0/`) matched `/app0/webrtc_daemon.self` (PID 78).
   - `PT_ATTACH` attached to PID 78 (`ret=0`), but because it is a restricted system daemon without userland heap capability, remote `mmap` returned `0xffffffffffffffff`.
   - `PT_DETACH` executed cleanly with `ret=0`.
2. **Analysis**:
   - All retail PS4 and PS5 games have executable name `eboot.bin`, never `.self` or `.elf`.
   - Auxiliary background daemons mounted under `/app0/` (like `webrtc_daemon.self`) must not be matched as retail games.
3. **Remediation (`src/injector/target.c`)**:
   - **Filter `.self` and `.elf`**: Any binary path or comm containing `.self` or `.elf` is rejected immediately.
   - **Strict Path Heuristic**: Path matching requires both `/app0/` (or `/user/app/` / `/mnt/sandbox/`) AND `eboot.bin`.
   - System daemon checks are executed prior to path heuristics.

Verified: `make payload injector HARDWARE=1` (`9,410,856` bytes) compiles 100% clean with zero warnings and zero errors.

