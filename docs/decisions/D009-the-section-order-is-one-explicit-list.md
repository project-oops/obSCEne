# D009 - The section order is one explicit list

**Status:** decided
**Date:** 2026-09-26

`src/probe/registry.c` holds the sections in running order as one array. Section identifiers
carry a numeric prefix that carries the layering, and the report contract requires the prefixes
to ascend.

**Why:** the order is part of the report's value - base layers first, so a video failure is read
against a memory subsystem already known to work - and it should be visible in one file. The
ascending prefix makes the intended order checkable.

**Rejected:** self-registration through linker sections or constructors - tidier to extend, and
produces an order nobody can read.
