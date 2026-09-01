# D136 - 352 libraries and 35,518 imports load and run. The ceiling was far higher than assumed


Status: derived - observed on two loaders.

`link/module.ld` reserved 0x800 for the dynamic table, which holds 128 entries. Every
imported library costs four tags, so that ceiling was 27 libraries and the corpus wanted
556. Raised to 0x20000 - about 8,000 tags, so roughly 2,000 libraries.

The result, which nothing in this project could have predicted:

| | shadPS4 | PS5PCEM |
|---|---|---|
| census records | 35,339 | 35,339 |
| present | **35,339** | 3,738 |
| absent | **0** | 31,601 |
| ran to the end | yes | yes |

**Both loaders accept a module importing 35,518 symbols from 352 libraries and run it to
completion.** A 13 MB module, 3.5 MB of vendor segment, 1,426 dynamic tags, and the host
build compiles it in 4.7 seconds - the cost estimates that argued against this were an
order of magnitude pessimistic.

shadPS4 resolving **all** 35,339, including 15,999 browser-engine symbols and 333 libraries
it does not implement, is the same finding as D130 at five times the scale and past any
possible argument. PS5PCEM's 3,738 is what resolution against an implementation looks like.

### What this changes

The runnable module can hold far more than was thought, so the question stops being "what
can we afford to import" and becomes "what is worth measuring". Two artefacts, not one
compromise:

- the probe, carrying the platform surface, which loads and reports behaviour;
- a carrier, carrying everything, which **does not need to load at all** - a static sweep
  of an import table is all a name search needs, so a module no emulator would map is still
  a complete answer for that purpose.

