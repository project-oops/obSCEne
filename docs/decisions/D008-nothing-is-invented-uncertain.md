# D008 - Nothing is invented; uncertain signatures are omitted

**decided** · 2026-08-19

Where an arity or a constant is uncertain, the function is left out.

A wrong arity is not a compile error, it is a corrupted stack and a crash somewhere
unrelated. A wrong constant is worse: the call succeeds and does something other than
what was asked, and nothing ever says so. Both cost far more to diagnose than the
declaration saves, and this program's entire value is that its report can be trusted.

Adding one is a matter of confirming the signature and writing three lines.

