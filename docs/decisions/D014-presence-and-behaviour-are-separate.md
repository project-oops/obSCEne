# D014 - Presence and behaviour are separate questions

**Status:** decided
**Date:** 2026-09-26

Behavioural sections ask whether a function works and cost a confirmed signature each. The census
(`900-surface`) asks only whether a name resolves, and declares every censused name as
`const char`, so the type system rejects calling one. A name is either censused or declared as a
function in `platform.h`, never both.

**Why:** presence scales to tens of thousands of names at no risk; behaviour does not. A wrong
census name reports absent, a visible and harmless false negative, while a wrong behavioural
signature crashes. Declaring census names as data makes the rule hold without anybody remembering
it.

**Rejected:** one list serving both questions - either the census stays tiny or unconfirmed
signatures get called.
