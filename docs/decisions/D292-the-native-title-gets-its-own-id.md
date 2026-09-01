# D292 - the native title gets its own id, distinct from the package


D287 had the native title reuse the package's id, so the two artifacts were one app. That is right
when only one is ever installed, and wrong the moment both are: the ps4 package and the ps5 native
title each register under their title id, and a shared id makes the second install collide with the
first (observed - a package install of OBSC00001 claimed `/user/app/OBSC00001` as a download-stub and
a native title of the same id could not take it).

So `data/identity.toml` now carries two ids: `content_id` for the package (build-pkg.sh) and
`content_id_native` for the native title (build-native.sh). Each build derives its own title id from
its own content id - `OBSC00001` for the package, `OBSC00002` for the native title - so the two are
distinct and can be installed side by side for A/B testing. Both still live in the one file; neither
id is written twice (D288 holds - the fix there was one *home*, not one *id*).

**The uniqueness is obSCEne's, not selfish's, and deliberately so.** The natural request is to have
selfish emit a different id for `native` than for `pack`. It must not: selfish is a generic tool that
takes the id as an argument and has to emit exactly that id, and a tool that silently varied it would
be inventing identity - the one thing the provenance rule forbids, and a miserable thing to debug
when a title registers under an id nobody set. So obSCEne derives the two ids and hands each to
selfish explicitly; selfish stays faithful.

The `sed` readers do not cross-match: `^content_id[[:space:]]*=` stops at the `_` in
`content_id_native`, and `^content_id_native` needs the suffix the plain line lacks. Verified: the
native build yields `titleId OBSC00002`, the package reader yields `OBSC00001`.
