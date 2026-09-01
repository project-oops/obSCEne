# D125 - A gate is not trusted until it has been shown to reject something


Status: derived - the first version of this test proved the point by failing to.

`scripts/protocol-selftest.py` takes the transcripts that pass, breaks them one at a time
in ten ways the specification carries meaning through, and requires the checker to catch
every one. `verify.sh` runs it **before** it runs the checker.

This exists because the project has shipped two gates that could not fail: `verify.sh`
returned success four different ways regardless of outcome, and `lint.sh` printed `clean`
when clippy had not run at all. Both looked healthy for as long as nobody asked whether
they could say no.

The test earned its place immediately, twice:

1. **Its own first version caught nothing and reported success.** It pointed the checker at
   a mutated copy through an environment variable the checker ignored, so all eight runs
   re-checked the unmodified originals. A test that could not fail, testing a gate for
   whether it could fail.
2. **Once it worked, it found a real hole.** The checker allowed a command acknowledged and
   never answered whenever the file mentioned `died` anywhere - and `06-read.txt` refers to
   `03-died.txt` in a comment, which satisfied the substring search. A truncated transcript
   passed.

Fixing (2) made the contract stricter rather than looser: **every acknowledged command has
exactly one terminal reply, with no exception**, because the specification puts the burden
of recording a non-answer on the driver. There is no legitimate dangling `ack`.

The control matters as much as the mutations. The self-test first confirms the unmodified
transcripts pass; a checker that rejects everything would otherwise "catch" all ten and
look perfect.

