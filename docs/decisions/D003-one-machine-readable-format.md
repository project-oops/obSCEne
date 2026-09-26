# D003 - One machine-readable format; presentation lives in the tool

**Status:** decided
**Date:** 2026-09-26

The probe emits pipe-separated `OBS|` records and nothing else. `obscene-tool pretty` renders
colour, grouping and alignment for a person.

**Why:** the primary reader is a tool diffing runs, to which escape sequences are noise. ANSI
handling and column arithmetic inside a freestanding program that must survive a half-finished
emulator buy nothing the tool cannot do better.

**Rejected:** colour and layout in the binary - more code in the one place that has to stay small
and robust.
