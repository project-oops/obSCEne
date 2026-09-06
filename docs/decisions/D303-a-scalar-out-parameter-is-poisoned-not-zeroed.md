# D303 - a scalar out-parameter is poisoned, not zeroed

**assumed** - 2026-09-03

`106-encoder/path-probe` calls `sceKernelLoadStartModule` down twenty-four search paths and
reports both the return and the out-parameter beside it:

```c
int res = 0;
int h = sceKernelLoadStartModule(search_paths[i], 0, (void *)0, 0, (void *)0, &res);
```

All twenty-four reported `res` as `0x0`. **That reading separates nothing.** A platform that
never writes the field and a platform that writes zero produce the same number, so the value
carries no information about the platform at all - it is the probe reading back its own
initialiser. orbistoun, consuming the report, had to mark all twenty-four opaque with exactly
that reason (its decision 497), which is where this was noticed.

Initialised to `0xC7C7C7C7` instead. Untouched is now a visible answer rather than an
indistinguishable one, and a real zero is a real measurement.

## Why `0xC7` and not a marker invented for this

It is already this project's pattern byte - `obs_layout_patterns` uses it, so a reader who
recognises it anywhere recognises it here. A word of it is a value no error code and no handle
would plausibly be, which is the whole requirement.

The residual is that a platform writing exactly `0xC7C7C7C7` reads as untouched. No single
pattern avoids that; naming it is the honest response, and it is the same residual
`obs_report_written` already carries.

## The general form, which is the part worth keeping

**`obs_report_written` makes this argument for buffers and the probe never made it for
scalars.** A check that reports an out-parameter it initialised is reporting its own value
until the platform overwrites it, and the only defence is to initialise it to something the
platform would not choose. Recorded in `docs/backlog/022` so the rest of the suite gets swept
for the same shape rather than fixed one site at a time.

Principle 2 is not in tension here: nothing is invented about the platform. The poison is a
fact about the probe, and the report says what it saw.
