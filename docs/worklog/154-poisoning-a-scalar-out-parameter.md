# 2026-09-03 - poisoning a scalar out-parameter, and a list of what orbistoun cannot decide

Two changes, both from the consuming side. orbistoun read this project's report and found
twenty-four measurements that measured nothing.

## The probe was reading back its own initialiser

`106-encoder/path-probe` reports `res` after each `sceKernelLoadStartModule`, and `res` starts
at zero. Every one of the twenty-four came back `0x0` - which is what a platform that never
touches the out-parameter produces, and equally what one that writes zero produces.

Now `(int)0xC7C7C7C7u`, the pattern byte `obs_layout_patterns` already uses. **Untouched is a
visible answer.** D303 has the argument and the residual.

The general shape is the interesting part: `obs_report_written` has always made this case for
buffers, and nothing made it for scalars. Swept into the backlog rather than fixed one site at
a time.

## And a backlog entry written from the other side

`docs/backlog/022-measurements-orbistoun-is-blocked-on.md`. Four entries, each with what would
settle it:

- **`scePthreadSetprio` / `scePthreadSetaffinity`** - census-only, no arity. The unblock is a
  lawful reference confirming the FreeBSD analogues, not a console run.
- **What `sceKernelLoadStartModule` does to a module already placed** - needs hardware, and the
  run that settles it is loading a module this project built and reporting whether anything of
  its own ran. "Nothing ran" is as useful an answer as a name.
- **The width of a vendor error return** - every probe reports `(uint64_t)(uint32_t)x`, so the
  upper thirty-two bits are the cast. One assembly thunk reporting `rax` verbatim retires a
  caveat repeated at every comparison site.
- **The out-parameter sweep above.**

Written here rather than in orbistoun because the side that owns a console is the side that can
close them, and a gap nobody wrote down is one the next capture will not close.

## And the decision gate has been failing, checking nothing

Found while reading this repository's own report. `obscene-tool decisions` built the index for
a single-file `DECISIONS.md`; the log was split into one file per entry long ago and the
subcommand was never repointed, so it has been printing `no entries found` and exiting 1. A
gate in `verify.sh` that checks nothing, for weeks.

Rewritten to check what is actually here - every entry has exactly one row, every row a real
file, no number claimed twice - and to **refuse an empty directory**, because a gate with
nothing to check has not passed. D314.

Watched failing before being believed, and both failures were worth having. Hiding one entry
gives `D303-...md: no row`, exit 1; restoring gives consistent, exit 0. The first parser
matched the bare string `decisions/`, which the front matter uses to *describe* the layout, and
ran to the next `)` - a "link" six rows long. The unit test written for exactly that caught it.

Numbered 314 rather than 304: worklogs 136-145 cite the ten numbers from 304 upward in their
titles with no files behind them, and writing into that range would give those citations a
wrong target. `doccheck` has been reporting the gap all along.

## And a second gate looking in only one place

`doccheck` reported `src/porthole/README.md: make elf - no such rule`. It is rule 61 of
`src/porthole/Makefile`, and the README names the directory two lines above. The gate resolved
every rule against the root Makefile alone, so a document beside its own Makefile could not
name its own rules. Now resolved against both. D315.

Same shape a third time: a negative from a gate that only looked in one place is a fact about
the gate. Watched failing - a fake rule appended to that README is still caught.

## Eight rows in the index had no title at all

Visible the moment the gate was repointed and the index regenerated:

```text
| 🟢 | D302 | [](decisions/D302-the-conformance-matrix-and-its-naming.md) | decided | 2026-09-03 |
```

The splitter reads a title from `# D<NNN> - Title` and nothing else. Nine entries had been
written `# D302: Title`, so nine rows rendered as a bare link with no text - an index of
unlabelled links, which is the one thing an index must not be. Headings normalised, index
regenerated, **eight empty titles to zero**, and the gate now refuses an untitled row so it
cannot come back quietly.

## State

Host build clean: `exit=0`, zero errors or warnings under `-Werror`. Identity scan clean.
Tool builds, 5 gate tests pass, and the gate reports 302 entries consistent.

clippy on `tool/` still has **5 pre-existing errors, all in `main.rs`** (`redundant_closure`,
four `collapsible_if`) and 7 warnings. None are in the rewritten file, and they are the
standing question with the user rather than this unit of work.

Nothing committed. The day holds D302, D303, D314, D315, worklogs 153-154, the `matrix`
subcommand, the rewritten `decisions` gate, the `doccheck` fix, the MODULE-FORMAT.md note, and
backlog 022.
