# 2026-09-02 (ps5 native execution) targeted SDK dictionary, procparam inspection, and PRX module contract

Resolved the native PS5 process startup blocker `0x80020063 (SCE_KERNEL_ERROR_ESDKVERSION)`, decoupled SDK versioning into an external dictionary, canonized procparam auditing in `048-selfaudit`, and uncovered the `PT_SCE_MODULE_PARAM` contract on hardware.

1. **Targeted SDK builds and external dictionary (D297)**:
   - Created `selfish/data/sdk-versions.toml` mapping human-readable version strings (e.g. `"2.000.009"`, `"8.050.001"`), aliases (`ps5-native`, `ps5-current`, `ps4-compat`), generations, and descriptions outside of source code.
   - Implemented `selfish-container::sdk::SdkDictionary` with validation against console generations (rejecting Gen-5 Prospero versions for Gen-4 packages and vice versa).
   - Added `patch_elf_procparam()` to locate `PT_SCE_PROCPARAM` and stamp both `sdk_version` and `sdk_version_second`.
   - Exposed `--sdk` on `selfish-cli wrap`, `obscene-tool mkself`, and `Makefile`. Default for native is `ps5-native` (`0x08050001` / `0x02000009`).
   - Verified on PS5 FW 12.40 hardware: `[KERNEL] INFO: SDK vesion: PS4:08050001 PPR:02000009`. `SCE_KERNEL_ERROR_ESDKVERSION` is completely resolved.

2. **`libc_param` and SIGSEGV at 0x28 diagnosis (D298)**:
   - Setting `.libc_param = 0` caused an immediate `SIGSEGV` page fault at address `0x0000000000000028` inside `libkernel.sprx` during startup.
   - Restored `.libc_param = &obs_libc_param`: `libkernel`, `libSceLibcInternal`, `libSceSysmodule`, and `libSceAmpr` loaded cleanly without memory faults.

3. **`PT_SCE_MODULE_PARAM` contract identified (D298)**:
   - After the eboot's primary libraries loaded, `libSceSysmodule` reported `PRX_SCE_MODULE_LOAD_ERROR (0xa0020102)` ("Lack of a .prx file in /app0/sce_module is detected").
   - Investigation identified that PS5 PRX modules require program header `PT_SCE_MODULE_PARAM` (`0x61000002`), which carries module parameters including SDK version (`0x08050001` / `0x02000009`).
   - Adding `PT_SCE_MODULE_PARAM` directly to our synthetic `src/probe/sce_module.c` satisfies `libSceSysmodule`, allowing modules to build completely from open source.

4. **Canonized `PT_SCE_PROCPARAM` audit in `048-selfaudit` (D299)**:
   - Extended `048-selfaudit/container-structure` in `src/probe/sections/selfaudit.c` to locate and read `PT_SCE_PROCPARAM`, reporting `ps4_sdk`, `ppr_sdk`, and libc/mem pointers.
   - Verified that both host harness (`make check`) and tooling tests (all 209 unit tests) pass cleanly.

5. **End-to-end native execution on PS5 hardware (10,112 OBS records)**:
   - Integrated `PT_SCE_MODULE_PARAM` (0x61000002) directly into `link/library.ld` and `src/probe/sce_module.c`, and omitted `libSceFios2` for Gen-5.
   - Closed stuck processes cleanly via Home Menu (avoiding POSIX `kill` signals that bypass VSH app lifecycle state).
   - Deployed the complete native build to `/user/data/homebrew/PPSA99980` via `./bin/obscene native --deploy`.
   - Executed `launch PPSA99980` on console FW 12.40: process ran natively without errors or faults, executing sections `000-boot` through `910-bulk`.
   - Captured full live run report in `reports/hardware/console-native-run.txt`: **10,112 records**, tally `122|12|381|54`, concluding cleanly at `OBS|end|puts`.


