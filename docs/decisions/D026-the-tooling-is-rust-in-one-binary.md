# D026 - The tooling is Rust, in one binary

**Status:** decided
**Date:** 2026-09-26

Every checker, generator and analysis command is a subcommand of `obscene-tool`, built from
`tool/`. There is no Python in the repository. The tool does not depend on the emulator it
measures; shared format knowledge comes from `selfish` (D200).

**Why:** most of the work is binary formats, where typed integers and `cargo test` beat untyped
scripts. One binary needs no runtime, and it matches the rest of the collection. Depending on the
emulator would make the probe measure that emulator's opinion of itself.

**Rejected:** Python scripts - a runtime dependency and untyped arithmetic on the formats. Reusing
the emulator's crates - couples the probe to what it measures.
