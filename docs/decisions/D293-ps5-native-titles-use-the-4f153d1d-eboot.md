# D293 - a real ps5 native title uses the 4F 15 3D 1D eboot, so `native` defaults to it


D289 made `native` default to a gen-5 container (`54 14 F5 EE`), on selfish's table labelling that
magic "current generation" - a labelling the table itself calls a hypothesis. Hardware refuted it.

An injector run into a retail ps5 title (PPSA02664) measured, through `048-selfaudit`:

    metadata/ps5_param_json    1                 (it is a native ps5 title - it has param.json)
    metadata/gen5_containers   0                 (no eboot carries 54 14 F5 EE)
    metadata/gen4_containers   1                 (its eboot carries 4F 15 3D 1D)
    metadata/verdict           ps5_native_only

So a genuine current-generation native title carries a **`4F 15 3D 1D`** eboot - the magic selfish's
table calls "previous generation" - and no *eboot* on the console carries the `54 14 F5 EE` magic we
had been building. selfish's own source note reconciles it: `54 14 F5 EE` is the magic a title's
**bundled modules** carry, not its eboot, which uses `4F 15 3D 1D`. So the magic is real and current,
just not the eboot's - and obSCEne builds an eboot, so it never needed it. The table now records this
distinction (`data/self-format.tsv` generation rows) rather than asserting "the current console uses
this" for eboots.

`native` now takes the default `EBOOT_GEN=4`, so its eboot is `4F 15 3D 1D` like a real title's. This
is not a retreat to "ps4": the container magic `4F 15 3D 1D` is what the *current* generation uses
for app eboots, and what makes obSCEne's entry a ps5 native title is `param.json` + native
registration, not the eboot's magic. `make native EBOOT_GEN=5` still builds the `54 14 F5 EE`
variant, kept only to try against hardware - it matches nothing measured.

`confirm-table` passing 9/9 and `ex_info.paid = 0x3100000000000002` matching on this same container
say the fixed header rows and the authority id are generation-stable, which is why the default gen-4
container is a faithful match rather than a guess.

**Two follow-ups this measurement opens, not done here.** (1) selfish's `Generation::Current =
54 14 F5 EE` attribution (`selfish-abi/src/generation.rs`, `data/self-format.tsv`) is refuted for app
eboots and should carry this finding - the format's home is selfish, so that edit belongs to a
selfish session. (2) A real title's `sce_sys` holds fourteen entries (`keystone`, `nptitle.dat`,
`pfs-version.dat`, `disc_info.dat`, `pic0-2`, `trophy2`, `uds`, ...) where obSCEne's native title has
two (`param.json`, `icon0.png`); ShadowMountPlus needs only `param.json`, but if a launch is ever
refused for a missing piece, `keystone` (selfish already generates it for the package) is the first
to add.
