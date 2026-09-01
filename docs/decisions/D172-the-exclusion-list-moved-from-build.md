# D172 - The exclusion list moved from build time to run time, so one ELF runs on every loader


A check that ends the process takes every check behind it with it, so fpPS4 reported six
results and stopped. The answer was `-DOBSCENE_EXCLUDE`, a list of ids compiled in, and it
worked: 44 exclusions and fpPS4 reaches the end.

What it cost is the thing this program is for. **Each loader needed its own module**, so "the
same binary behaves differently on shadPS4 and fpPS4" was a build difference rather than a
measurement, and a reader comparing two reports was comparing two programs. The lists drifted
too - one loader's became another's more than once, and shadPS4 spent a day running with four
exclusions when it needs two.

### The report was already the state file

`obs_sink_write` puts every record on disk *before* the risky call that follows it - the same
durable-write-ahead-of-risky ordering as announce-before-attempting. So a report from a run
that died already names the check that was in flight: a `try` with no `res`.

Read it at startup, skip that check, run everything else. `obs_sink_backend_open` opens
`O_TRUNC`, so the read has to happen first, and `harness.c` calls `obs_resume_load` immediately
before `obs_sink_open` for that reason alone.

### One remembered id was not enough, and the comment claiming otherwise was wrong

The first version kept a single id and its comment said successive runs would "converge
anyway". They oscillate. Measured against shadPS4 with no build-time list:

```text
run 1  died at 040-file/open-rejects-null
run 2  skipped it, died at 080-video/flip-rate-rejects-bad-handle
run 3  forgot the first, died at 040-file again
```

Each run discovered one thing and lost the last one. **The set has to accumulate**, and it
accumulates *through the report*: a run records every check it skipped for this reason, so the
next run reads its predecessor's decisions back and adds the new discovery. No second file, no
new format.

After the fix, same binary, no exclusions compiled in:

```text
run 1  36,577 records  COMPLETE   carried 4
run 2   7,438 records  died at 900-surface/corpus_0138_libSceLibcInternal
run 3  33,597 records  COMPLETE   carried 5
```

Run 2 is worth its own note: shadPS4 crashed somewhere run 1 did not. **It is
non-deterministic**, which a fixed build-time list presented as a stable property.

### Three guards, because a stale skip is the hazard this introduces

A skip inherited from a report that does not describe this program would silently remove a
check nobody excluded. All three are tested:

- **the build id** - weak alone, since it defaults to the literal `dev`
- **the check count** from `meta` - a build that added or removed a check cannot match
- **an answered check** - a `try` with its `res` is not a candidate at all

The first attempt read the build id out of the `meta` record, whose third field is the format
version, so it never matched and every resume was silently discarded. The mechanism appeared
to do nothing.

### Two skip reasons, kept distinct

`excluded at build time` is an operator's judgement; `did not return on the previous run of
this build` is this program's own observation. Only the second is a finding, and sharing a
sentence would have made the compatibility table's count of build-time exclusions wrong.

### The set is bounded, and says so

`OBS_RESUME_MAX` is 96 against the worst loader measured (fpPS4, 44). A run that filled it
would quietly stop learning - the oscillation again, without a symptom - so `OBS|resume|<n>|
<ok|full>` is emitted every run whether or not anything was carried. "Nothing was skipped" and
"this build cannot skip" are different runs and silence cannot separate them.

