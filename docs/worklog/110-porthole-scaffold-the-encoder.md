# 2026-09-01 - Porthole scaffold + the encoder-reachability section (D280)


Scaffolded the Porthole payload in porthole/ (tracer-style isolated subtree): the wire contract is
real (24-byte PPAD input record + host self-test), the three real operations are gated stubs. Builds:
`make -C porthole check` (host contract) and `make -C porthole skeleton` (freestanding target) both
green. Added 106-encoder, a presence-only reachability section for libSceVencCore's key entry points -
the first half of Porthole's go/no-go - following 105-record's discipline (declare as data, never call
an unconfirmed arity; the harness's skip-if-unresolved is the measurement). Runs on host: three skips,
correct (no encoder there). encoder.c + registry both compile clean.

Surprise worth recording: `make host` is currently broken, but not by any of this - the untracked
src/common/ (the D278 injector's freestd helpers) includes "common/freestd.h", which -Iinclude cannot
find. Verified my additions build in isolation and under an -Isrc override; left src/common/ alone as
the injector workstream's. Flagged in D280.

