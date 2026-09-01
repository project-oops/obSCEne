# 2026-08-31 (script canonicalization) - one `bin/obscene` verb per hardware task; `report` folded into the tool


Swept the hardware round-trip onto a single entry point. It had grown three overlapping deploy/log
scripts (`oops-rebuild-pkg.sh`, `oops-deploy.sh`, `oops-klog.sh`) and three near-identical recovery
scripts (`oops-recover.sh`, `oops-bootlog.sh`, `oops-crashhunt.sh`), each written mid-session and
run by hand - the proliferation the `scripts/README.md` warning is about, happening again.

**Every hardware task is now one `./bin/obscene` verb** (`deploy`, `report`, `recover`, `hwsweep`,
`minbuild`, `digcheck`, `prep`); the surviving `scripts/oops-*.sh` are their implementation and are
not run directly. `bin/obscene help` now computes its own range (like `bin/oops`) so a new verb
cannot silently truncate the help, which is how it had lost the hardware verbs once.

**`report` is folded into `obscene-tool`** as a subcommand (`Command::Report` / `run_report`),
alongside `pretty`/`verify`/`diff`. It reads the console system log via `pros_link::log::read`,
keeps only the `OBS|` records, writes them to `reports/hardware/console-klog.txt`, and prints the
meta/build/tally/end lines. It connects out, so it runs from either side. `oops-rebuild-pkg.sh`
phase 3 now captures that log concurrently across the launch instead of the old
`hw pull /data/obscene-report.txt` - that pull chased a file sealed `0600` inside the title's
sandbox (D233/D237), which is the whole reason the log channel exists.

Deleted `oops-deploy.sh`, `oops-klog.sh`, `oops-bootlog.sh`, `oops-crashhunt.sh` (the last two are
subsumed by `oops-recover.sh`, which absorbed `crashhunt`'s recursive crash-dump find). Docs
brought current: D269, `CLAUDE.md` ("Which side runs what" now opens with the verb runbook),
`scripts/README.md`, `docs/TOOLING.md` (a `report` row and a hardware-verb table), `docs/WORKFLOW.md`,
`docs/ARTIFACTS.md`, `docs/HARDWARE.md`. The surprise: `oops-deploy.sh` already captured the report
off klog (its launch block grepped `^OBS|` from `hw logs`) - the readable path was in the tree the
whole time, and `oops-klog.sh` reinvented it. Tool builds clean, clippy clean.

