# D192 - A project that reads the others is not another vote, and prosper says so in its own comments


prosper was added to the toolkit on the argument that a fourth mature implementation would
promote several one-source constants to corroborated without needing hardware. Reviewing it
before running it showed that argument was wrong, which is the whole reason to review before
counting.

**It cites Kyty and shadPS4 by name, throughout.** A structure is described as *"0x50 bytes,
Kyty layout"*; a setter *"mirrors Kyty's"*; an error value is sourced to *"Kyty Errno.h"*. Where
it agrees with Kyty, that is very often the same reading arriving twice, and treating it as two
would be **double-counting the first**.

### The rule this sets for the whole toolkit

Agreement between implementations is only evidence when the implementations are independent,
and independence is a property to check rather than assume. Five loaders in a table look like
five readings; some of them are one reading with four citations.

So corroboration is counted **per claim**, not per project, and a claim is worth what its
provenance says:

| what the source says about a claim | worth |
|---|---|
| annotated, citing FreeBSD or another published contract | strong - a citable source, the standing `SPEC` has |
| annotated, citing a live capture of a retail title | real - guest-observed, the platform's own software |
| bare constant, no annotation | no better than ours |
| explicitly derived from another project here | **not independent; do not count** |

prosper makes this practical by grading itself - 40 `CONFIDENCE: HIGH`, 13 `MED`, 4 `MED-HIGH`,
3 `LOW`. That is the same instinct as this project's provenance ladder and orbistoun's
`known_by`, and it is what makes the source usable rather than merely present.

### What the review produced anyway, which is more than agreement would have

**A named disagreement on mutex type 4.** Both accept exactly `{1,2,3,4}` and refuse `0` - so
the *shape* of D177 is corroborated and the POSIX numbering is ruled out twice. But Kyty maps 4
to `NORMAL` and prosper maps it to `ADAPTIVE_NP`, deliberately:

> *"Weight Kyty DOWN here: no PS4 title it runs exercises adaptive self-lock; FreeBSD libthr is
> the platform contract. CONFIDENCE: HIGH (FreeBSD source + the live wedge -> unwedge flip)."*

`015-sync/mutex-recursion` already reports second-acquisition behaviour per type, so it
distinguishes these: a type returning `EDEADLK` on self-lock is a different observation from
one that succeeds. **A specific contested value one hardware run settles is worth more than
another agreement**, and this project had no way to generate one before.

**Independent confirmation that `EINVAL` is right.** prosper returns `0x16` for an unrecognised
type where Kyty calls `EXIT`. That is what POSIX specifies and what
`patches/kyty-probe-friendly.patch` changes Kyty to do - reached without reference to that
patch, so the patch is a correction rather than an accommodation.

**The user model has a corroborated shape and an uncorroborated value.** Capacity four with
`-1` for empty now holds across prosper, PS5PCEM and shadPS4. The initial user's *value* still
has four implementations giving three answers, and prosper's `1` is unannotated. That split -
shape settled, value open - is exactly what `HANDOVER-ORBISTOUN.md` asks orbistoun to build
against.

Status: **decided** - the counting rule follows from what the source says about itself.

