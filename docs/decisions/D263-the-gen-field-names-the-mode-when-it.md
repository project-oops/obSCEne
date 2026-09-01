# D263 - The GEN field names the mode when it cannot name the console; the PS4-compat version is its own field


*status: decided*

Two related additions, both to the HUD and both meaningful only in a UI build (D262), and both
gen4-only because they describe the PS4-compatibility environment a `ps4_game` runs in.

**GEN reads `ps4_mode` where it read `unknown`.** The console's generation genuinely cannot be
identified from inside a `ps4_game` - the current generation's driver is refused (D255) - so the
old answer was `unknown`. But the *mode this title runs in* is not unknown: it was built `gen4`
and it is running, so it is running as a `ps4_game`, which the platform calls PS4 compatibility
mode. That is a build fact this program is certain of, not an inference about the machine, and
it is a more useful answer than `unknown`. On a gen5 build the field keeps its `unknown`.

**A `PS4FW` field carries the compatibility environment's version.** `sceKernelGetSystemSwVersion`
reports 13.090.001 on the 12.40 console it was measured on - not the console's firmware (that is
the FW field, from `kern.version`, D261) but the version a `ps4_game` is told it runs on. The
two differ, and the difference *is* the fingerprint of compatibility mode, so both are worth
showing side by side rather than picking one and calling it "firmware".

The field exists only in a gen4 build: it is added to `obs_sys_field` under `#if OBSCENE_GEN == 4`,
so a gen5 build has no such enum member, no column, and no getter - which is how "do not show it
outside ps4_mode" is honoured without a run-time flag. A gen4 build *is* ps4_mode, so gating on
the build generation and gating on the mode are the same gate. `sysinfo.h` guards `OBSCENE_GEN`
with a default so a stray include cannot silently change `OBS_SYS_COUNT` and desync the enum.

