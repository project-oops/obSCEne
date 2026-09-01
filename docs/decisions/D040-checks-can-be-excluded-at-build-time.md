# D040 - Checks can be excluded at build time, and the default list is empty


Status: decided.

A conformance probe against a platform under development meets calls that do not merely
fail but end the run, and everything behind such a call is lost. Here it was two checks
out of seventy-nine, and they cost eight sections including the census - the section
whose answer is most useful when everything else has gone red. The first run reached 110
records of 513.

    make module EXCLUDE="040-file/open-rejects-null 080-video/flip-rate-rejects-bad-handle"

**This is not for making a report look better**, and the design says so in three places:

- The default list is empty, so the first run finds the crash. That is the finding.
- An excluded check is reported as a **skip with the reason**, not omitted. It stays
  visible in the report.
- `Skip` ranks below `Fail` in `diff`, so excluding a check to bury a failure reads as a
  regression - which is exactly what it would be.

Both runs are worth keeping and they answer different questions: the first says what
takes the process down, the second says what everything else does. Both are in
`docs/examples/`.

