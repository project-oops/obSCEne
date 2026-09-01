# A cracker, and the words that kept getting confused


Four things sound like each other and are not:

| | what it is | reversible |
|---|---|---|
| hashing | name to eight bytes | no |
| encoding | those bytes to eleven printable characters | yes |
| decoding | those characters back to the bytes | yes |
| cracking | guessing names and hashing them until one matches | the only way back |

`decode` already existed and is the one that misleads - it opens the envelope and leaves
you holding the shreds. Its own doc comment says so, which suggests this confusion is
older than today.

**`obscene-tool crack` is built** (D049). Given a list of NIDs and a list of candidate
names it hashes each candidate once and reports the matches, with a header recording the
suffix, how many candidates were tried, and how many known pairs the list reproduced.

**Self-test:** fed the 389 harvested pairs as both targets and candidates, it recovers
389 of 389 and reports the generator as reproducing 389 of 389. The corpus is now
`data/nid-corpus.txt` with its provenance written into the file.

That corpus is worth more than it looks. Every pair in it was produced by somebody else's
implementation of the same hash, and all 389 agree with ours - an independent check on the
whole chain, obtained from log output that cost nothing.

**What is deliberately absent:** a candidate generator. Describing the naming convention
is the actual problem and it wants iterating on, so it belongs in a script that emits a
word list rather than compiled into the tool. The tool does the fast exact part.

---

