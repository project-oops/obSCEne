# The bridge, and a contract change that went out unannounced


`<shared>\obscene-orbistoun-bridge.md` already existed - a shared log between the two
sessions, set up so neither has to relay by hand. I had reported that nothing was shared
with the sibling; that was wrong, and reading the log before posting is what caught the
rest of this.

**A value the consumer renders changed this morning without being announced.** D147 replaced
`sysinfo|generation` values `5 (current)` / `4 (previous)` with `5 (agc)` / `4 (gnm)`. The
consumer's own bridge entry says it renders `4 (previous)` explicitly, so the change lands
on their display. Under the open-enum rule the two sides agreed, an *added* value is safe and
a *changed* one is exactly the case that rule does not cover - so it needed saying and did not
get said.

Two documents were stale with it, both of them contracts rather than prose: `docs/OUTPUT.md`,
which is the format contract the consumer parses against, and `docs/CLIENT.md`, which is the
copyable client guide. Both still documented the retired strings. Changing a value in
`sysinfo.c` and leaving the contract describing the old one is the same drift the audit found
this morning, committed hours after finding it.

**The renames crossed a boundary the new gate cannot see.** `doccheck.py` now fails on a
pointer to a missing document, which is what made the `HANDOVER-ORBISTOUN-NET.md` →
`CLIENT.md` rename safe inside this repo. The bridge file lives under `<shared>` and is owned
by neither repo, and its own instructions still name the old path. Flagged in the log rather
than edited, since neither side rewrites the other's text - but worth recording that a gate
stops at the repository boundary and cross-repo references do not.

Also answered a question the consumer left open: `sysinfo|listening` is the server's state in
a record kind framed as the target's account of itself. Taking their lighter option - the
contract now says a consumer rendering machine identity may filter it, and why - rather than
adding a record kind for one field.

