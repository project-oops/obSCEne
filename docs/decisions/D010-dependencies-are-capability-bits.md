# D010 - Dependencies are capability bits, not check names

**Status:** decided
**Date:** 2026-09-26

A check declares what it requires and what a pass provides as capability bitmasks. The harness
grants capabilities in running order. `obscene-tool caps` reports any requirement that nothing
earlier in the registry can grant.

**Why:** a mask compare needs no strings and no allocation in a freestanding binary, survives a
rename, and states the real dependency: a check needs memory to work, not a particular check to
have passed.

**Rejected:** naming prerequisite checks - string handling in the probe, and a dependency that
breaks silently on a rename.
