# D009 - The section order is one explicit list

**assumed** · 2026-08-19

`src/registry.c` holds an array of section pointers in running order, rather than
sections self-registering through linker sections or constructors.

Self-registration is tidier to add to and produces an order nobody can read. The
running order is the whole value of the report - base layers first, so a video
failure is read against a memory subsystem already known to work - and it should be
visible in one file without knowing how the linker feels that day.

Ordering is also verified: section identifiers carry a numeric prefix and
`tools/verify.py` fails a report whose sections are out of order, so the intent is
enforced rather than merely documented.

