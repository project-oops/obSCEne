# D062 - The console generation is a build option, because the loaders disagree and all of them are right


Status: decided, correcting D061 in the same session.

D061 set `e_ident[EI_ABIVERSION]` to 2 and claimed the change was safe for shadPS4 because
that emulator "reads this field only to name it in a log line and does not gate on it".

**That was wrong.** shadPS4 rejects the module outright:

```
IsElfFile: e_ident[EI_ABIVERSION] expected 0x00 is (0x2)
```

The claim came from grepping for the field name, finding a function that formats it for a
log, and stopping there. The check is forty lines away in the same file. A grep that finds
*an* answer is not the same as finding *the* answer, and the sweep went from 666 records to
zero.

**The conflict is real and nobody is at fault.** shadPS4 is a previous-generation emulator
and refusing a module marked for the current generation is the entire purpose of the field.
Kyty accepts 0 or 2 and reads 2 as current; craziiEmu reads it the same way. A single
constant cannot satisfy both, and neither should be worked around.

So the generation is a build parameter: `make module GEN=5` by default, `GEN=4` to run
under a previous-generation emulator. `scripts/sweep.ps1` defaults to 4 because it drives
shadPS4.

**Not a disguise.** A `GEN=4` build tells that loader something true about what it was
built as. The alternative - leaving the byte at zero always - was telling every
current-generation loader something false, which is what D061 correctly identified and
half-fixed.

The probe is for the current generation. That is the default, and the other build exists
because the most complete emulator available is for the previous one.

