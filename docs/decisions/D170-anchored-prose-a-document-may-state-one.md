# D170 - Anchored prose: a document may state one checkable fact about the source beside the passage that depends on it


`doccheck` catches a document *naming* something which does not exist. It cannot catch one
*describing behaviour*, because behaviour is not a name and English is not diffable.

`docs/PROTOCOL.md` spent part of a day stating "It binds loopback by default" after that
change had been reverted from `net_posix.c`, which still read `INADDR_ANY`. Nothing failed; it
was found by accident while rewriting the section for an unrelated reason. That is the fourth
instance this week of a mechanism reporting something reasonable while being wrong, and the
only one of the four with nothing to compare against - D158, D163 and D168 were each caught by
diffing two independent readings of the same fact.

### Verifying the paragraph is not on the table; verifying what it rests on is

The loopback claim rested entirely on which constant `net_posix.c` hands to `htonl`. So a
document anchors a passage to a token:

```text
<!-- obscene:claim file=src/probe/net_posix.c contains=INADDR_ANY -->
- **It binds every interface**, because a console module must.
```

with `absent=` for the negative form. The gate says nothing about whether the prose is a good
description - only that the fact it stands on is still true. Reproducing the original failure
by flipping the constant back produces:

```text
docs/PROTOCOL.md:428 says src/net_posix.c contains INADDR_ANY, and it does not
```

### Opt-in, and that limit is the right trade

Coverage is whatever somebody bothered to anchor. A gate that inferred claims would produce
false failures, and **a checker that cries wolf gets switched off** - which `doccheck`'s own
documentation gives as the reason it matches only inside code spans, and which the first
version of `obscene-tool rows` nearly did by reporting 368 macro-generated rows as missing.

Four claims exist, all in the security posture: what the socket binds, that `write` and `blob`
need `OBS_NET_ESCAPE`, that the secret is generated at run time, and that no build-time secret
constant exists. Those are the passages a reader acts on.

A marker without `file=` is skipped rather than failed, so an ordinary HTML comment cannot
break the build. Markers inside a fenced block are examples and are skipped too - this entry
demonstrates the syntax above, and the first version of the parser read that demonstration as
a live claim. It happened to be true, which is the worst way to be wrong: a documentation
example that has quietly become a build dependency breaks the moment somebody edits it to
illustrate rather than to describe.

