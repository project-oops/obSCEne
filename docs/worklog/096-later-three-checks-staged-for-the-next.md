# 2026-08-31 (later) - three checks staged for the next hardware run


Console available again, so the questions the last run left open go on the wire now. Added, all
green under `make host` and the `guards`/`caps`/`counts` gates (171 checks, 36 sections):

- **`139-exports`** grew from two candidates to eight - getuid/geteuid/getgid/getppid (stable-id),
  sceKernelGetTscFrequency (returns exactly `0x5f259b8e`), sceKernelGetProcessTime (monotonic).
  Each confirms its vaddr by behaviour; the frequency one is the strong signature.
- **`130-layout/memory-type`** - allocates a span of each of WB_ONION/WC_GARLIC/WB_GARLIC and
  reads the query record's third word to decide whether that field is the memory *type* (the `3`
  seen on hardware was `WB_GARLIC`) or just state. On the host stub it reads a constant, so it
  correctly returns "state, not the type".
- **`130-layout/short-buffer-overrun`** - poisons past a short *declared* size (8/16/24/32) and
  checks whether the query wrote past it. The size ladder proved every size is accepted; this
  says whether a small one truncates or overruns. Per-declared-size sibling of the ladder's
  256-byte tail guard, mirroring the guard-word already on `110-modules/info-size`.
- **`120-measure/clocks-advance`** now emits each clock's absolute origin beside the busy delta.

Two surprises worth recording. First, an over-eager `sed -i` deleting `unsigned int refused = 0;`
by exact-line match also removed the identically-worded declaration and increment in
`check_direct_memory_query_flags` - a reminder that line-literal edits across a file hit every
match, not the one in view; restored both. Second, `counts --write` is a no-op without `--root ..`
(it defaults the root to the working dir), which read as "wrote, but drift persists" until the
missing flag was obvious. See D266. Unlike the previous entry's note, the host `make check` is
clean now - the parallel-session `selfdump.c` breakage is resolved.

