# D204 - The layout verdict counted non-zero bytes while its dump counted changed bytes


**The layout verdict counted non-zero bytes while its own dump counted changed bytes, so a call that wrote zeroes reported both a full dump and "wrote nothing".**

`130-layout` was moved onto a poison basis (D-numbered earlier): the buffer is filled with a
poison pattern rather than zeroed, and `obs_report_written` reports a byte as written when it
*changed* from the poison, not when it is non-zero. This is the fix for the one blind spot the
section exists to avoid on hardware - a field the platform writes as zero being indistinguishable
from a field it never touched.

The dump was moved; the **verdict counter beside it was not.** `layout_report` still computed
`written` as the last non-zero byte, so the two disagreed on exactly the case the poison catches:

```text
OBS|bytes|130-layout/direct-memory-query|...|extent|256|      <- dump: 256 bytes changed
OBS|res  |130-layout/direct-memory-query|pass|0x11|...        <- verdict: 17 bytes "written"
```

On the host stub the buffer is filled entirely, so the dump reads 256 changed while the verdict
saw the last non-zero byte at offset 16 and reported 17. On hardware the failure is worse in the
other direction: a call that writes a real structure whose trailing field is a zeroed reserved
word gets `obs_fail("the call succeeded and wrote nothing")` - a false failure about a call that
demonstrably wrote, contradicting the dump on the same line.

### Fix

The verdict now counts `before[i] != buffer[i]`, the same basis as the dump. `written` becomes
the extent - the last byte the call touched - and "wrote nothing" now means "nothing changed
from the poison", which is the true statement. After the change the same call reports
`pass|0x100` (256), matching its dump.

The pass *value* shifts from last-non-zero to last-changed for these measurement checks. That is
a refinement of a field already documented as a weak measurement, not a format change: the record
shape and the field's meaning ("how much did the call write") are unchanged, and the two numbers
only ever differed when a written byte happened to be zero - the case this is correcting.

Status: **derived** - the inconsistency was observed live on the host build, and the fix makes
the verdict agree with the dump it sits beside.

