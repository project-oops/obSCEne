# D016 - The census lists are fenced off from clang-format

**decided** · 2026-08-19

`surface.h` wraps its X-macro lists in `clang-format off` / `on`, and writes one
symbol per line.

clang-format reads `X(name) X(name) X(name)` as a chain of function calls and reflows
it into a staircase of ever-deeper continuation indents. Two things make that
unacceptable rather than merely ugly:

- **It is not idempotent.** Formatting the file and then checking it still reported
  violations, so the format gate could never pass. That is a broken gate, not a style
  disagreement.
- **It destroys the diff.** Re-wrapped lists mean adding one symbol rewrites twenty
  lines, and the census is expected to keep growing.

One symbol per line makes adding or removing one a one-line diff, which is worth more
than the horizontal density it costs. `tools/gen-surface.py` regenerates the file, so
the mechanical part stays mechanical - but the header remains the committed source of
truth and editing it by hand is fine, as long as the fences stay.

