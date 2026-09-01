# D269 - One verb per hardware task; the report comes off the system log, in the tool


The hardware round-trip had grown three overlapping scripts - `oops-rebuild-pkg.sh` (build + install + pull the report from `/data`), `oops-deploy.sh` (install + launch + read the report off the system log), and `oops-klog.sh` (read the report off the system log) - plus three near-identical recovery scripts (`oops-recover.sh`, `oops-bootlog.sh`, `oops-crashhunt.sh`). Each was written mid-session and invoked by hand. The scripts drifted because there was no single entry point: a task got a new script instead of an existing verb.

### The single entry point is `bin/obscene`, not a script path

Every hardware task is now one verb, and the scripts under `scripts/` are its implementation, never run directly:

| verb | does |
|---|---|
| `./bin/obscene deploy` | build (WSL) + install/launch (Windows) + capture the report - the whole round-trip |
| `./bin/obscene report` | capture obscene's records from the system log into a file |
| `./bin/obscene recover` | read-only: what the console recorded, after a crash |
| `./bin/obscene hwsweep` | iterate against hardware, excluding each call that does not return |
| `./bin/obscene minbuild` | the minimal diagnostic package |
| `./bin/obscene digcheck` | does a built package still agree with its own digests? |
| `./bin/obscene prep` | bring the readable services (klogsrv/shsrv) up |

This is the obSCEne half of the same rule `oops` follows: a task that is one repo's is a verb on that repo's entry point; only genuinely cross-repo work (`oops check`, the `oops-ci.sh`/`oops-selfcheck.sh` gates) belongs to `oops`. The `oops-` prefix on the surviving scripts is kept only because `DECISIONS.md` and `CHANGELOG.md` cite the filenames; the verbs are the interface, so the names are now implementation detail.

### The report is captured, not pulled - because the file is sealed in the sandbox

`report` is folded into `obscene-tool` (a subcommand, alongside `pretty`/`verify`/`diff`), not a shell script. It reads the same channel `hw logs` does, keeps only the `OBS|` records, writes them to a file, and prints the run's outcome. It connects *out* to the console, so it needs no Windows build and runs from either side.

This retires the last `hw pull /data/obscene-report.txt`. That pull was chasing a file the console will not hand over: a packaged run's report file lands `0600` and mounted *inside* the title's sandbox (D233, D237, D238), where ftpsrv and shsrv are a different user and get `Permission denied`. The system log is the one channel that leaves the sandbox as it goes, which is why the probe writes every record there unconditionally. `deploy` now captures that log concurrently across the launch instead of pulling the sealed file afterwards.

The four superseded scripts (`oops-deploy.sh`, `oops-klog.sh`, `oops-bootlog.sh`, `oops-crashhunt.sh`) are deleted; `oops-recover.sh` absorbed `crashhunt`'s recursive crash-dump find so recovery is one script behind one verb. `docs/WORKFLOW.md` (the run sequence), `CLAUDE.md` ("Which side runs what"), `scripts/README.md` and `docs/TOOLING.md` carry the verb list.

Status: **done**.

