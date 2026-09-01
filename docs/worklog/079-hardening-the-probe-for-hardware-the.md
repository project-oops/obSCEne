# 2026-08-27 - hardening the probe for hardware: the extent blind spot was half-closed


Asked to harden the active probe before the hardware run. The two axes named - the `size` record
(binary-search a size argument to derive exact structure sizes) and the "wrote-0 vs untouched"
extent fix - were **already built** by concurrent work: the poison-fill dump, `obs_report_written`
with `changed`/`untouched` runs, `obs_report_size`, and `check_query_size_ladder` all present, and
both records documented in `OUTPUT.md`.

The residual: the dump was moved onto the poison basis and the **verdict counter beside it was
left on the old non-zero one.** Observed live on the host - the same call reporting `extent 256`
in its dump and `pass|0x11` (17) in its verdict. On hardware that mismatch fails the other way: a
call writing a real structure with a zeroed trailing field gets "the call succeeded and wrote
nothing", a false failure contradicting its own dump. Fixed to count changed-from-poison; the
call now reports `pass|0x100`, matching. `verify: ok`, 24 gates. (D204)

Surprise worth noting: the decision index had drifted (203 body entries, stale index) from the
concurrent `selfish` split and D199-D203 - regenerating brought it current before D204 landed.

