# 2026-09-01 (hardware injector capture #3 analysis: exit via SYS_exit & socket log deduplication) (D287)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` and `reports/hardware/injector-klog.txt.klog`):
1. **Telemetry & Safety Verification**:
   - Syscall trampoline dispatched all calls through `libkernel` without triggering any `PPRBUG-22859` traps.
   - Kernel R/W cleanly established on FW 12.40 (`0x12400009`), locating kernel data base `0xffffffffc4940000`.
   - Process elevation succeeded (`authid=0x4800000000010003`) and syscall restriction boundary was unlocked.
   - `target_resolve` successfully verified that no sandboxed retail title was running, protected all system processes, logged `"ERROR: no running retail game found in userland"`, and restored credentials via `krw_restore_current_process()`.
2. **Post-Execution Crash Cause (`SIGILL` at `0x4000ab`)**:
   - `injector_start` exited via `return -4;`, executing `ret` at `0x2000044fd`.
   - This popped the address `0x4000ab` left on the stack when `elfldr` intercepted the sacrificial daemon `NPXS40112` (`SceSpZeroConf`). Returning into `NPXS40112` at `0x4000ab` triggered an unhandled `SIGILL` (`privileged instruction fault`), causing `Syscore App` to log `App Crash: PID=0xe5, reason=0x4`, invoke `coredump.elf`, and terminate the process.
   - As established in `src/probe/start.c`, freestanding payloads must never execute `ret`.
3. **Clean Process Termination**:
   - Added `#define SYS_exit 1` to `src/common/syscall.h`.
   - Added `injector_exit(int code)` to `src/injector/injector.c` issuing `sys_call(SYS_exit, code)` with a fallback spin loop (`pause`).
   - Replaced all `return -N;` and `return 0;` statements in `injector_start` with `injector_exit(code)`, terminating the sacrificial host process cleanly without coredumps.
4. **Socket Log Deduplication & Hex Prefix Formatting**:
   - Traced duplicated lines in `injector-klog.txt` to `klog_write` writing to both `fd 1` and `fd 2`. Because `elfldr` `dup2`'s the incoming TCP socket to both descriptors (D205), every message was transmitted twice over the network.
   - Removed `sys_call(SYS_write, 2, ...)` from `klog_write` so each line is sent once over `fd 1`.
   - Corrected static string prefixes in `klog_write_hex` calls to avoid prepending redundant `0x` ahead of `obs_format_hex`.

Verified: `make check BUILD=$HOME/obs`, `make payload injector HARDWARE=1 BUILD=$HOME/obs` (`9,393,592` bytes), and `bash scripts/verify.sh` pass 100% clean.

Hardware test `./bin/obscene inject` re-run (Run #4) validated all fixes live on console:
- **Clean process exit verified**: PID 231 logged `# process pid=231, payload.elf calls exit() exit_value=fffffffc.` with ZERO fatal signals, zero `SIGILL`, zero coredumps, and no `App Crash` events.
- **Log deduplication verified**: `injector-klog.txt` received exactly 7 non-duplicated lines over the socket.
- **Hex format verified**: Correct `fw=0x12400009` and `base=0xffffffffc4940000` prefixes without duplicate `0x0x`.

