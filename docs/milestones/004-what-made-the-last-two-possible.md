# What made the last two possible


Worth recording because it is the transferable part.

**A sweep inside the probe instead of a rebuild per guess.** One refusal code from a
five-argument call says which step failed and nothing else. Finding out by editing a constant
costs a package build, an install and a launch - about five minutes, with a healthy console
needed for each. `085-videobuf` varies one argument per call and reports each code, so the
whole table comes out of one run:

```text
baseline        0x80290015
format=0        0x80290003     <- the argument is read, and the baseline value is fine
aspect=1        0x80290008     <- so is this one
720p            0x80290015
```

Two codes moving is what proves the attribute is parsed *and* that our values pass - which is
what made it possible to stop varying the attribute and go and vary the allocation instead.
Neither half would have been enough alone. (D251, D252)

**A second source.** Three findings recorded as facts about the console turned out to be facts
about this program, and in every case a second source existed and had not been consulted:
`~/oracle/uroot/eboot.bin`, a title that launches. Every question put to it this session was
answered in one command.

---

