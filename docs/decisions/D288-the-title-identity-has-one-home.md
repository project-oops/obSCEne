# D288 - the title identity has one home: data/identity.toml


The title/content id was written in two places - `build-pkg.sh` and `build-native.sh` - once D287
made the native title reuse the package's id. Two homes for one fact is the exact failure the format
tables were moved to selfish to avoid (D200): change one and the two artifacts quietly become
different apps under different ids, and nothing catches it until a console shows two icons or refuses
an install for a mismatch.

`data/identity.toml` is now the single home. It holds `content_id` (the one fact) and `title`. The
title id (`OBSC00001`) is the content id's middle field, which each build derives with the same
`${x#*-}` / `%%_*` split, so the title id is never written independently of the content id it lives
inside. Both scripts read the two strings with a one-line `sed` and take their defaults from them,
preserving the existing `CONTENT_ID=` override (a stuck title holding its id; D223).

**Why toml and not a sourced `.sh`.** The first version of this was `scripts/identity.toml`'s
predecessor, a `scripts/identity.sh` the builds `source`d. That was expedient - a shell file needs no
parser, and the only consumers are shell - but it was wrong on the project's own terms: an identity is
*data*, and `scripts/` is orchestration, not data (data lives in a data format - selfish already keeps
`pkg-keys.toml`). A sourced `.sh` is also executable, so it is a larger surface than an inert value,
and it cannot be read by a non-shell tool. toml is the format this codebase already uses for data, and
a flat `key = "value"` file is read with a `sed` one-liner - no dependency, and this repository has no
Python to reach for anyway.

**selfish does not read it, deliberately.** The identity is obSCEne's, not a property of the format,
and selfish is a generic tool shared by several projects - so the ids reach it as
`--content-id`/`--title-id` arguments, passed by obSCEne's scripts. `param.json` is not the home
either: selfish generates it *from* the id, so it is the output, not the source.

Verified: both scripts read the one value (`build-native.sh` default yields `titleId OBSC00001` /
`titleName obSCEne`; `build-pkg.sh` resolves the same `content_id` and `title`), an override flows
through to the derived title id (`CONTENT_ID=IV0002-TEST00099_00-...` -> `titleId "TEST00099"`), and
both scripts pass `bash -n`.
