# 2026-08-31 (compatibility ↔ hardware) - the compat table now points to the hardware results


The question "does obSCEne compatibility have a hardware entry?" exposed a real gap: `docs/COMPATIBILITY.md` compares six loaders (host/shadPS4/PS5PCEM/fpPS4/kyty/orbistoun) but had no sign of the real-console results, which live in `docs/HARDWARE.md` and `data/hardware/` - and it still carried a stale "0 hardware confirmations" line.

Added a prominent **pointer section** (not a column): real hardware is the source of truth the loaders scaffold toward, so it gets a link to HARDWARE.md, the committed `ps5-hardware.png` screenshot, and the two committed runs (`ps5-full.txt`/`ps5-imports.txt`) - but no pasted tally, because the compat table is generated and drift-gated (D069) and a hand-maintained hardware cell is the staleness this doc quarantines patched results for. Qualified the "0 hardware confirmations" line to its real scope (the 910-bulk behavioural map is shadPS4-only; the suite itself has run on hardware). Recorded the choice as D270 so nobody re-derives it as a column that breaks the gate. compat --check still current (table untouched), doccheck clean.

