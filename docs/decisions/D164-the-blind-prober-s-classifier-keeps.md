# D164 - The blind prober's classifier keeps privileging `0x8002`, and the facility table stays out of the probe


`classify()` in `src/sections/bulk.c` buckets a return value four ways, and the middle test
is a hardcoded constant:

```c
if ((returned & 0xFFFF0000ull) == 0x80020000ull) {
    return "rejected";
}
```

Everything else with the high bit set becomes `error-shaped`, which `docs/OUTPUT.md`
describes as "another facility, or an errno returned directly".

The sweep against shadPS4 has now answered 31,754 calls and the picture is clear: `0x8002`
is **one facility of at least 33**, and the scheme is `0x8` + a 16-bit facility + a
facility-local code, with the facility tracking a subsystem *family* rather than a library -
`0x8055` across eleven `libSceNp*` libraries, `0x80b8` across seven dialog libraries.

So 901 records that are genuine, facility-coded argument rejections sit in `error-shaped`
alongside seven plain `-1`s, while 53 records from one favoured facility get `rejected`.

### The tempting fix is to widen the test, and it is wrong twice

**It would change a documented meaning.** `rejected` means "matches the vendor error scheme"
and the report is an interface (CLAUDE.md rule 3). Widening it silently makes every
historical report's `rejected` count mean something different from every new one, and the
check ids are the key when diffing runs.

**More importantly, it would encode emulator-measured knowledge into the probe.** The facility
table came from shadPS4. There are **0 hardware confirmations**, and a probe that classified
by that table would be asserting on hardware something it learned from an emulator - which is
exactly what `900-surface/presence-is-not-behaviour` exists to warn about, committed by the
instrument itself.

The section already states the principle it would be breaking: the comment beside
`error-shaped` says the bucket separates a negative answer from an ordinary value *"without
claiming to know which"*. A facility table is claiming to know which.

### Nothing is lost by leaving it

**The bucket is a summary; the value is in the record.** Every `error-shaped` line carries the
full 64-bit return, so facility decoding is available to any reader who wants it and costs
nothing to defer. A summary that groups two things a reader can separate is a weaker summary,
not a lost measurement.

### What this does mean

The existing `0x8002` constant is itself emulator-derived, from PS5PCEM (D088), so
the probe is already doing a small version of the thing rejected above. It stays because
removing it would change the same documented meaning in the other direction, for no gain -
but it is now known to be a historical artefact rather than a principled boundary, and if the
contract is ever versioned for another reason, classifying by *shape* rather than by a named
facility is the change to make.

Recorded because the next person to see 901 records in a vague bucket will want to widen the
test, and the reason not to is not obvious from the code.


