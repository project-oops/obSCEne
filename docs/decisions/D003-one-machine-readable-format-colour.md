# D003 - One machine-readable format; colour lives in a script

**assumed** · 2026-08-19

The binary emits pipe-separated records and nothing else. `tools/pretty.py` renders
colour, grouping and alignment.

The request was for red, amber and green grouped into sections, and that is what a
reader gets - from the pretty printer. Putting it in the binary would mean ANSI
handling, terminal detection and column arithmetic inside a freestanding program that
has to survive running in a half-finished emulator, in exchange for nothing the
script cannot do better. The stated primary consumer is an agent diffing runs, and
escape sequences are noise to it.

One format, one parser, colour as presentation.

