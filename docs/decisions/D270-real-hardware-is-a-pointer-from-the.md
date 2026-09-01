# D270 - Real hardware is a pointer from the compatibility table, not a column in it


`docs/COMPATIBILITY.md` compares loaders - host, shadPS4, PS5PCEM, fpPS4, kyty, orbistoun - and a reader reasonably asks where the real PS5 is. It is not a column, deliberately.

Two reasons. First, a retail console is not a loader being compared; it is the source of truth the loaders are scaffolding *toward* (the document's own framing), so ranking it in the same table as the approximations inverts what the table is for. Second, the compat table is *generated* by `obscene-tool compat` from emulator report files and gated against drift by `verify.sh` (D069). A hardware tally added as a column would either have to be regenerated from a committed hardware report on every run - coupling a gated, machine-maintained table to a measurement that changes shape as the suite grows - or be hand-maintained beside machine-maintained cells, which is the exact staleness this project quarantines patched-loader results for (D176).

So the hardware results live in `docs/HARDWARE.md`, cited record by record to `data/hardware/` (`ps5-full.txt` before imports bound, `ps5-imports.txt` after), and `COMPATIBILITY.md` carries a prominent pointer section to them rather than a copy. The committed screenshot `screenshots/ps5-hardware.png` is shown in both. `./bin/obscene report` / `deploy` capture a fresh run (D269). This is what the question "why is there no hardware column?" resolves to, recorded so the answer is not re-derived as a column that breaks the gate.

Status: **done**.

