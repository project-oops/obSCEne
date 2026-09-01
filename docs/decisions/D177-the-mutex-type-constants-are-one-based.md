# D177 - The mutex type constants are one-based and are not the POSIX values


`015-sync/mutexattr-round-trip` sweeps candidate type values rather than naming one, which was
right - D008 forbids inventing the constant, and which values are accepted is itself a finding.
The **range** was picked without evidence: `0..3`, on the reasoning that POSIX names three types
so four is a little wider.

Kyty states the mapping outright:

```cpp
case 1: ptype = PTHREAD_MUTEX_ERRORCHECK; break;
case 2: ptype = PTHREAD_MUTEX_RECURSIVE;  break;
case 3:
case 4: ptype = PTHREAD_MUTEX_NORMAL;     break;
default: EXIT("invalid type: %d\n", type);
```

The accepted set is `{1, 2, 3, 4}`. So the old sweep spent a quarter of its range on a value
that implementation rejects outright, **while never trying one it accepts** - and `0` is exactly
what a POSIX-shaped guess reaches for, since POSIX `PTHREAD_MUTEX_NORMAL` is `0`.

Widened to `0..4`. `0` is kept deliberately: if it really is invalid, a platform refusing it is
a *result*, and it is the value most likely to be tried by mistake.

Provenance is `IMPLEMENTATIONS`, not `SPEC` - one emulator's reading, which is why the check
still sweeps and reports rather than encoding the mapping. Confirming it is a hardware question.

Status: **assumed**.

