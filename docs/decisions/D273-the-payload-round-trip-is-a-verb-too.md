# D273 - The payload round-trip is a verb too: `./bin/obscene payload`


The package round-trip has been a verb (`deploy`) since D269; the *payload* round-trip - build the unsandboxed plain-ELF shape, run it through elfldr, read back what it wrote - was still being assembled by hand each time as `hw logs & ; hw send ; grep`. That is the exact ad-hoc-bash trap D269 closed for the package, and it bit twice: the `wsl.exe -- bash -lc '...'` form mangles inline shell **variables** under Git Bash, so the assembled command silently ran with empty paths (a `T=$HOME/...; "$T"` became `"" hw send`), capturing nothing, twice, before the cause was seen.

So `./bin/obscene payload` now owns it, backed by `scripts/payload-run.sh`. It runs `make payload`, then captures **both** channels a payload can report on and reads whichever spoke: the send socket, when the net sink connects back (a run that reached that far - the first full run on 12.40 came home this way, 36,361 records to `OBS|end`), and the **raw** system log otherwise (raw, not the OBS-filtered `report`, because a crashing payload writes no `OBS|` records but the kernel writes the fatal-signal lines there - D233, and the finding). It sends the ELF to elfldr and says which channel spoke, so a running payload's report and a crash are told apart rather than both reading as "nothing arrived". Everything connects out, so unlike `deploy` there is no Windows half; the script re-enters WSL itself from a Windows shell. Flags: `--build-only`, `--seconds`, `--into`, `--name`, and `-- GEN=5 CORPUS=0` to make.

The rule is the standing one (D269, and the recurring feedback on ad-hoc scripts): a hardware capability is a `bin/obscene` verb backed by one script, never bash reassembled at the call site - because reassembling it is where the time goes.

Status: **done**.


