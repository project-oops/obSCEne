# Repo survey: litter, and a log nobody can read


Asked what else was worth rethinking. Four things, and only the last is architectural.

**133 hidden files, half a megabyte, in `src/shaders`.** `.fuse_hidden*` - what the VM's
mount leaves behind when a file is deleted while still open, which is what regenerating the
shaders does. Named with a leading dot, so `ls` never showed them and nothing in
`.gitignore` covered them. Deleted and ignored.

**A three-megabyte report in the repository root.** `obscene-report.txt`, the only file in
the root that nothing covered. Ignored now.

Credit where due: `.gitignore` already handled the rest - `display.png`, `tool/target`,
`reports/`, and the `_TempData`/`_Textures`/`_DownloadData` litter loaders drop when run
from here. The gaps were the two that were invisible.

**`DECISIONS.md` is 155 entries over 5,337 lines with no index**, and `CLAUDE.md` says to
read it at the start of every session. Nobody reads that; they grep it, which only works if
you already know the word - and the entries most worth finding are the ones you do not know
exist. `scripts/gen-decision-index.py` now generates one from the entries themselves, so it
cannot disagree with the log, and `verify.sh` fails when it drifts. Titles are read from
whichever style an entry uses: the heading for the early ones, the first bold claim for the
later ones.

Two things fell out of building it:

- **The anchors had to be computed, not assumed.** `#d001` is not where `## D001 - Two
  builds from one source tree` anchors to; the whole heading is slugified. A hand-written
  short anchor would have been a dead link for every entry in the early style - an index
  that looks navigable and is not, which is worse than none.
- **The file is not in numeric order and has not been for a long time** - D153, D152, D150,
  D148, D146, then D142 is one real stretch of it. That disorder is what let three appends
  in a single session collide with numbers already taken. The index sorts by number rather
  than reproducing the file's order, because a reader looking up D107 wants it where D107
  belongs.

