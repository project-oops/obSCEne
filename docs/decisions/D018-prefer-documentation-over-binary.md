# D018 - Prefer documentation over binary extraction when sourcing names

**assumed** · 2026-08-19

Symbol names are taken from published interface documentation, specifications, and
hand-written SDK headers. Names recovered by dumping decrypted vendor libraries are
**not** used, even though the resulting string is identical.

The distinction is provenance, not the value. A name is a fact about an interface and
`strlen` is `strlen` wherever it is read. But the project rule bans material derived
from decrypted vendor binaries, and "we only took the names" is exactly the argument
that rule exists to refuse - the same reasoning would justify taking struct layouts
next, and then constants, each step looking small.

Practically this rules out generated stub lists. The mainstream community SDK for the
current-generation console builds its symbol stubs by processing decrypted `.sprx`
files; the names are correct and the chain of custody is not one this repository can
adopt. Where a name is genuinely only available that way, the honest position is that
this project does not have it yet.

**Recorded as an assumption because it is a judgement about someone else's rule.** It
is a stricter reading than "names are facts", and a looser reading would be defensible.
It should be confirmed rather than left to drift.

