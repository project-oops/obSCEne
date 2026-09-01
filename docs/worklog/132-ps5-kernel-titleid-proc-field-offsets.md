# 2026-09-01 (PS5 Kernel TitleID & Proc Field Offsets Discovery: 0x470) (D300)


Analyzed hardware test logs (`reports/hardware/injector-klog.txt` lines 341-370):
1. **Critical Target Misidentification Discovered**:
   - The injector previously scanned `allproc` in reverse order and picked the first process with comm `eboot.bin`, which in Run #13 was PID 191.
   - Target parent telemetry revealed:
     `target p_pptr=0xffffca1378d84cf0`
     `target parent pid=112`
   - PID 191 was NOT the root retail game process - it was a sub-worker/child process spawned by PID 112 (`eboot.bin`), explaining why `PT_ATTACH` was blocked by the kernel.
2. **PS5 Firmware 12.xx `proc_field_offsets` Confirmed from `ps5debug-NG`**:
   - `proc_get_field_offsets()` confirms that on PS5 (FW >= 12.00):
     - `proc + 0x470`: `titleid` (16 bytes, ASCII: `CUSAxxxxx` or `PPSAxxxxx`)
     - `proc + 0x504`: `contentid` (64 bytes)
     - `proc + 0x5E4`: `name` (32 bytes)
     - `proc + 0x604`: `path` (64 bytes)
3. **Remediation**:
   - **Precise Title ID Detection (`src/injector/target.c`)**: Updated `get_proc_title_id()` to read directly from `proc + 0x470` (`0x470` on FW >= 8.xx, `0x49A` on 7.xx, `0x498` on 6.xx).
   - **Candidate Metadata Logging (`src/injector/target.c`)**: Every candidate process now logs its Title ID (from `0x470`), application path (from `0x604`), and parent PID (from `p_pptr + 0xBC`).
   - **Target Retail Game Resolution**: When a candidate process has a valid retail Title ID (`CUSA*` or `PPSA*`), it is immediately identified and selected, distinguishing the authentic game from auxiliary WebKit/daemon `eboot.bin` helpers.

Verified: `make payload injector HARDWARE=1` (`9,410,824` bytes) compiles 100% clean with zero warnings and zero errors.

