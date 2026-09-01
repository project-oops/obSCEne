# D158 - Two relational checks had never run anywhere, and the report said so in words that read as a platform limitation


`018-relational/descriptors-distinct` and `018-relational/close-is-not-idempotent` both
declared `OBS_CAP_FILE` as a prerequisite. `harness.c` grants capabilities **in running
order**, starting from nothing, and the check that grants `OBS_CAP_FILE` lives in
`040-file` - twenty-two sections after `018-relational`. Neither check had ever executed a
single instruction, on any target, host build included, from the day it was written.

### Why nobody noticed

They did not fail. They reported

```text
skip: a prerequisite capability was not established
```

which is exactly what a platform genuinely without a filesystem would report. The message
was true. It was about the wrong thing, and there is no way to tell the two apart by reading
the report - which is why it survived every reading of every report it appeared in, on four
loaders.

This is the same failure shape as the fpPS4 record count and `gen-surface.py` refusing its
own definition: a mechanism stopped working, said something reasonable while it did, and the
reasonable thing it said was indistinguishable from a real result.

### The fix is to delete the requirement, not to reorder the sections

The capability was redundant as well as fatal. Both bodies open the file themselves and skip
with a better message when they cannot - `nothing could be opened to compare` says which
question went unanswered, where the capability message only says something upstream did not
happen. Moving `018-relational` after `040-file` would fix these two and break the section's
reason for sitting where it does: the relations are read after the value checks they compare
against, and the sections it actually exercises are 010 through 017.

### `obscene-tool caps`

A gate, because this class of mistake is silent by construction and the repository now has
several instances of "a thing that quietly stopped counting". It walks the registry order,
accumulates `provides`, and reports any `requires` that nothing earlier can satisfy.

It assumes the **best case** - every check reached grants what it claims - because the
harness only grants on a pass and a gate cannot know what passes. So it reports only
requirements that are unsatisfiable *even then*, which is precisely the set that is a
build-order mistake rather than a result about a platform.

Section order is read from `registry.c` rather than sorted by the numeric prefix. The
prefixes ascend today; a gate that assumed they always would is one that stops working the
first time someone reorders the list without renumbering, silently and in the direction of
passing. Rows are taken in source order within a section, because the table is an array and
the harness walks it - `sections::rows` sorts, which is right for counting and wrong here.

Run against the tree with the requirement put back, it names the grant site:

```text
018-relational/descriptors-distinct wants OBS_CAP_FILE - granted later, by 040-file/open-rejects-missing
```

