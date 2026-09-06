# D315 - a document beside a Makefile means that Makefile

**decided** - 2026-09-03

`doccheck` reported

```text
src/porthole/README.md: `make elf` - no such rule
```

`make elf` is real. It is rule 61 of `src/porthole/Makefile`, and the README says which
directory to run it in two lines above:

```markdown
# Inside src/porthole/
make elf
```

The gate resolved every `make <rule>` against the root Makefile alone, so a document beside its
own Makefile could not name its own rules. **A negative from a gate that only looked in one
place is a fact about the gate.** Third instance this week, after a probe reading back its own
initialiser (D303) and an index gate reading a file shape the log no longer has (D314).

Rules are now resolved against the root Makefile **and** the Makefile in the document's own
directory, if there is one.

## The alternative, and why not

Require the document to write `make -C src/porthole elf`, so every rule resolves from the root.
That makes the gate simpler and the documents worse: a reader who has followed the instruction
to `cd src/porthole` is then told to run a command that would work from anywhere, and the two
readings of the same page disagree. The document is right; the gate was wrong.

## Watched failing

Appending `make definitely-not-a-rule` to that README is still caught, so the fix widened the
rule set without disabling the check. Restored afterwards.
