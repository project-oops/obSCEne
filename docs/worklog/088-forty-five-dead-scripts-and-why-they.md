# Forty-five dead scripts, and why they were not tests


`scripts/` held fifty-four ad-hoc probes and **not one was referenced** from the Makefile,
`verify.sh`, the docs or CI. Thirty-seven had no description of any kind. Nine were kept.

The cause is structural and worth recording, because it will recur: `wsl.exe -- bash -lc '...'`
mangles its argument under Git Bash, so the only reliable way to run anything is to write a file
and pass its `/mnt/c/...` path. **Every question asked therefore became a file.** The tell is in
the names - `phdr`/`realphdr`, `tags`/`tags2`, `preflight`/`preflight2`, `gh`/`gh2` - pairs
differing only in which file they point at, or in the second being the first one fixed. One
argument would have been all of them.

The right split, which is now in `scripts/README.md`:

- A question about a **file's contents** is a probe in `../selfish/crates/*/examples/`. Seven of
  those replaced roughly twenty-five of the deleted scripts, and are better in every way that
  matters: they take arguments, they compile in CI, and somebody can review them.
- A fact about **our own output** is a test. Every format defect found on hardware became one,
  which is why the knowledge survived the scripts being deleted.
- What crosses a boundary neither can - a real console, an uncommittable artefact, the WSL/Windows
  split - stays a script, and there are nine.

Nothing was lost by deleting them. Everything they established is in `DECISIONS.md`, in a test,
or in a probe.

