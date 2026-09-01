# 3. Verifying the NID mapping - done, and far better than hoped


`obscene-tool crack` exists (D049), `data/nid-corpus.txt` holds 389 pairs harvested from an
emulator's own resolution log, and the hash chain has been checked against a large external
database.

**78,372 of 78,372 pairs agree** (D063). Those identifiers were extracted from real hardware
firmware rather than computed, so reproducing them is agreement with hardware values across
a large sample - it rules out every byte-order, alphabet and digest-truncation mistake at
once.

**With one correction.** The two databases used are not independent of each other: fpPS4's
table is a superset of `ps4libdoc`, sharing 42,009 of its 42,010 names. Presenting them as
two witnesses was wrong (D064). The honest count of independent confirmations is two: the
published test vector, and this database.

What is still absent by choice is a **candidate generator**. Describing the naming
convention is the actual problem and it wants iterating on, so it belongs in a script that
emits a word list rather than compiled into the tool.

