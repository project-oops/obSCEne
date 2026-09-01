# `scripts/`

Orchestration only. This directory once held **fifty-four** scripts and forty-five of them were
dead; keeping the count low is the point.

## What belongs here, and how it is reached

Something that **crosses a boundary a test cannot** - the WSL/Windows split, or a real console.
Each of these is the implementation behind exactly one `bin/obscene` verb. **That verb is the
single entry point; none of these is run by its path directly** (D269):

| script | verb | what it crosses |
|---|---|---|
| `oops-rebuild-pkg.sh` | `deploy` | build in WSL, install/launch from Windows, capture the report |
| `payload-run.sh` | `payload` | build the plain-ELF payload, run it via elfldr, capture the raw log |
| `oops-recover.sh` | `recover` | ask a real console what it recorded, read-only |
| `oops-hwsweep.sh` | `hwsweep` | iterate against hardware, excluding each call that hangs |
| `oops-minbuild.sh` | `minbuild` | build the minimal diagnostic package |
| `oops-digcheck.sh` | `digcheck` | check a built package agrees with its own digests |
| `oops-prep.sh` | `prep` | bring the readable services (klogsrv/shsrv) up |

The report itself is no longer a script. `./bin/obscene report` (`obscene-tool report`) captures
obscene's records from the console system log, because that is the one channel that leaves the
title's sandbox (D233); `deploy` calls it across the launch. It replaced `oops-klog.sh` and the
`hw pull /data/...` half of the old deploy scripts, which fetched a file the console will not hand
over.

Two scripts here are cross-repo rather than obSCEne's own, so they belong to `oops`, not a verb:
`oops-ci.sh` and `oops-selfcheck.sh` run the gates across both `selfish` and obSCEne. `oops check
selfish obscene` and `oops all` are the thorough form of the same thing.

Everything here takes arguments or reads the environment. A script that hardcodes one path is a
script somebody will copy and edit rather than reuse.

## What does not

**A question about a file's contents.** Those go in `../selfish/crates/*/examples/` as Rust
probes - `container_entries`, `tag_diff`, `tag_values`, `symbol_names`, `relocs`, `procparam`,
`tags_probe`. They take file arguments, they compile in CI, and they are reviewable. Seven of
them replaced roughly twenty-five of the shell scripts that were deleted.

**A fact about our own output.** That is a test. Every format defect found on hardware in this
project became one: `a_data_entry_sizes_itself_by_its_data_and_never_by_p_memsz`,
`an_executable_declares_no_export_library_and_a_shared_library_does`,
`the_roots_parent_is_itself_and_not_the_super_root`, and the derivation relations in
`tool/src/derive.rs`. The shell script was only ever the runner.

## Why forty-five accumulated, which is not a moral failing

`wsl.exe -d Ubuntu -- bash -lc '...'` mangles its argument under Git Bash (see the repository
`CLAUDE.md` on `MSYS_NO_PATHCONV`), so the reliable way to run anything at all is to write a file
and pass its `/mnt/c/...` path. **Every ad-hoc question therefore became a file**, and nothing
swept them.

The tell is in the names: `phdr`/`realphdr`, `tags`/`tags2`, `preflight`/`preflight2`, `gh`/`gh2`
- pairs differing only in which file they point at, or in the second one being the first one
fixed. Thirty-seven of the forty-five had no description of any kind.

If you are about to write `oops-<noun>.sh` to look at a field in a file: add an argument to an
existing probe in `selfish/crates/*/examples/` instead. If you write one anyway because you are
mid-hunt and it is faster - that is fine, it usually is - delete it in the same change that
records what it found.

## Build caching (`sccache`)

`oops-rebuild-pkg.sh` uses [`sccache`](https://github.com/mozilla/sccache) when it is on `PATH`,
and says so on the first line (`cache: sccache <version>` or `cache: off`). It needs no flags -
the cache only ever helps a rebuild - and `--no-cache` turns it off for the rare case of ruling
it out.

**What it caches.** sccache caches both halves now: the **Rust** crates (selfish, the tool)
through `RUSTC_WRAPPER`, and the **C** objects, because the Makefile compiles each source to its
own `.o` (see the per-file object rules and their `.flags` sentinel there). A repeated clean C
build with the same flags comes back from the cache in seconds instead of recompiling the
thirty-five-thousand-symbol census. `ccache` would do the C half but not Rust, which is why
`sccache` is the choice.

**`-j` compiles the objects in parallel**, so even a cold full rebuild uses every core. The
script sets `CARGO_INCREMENTAL=0` for its own cargo calls only, because sccache declines to
cache an incremental build; an interactive `cargo build` in a checkout keeps cargo's incremental
compilation and is untouched.

**The everyday win is the Makefile's per-file compilation, not the cache.** A one-file edit
rebuilds one object (~1.5 s) rather than recompiling everything. sccache is what makes a *clean*
rebuild and a branch switch cheap on top of that; the two compose.

Install (no sudo): `cargo install sccache --locked`. `sccache --show-stats` reports hit rate.
