# D054 - The emulators live in `<emulators>`, source and all, outside this repository


Status: decided.

The shadPS4 install used for every emulator run in this project lived in the session
scratchpad - a directory that is transient by design. It survived by luck, and looking for
it in the wrong place is what surfaced the problem.

Binaries and shallow source clones now sit in `<emulators>`, and the run scripts
default there. Outside the repository deliberately: several hundred megabytes of
third-party material has no business in the history of a probe, and none of it is ours.

**The source is there because reading it has already paid for itself.** `DT_SCE_JMPREL`
and `DT_SCE_PLTRELSZ` were swapped in our tag table for weeks, and the derivation tool
could not have caught it - it checks `JMPREL + PLTRELSZ == RELA`, and addition is
commutative, so a swapped pair satisfies the identity exactly. It was found by reading a
loader that had them the other way round (D038).

Fourteen repositories: seven PS4 emulators, two PS5, and five format or symbol
references. `docs/EMULATORS.md` says what each is and what it is good for.

**The boundary is unchanged and worth restating.** These are read for facts - what a
format is, what a symbol is called. Several are GPL, and this program calls the
platform's own interface; it has no need of an emulator's code and must not acquire one.

