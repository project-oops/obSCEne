# D248 - Imports are encoded with global binding

**Status:** decided
**Date:** 2026-09-26

The probe declares platform functions `OBS_WEAK` in C so an absent symbol is null rather than a
link error, but the module writer (in selfish) re-encodes each import's binding as `STB_GLOBAL`
when it re-encodes the type as `STT_FUNC`.

**Why:** a `STB_WEAK` undefined import tells a loader not to resolve it if resolution costs
anything, so libraries that were mapped bound nothing. The weak binding is a compile-time need and
belongs in the C, not in the module. Six symbols the platform lacks stay null and their checks
skip, which is the outcome weak was chosen to give.

**Rejected:** carrying the weak binding into the module - fourteen imports the console had went
unbound.
