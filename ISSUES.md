# Issues

Open defects, gaps and unmeasured facts, one line each. Delete a line when it is fixed.

## Code

- `src/probe/sections/agc.c` is 19.7k lines; split it by topic (command buffers, queues, shaders, draws, textures, display) with one GPU-session helper.
- `agc.c` holds 11 functions parked behind `__attribute__((unused))` and two `#if 0` blocks.
- 16 symbol resolvers carry their own fallback order (`runtime.c`, `harness.c`, `fault.c`, `encoder.c`, `fiber.c`, `inputext.c`, `layout.c`, `net.c`, `posix.c`, `posixerr.c`, `syncaddr.c`, `agc.c`); one `obs_resolve` should replace them.
- `audiodec.c` and `videodec.c` hand-write the same `strcmp` ladder over symbol names; use one table.
- The probe-source comments still carry decision lists, `REQ-` IDs and history; the comment pass has not run on the C sources.
- 138 Orbis-target stub functions and a second check registry duplicate the table; a per-row target mask would replace them.
- `tool/src/elf.rs` is a second ELF reader beside `selfish-elf`.
- `scripts/format.sh` skips `agc.c`; remove the exclusion once it is formatted.

## Protocol

- A second connection waits in the listen backlog instead of being refused `busy`.
- The `resolve`, `write` and `gpu` verbs are not implemented.
- `read` does not check the address first; an unmapped address ends the process.
- `sysinfo` `ip` and `firmware` are unconfirmed.

## Measurements

- `110-modules/info-size`: re-run against a describable handle to get the `SceKernelModuleInfo` size.
- The `900-surface` generation gate records `libSceAgc` symbols absent though the Agc section calls them.
- Whether the memory-query third field `3` is a memory type or a state.
- Whether a short memory-query buffer truncates or overruns.
- System versus application module handle ranges, the flexible-memory budget, and counter-frequency stability.
- The `_umtx_op`/futex third-register timeout unit.
- Whether hardware calls `DT_INIT` for an executable, and the entry-point calling convention.
- What `.sce_process_param` must contain, and whether the loader requires a SELF.
- What `sceKernelDirectMemoryQuery`'s second argument selects; whether `0xFE00` is usable.
- Which user id `sceVideoOutOpen` accepts.
- Mutex type constants (D177).
- Whether the platform returns `EINVAL` or refuses invalid arguments.
- The current-generation export-library tag.
- Hardware refuses packages whose header region (e.g. a byte at `0x600`) is modified.
- The size-argument and enumerated-argument sweeps (memory-query flags, protection bits, memory types).
- GPU address versus CPU address, gated on `166-agc` reachability.
- GNM compute: SRD/V# and user-data SGPR layout.
- Generation is `unknown`/`ps4_mode` for a `ps4_game` title rather than detected.
- `libScePosix` does not load in the app sandbox, so five checks are untested.
- A third current-generation loader is needed for N-way consensus.

## Tooling and docs

- The tracer is unbuilt.
- No Deck-blessed GPU golden corpus exists.
- Nothing runs the emulator exclusion-and-retry sweep D144 describes; restore it or update D144.
- `obscene-tool.exe` (Windows debug build) overflows its stack, so `doccheck` cannot run there.
- `CLAUDE.md` and `scripts/wsl.sh`, `oops-rebuild-pkg.sh`, `sweep-emulators.sh` default to a WSL distribution named `Ubuntu`.
- `docs/WORKLOG.md` is still the old generated index of deleted files; replace it with milestone entries.
- The CI package job is `continue-on-error`.
- The `DESIGN.md` section table is hand-kept rather than generated from `src/probe/registry.c`.
- `docs/INJECTOR.md` cites the deleted D276.
- The positive-to-negative check ratio is low; struct-taking functions have no behavioural checks.
