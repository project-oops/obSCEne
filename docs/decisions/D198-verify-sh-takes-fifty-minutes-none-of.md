# D198 - `verify.sh` takes fifty minutes, none of it work, and D012's rule had one more place to apply


Three background runs sat for fifty, twenty-five and six minutes. They were not hung - this is
simply what the gate costs on a repository that lives on a Windows 9p mount. Three separate
costs, measured rather than guessed:

### 1. The sink wrote its report across the mount - fixed

The sink's last-resort path is a bare `obscene-report.txt`, which lands in the **working
directory**. Every host run therefore wrote 36,000 records, unbuffered, across 9p:

| cwd | host run |
|---|---|
| `<OOPS>/obscene` | **57s** |
| `$HOME/obs` | **8.9s** |

D012 says `BUILD` must be Linux-local. The sink does not follow `BUILD` - it follows cwd - so
the rule had a second place to apply and nobody had applied it. All four host-run sites in the
Makefile now `cd $(BUILD)` first, which also stops a 2.9 MB report appearing in the repository
after every run.

### 2. Fifteen `cargo run` invocations - not fixed here

Each gate shells out to `cargo run`, and cargo stats the whole source tree over 9p before
deciding it has nothing to do. The gates themselves do almost no work:

```text
[  53s] === tool lints
[ 100s] === tool formatting
[ 170s] === cross-symbol guards
[ 239s] === capability ordering
```

Fifty to ninety seconds each, fifteen times: **twelve to twenty minutes of startup**. Building
the binary once and calling it directly removes essentially all of it, and is safe because
nothing about the tool is parameterised per gate.

### 3. Every build is a full rebuild, and that is probably correct

```text
make host (full rebuild): 272s
make host (no change):    284s
```

A no-op costs the same as a full one, because `host`, `module` and `payload` are all `.PHONY` -
make never checks freshness and always runs the recipe. Three targets is roughly fourteen
minutes of every run.

**Left alone deliberately.** These builds are parameterised by flags make cannot see in a file
dependency: `GEN`, `EXCLUDE`, `BULK`, `GPU`, `SERVE`, `HARDWARE`. Given a file target,
`make host GEN=4` followed by `make host GEN=5` would silently reuse the first binary and every
measurement after it would describe the wrong build. That is the exact failure this project is
built to avoid, and it is worth fourteen minutes.

Making it both fast and correct means a stamp file recording the flags, which is real
machinery; worth doing, not worth doing carelessly.

### Why this went unnoticed for so long

Because it never failed. A gate that is merely slow is indistinguishable from one that has
hung, and `verify.sh` prints its verdict only at the end - so a fifty-minute run looks exactly
like a wedged one for forty-nine of those minutes. `scripts/verify-wsl.sh` now streams each
gate with the time it started, which is how all three numbers above were obtained.

Status: **derived** - every figure measured on this machine today.

