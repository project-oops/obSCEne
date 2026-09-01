# D064 - A correction to D063: fpPS4's table is derived from ps4libdoc, not independent of it


Status: correction, same session.

D063 called fpPS4's 78,372-pair table an independent confirmation and set it beside the
earlier 388-of-389 comparison against `ps4libdoc` as though they were two witnesses.

They are one. fpPS4's table contains **42,009 of ps4libdoc's 42,010 names**, plus 36,363
more; the unit is even called `ps4libdoc`. It is a superset of that database, not a
separate account of it.

Counting a source twice is exactly the error this project keeps guarding against
elsewhere, and it was made here while writing up how well corroborated something was.

### What the agreement still proves, stated precisely

`ps4libdoc` is not a name list somebody wrote down. It publishes symbol tables extracted
from real console firmware, one branch per system version, and each symbol carries the
identifier **observed in the binary** alongside a name where one has been recovered.
Recovering a name means hashing candidates until one matches an observed identifier.

So the anchor is observation, and the chain is:

1. an identifier appears in a real firmware binary;
2. somebody recovers a name whose hash matches it;
3. fpPS4 carries that pairing;
4. our hash of that name reproduces that identifier, 78,372 times out of 78,372.

That is worth having. It says this project's hash agrees with values observed on real
hardware, transitively, across a very large sample - and it rules out the whole class of
byte-order, alphabet and digest-truncation mistakes.

**What it does not say** is that three independent parties agree. Two of the three are one
source. The honest count of independent confirmations is: the published test vector, and
this database. The suffix's own self-verification (ACKNOWLEDGEMENTS.md) remains the reason
it is trusted at all.

### A data artefact worth knowing about

The one ps4libdoc name absent from fpPS4's table is `﻿__absvdi2` - a byte-order mark
on the first line of `known_names.txt`, not a symbol. Anything reading that file should
strip it; this project's reader did not, which is why the count came out as 42,009 of
42,010 rather than a clean 42,010 of 42,010.

