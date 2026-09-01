# D011 - The synthetic error code is 0xDEADBEEF, returned unmodified

**decided** · 2026-08-19

The host stubs return a value that can never be mistaken for a real platform error
code. Real ones begin `0x80` or `0x81`.

**Recorded because the first attempt was wrong in an instructive way.** It defined
`0x7F000001` - high bit deliberately clear - and returned the *negation* of it,
reasoning that a clear high bit meant the value could not be confused with a real
error. Negating it produces `0x80FFFFFF`, which is precisely the shape of a real
error code. The mistake was visible in the very first host run and invisible in the
source.

The value a caller observes is what matters, not the value written in the source.

