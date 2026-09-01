# D287 - the native title carries its eboot and is a `./bin/obscene` verb


`make native` laid out a title directory with `param.json` + `icon0.png` and **no executable**,
on the reasoning (recorded in the script itself) that a native title running its own code needs a
signed `eboot.bin` and no fake-signing keyset exists for this generation. That is true off
-jailbreak. It is false on the target: with **kstuff** active the kernel accepts a **fake-signed
fSELF** - the very eboot the package already installs and launches. So the earlier layout gave up
running native code for a constraint that does not apply where obSCEne actually runs.

Three changes, so the native path produces a droppable, self-launching current-generation title:

1. **It carries the eboot.** `build-native.sh` stages `$BUILD/eboot.bin` and passes it to
   `selfish native --root`, which copies it verbatim into `<TITLE_ID>/`. The result is
   `<TITLE_ID>/{eboot.bin, sce_sys/param.json, sce_sys/icon0.png}` - exactly the shape an installed
   PS5 title has, and exactly what an auto-mounter (ShadowMountPlus) scans for: a directory whose
   `sce_sys/param.json` names a title id. `NO_EBOOT=1` keeps the old deeplink-only launcher layout.

2. **A PS5-shaped id.** The default title id moved from `OBSC00001` to `PPSA00001` (overridable),
   with a matching `contentId` (`UP0000-<TITLE_ID>_00-OBSCENE000000000`), so the entry reads as
   current-generation and an auto-mounter accepts it. Override to avoid colliding with an installed
   title.

3. **A first-class verb.** `./bin/obscene native` runs `make native`, which now declares `eboot`
   as a prerequisite (it did not) so the executable the title carries is built before it is staged.
   Reached as a verb, never by invoking `build-native.sh` by hand (OOPS: verbs, not ad-hoc scripts).

Building stays in selfish: `selfish native` writes `param.json`/`icon0.png` and copies the eboot;
the eboot's bytes are selfish's format work through `make eboot`. `build-native.sh` only
orchestrates - it computes nothing.

**How registration happens is deliberately left open**, because two routes now fit the same
directory: a one-shot obSCEne payload calling `sceAppInstUtilAppInstallTitleDir(title_id,
"/user/app/", 0)` (self-contained, no third-party dependency), or dropping the directory where
ShadowMountPlus scans and letting it register via the same call (zero obSCEne payload, at the cost
of depending on that GPLv3 daemon running). The directory this produces is the input to both.

Verified: `selfish native --root` produces the complete title dir with a PPSA `titleId`, and
`build-native.sh` invoked as `make native` invokes it correctly end to end - tested with a stand-in
eboot. **The real eboot could not be built this session**: `make eboot` compiles
`src/probe/runtime.c`, which currently fails to compile in another session's in-flight module
-enumeration work (`sceKernelGetModuleList`/`GetModuleInfo` called but not in scope at the call site
- a minimal TU including `platform.h` compiles, so the defect is isolated to that file, not to the
declarations). Not this change's file and not fixed here. The moment `runtime.c` compiles,
`./bin/obscene native` yields the complete droppable title dir with no further change.
