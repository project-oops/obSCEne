# 2026-08-19 - The diff loop, which is what the format was for


**Done.** Built the capability the report format has been paying for since D003.

- `tools/diff.py` - compares two reports, exits non-zero on regression.
- `make diff BASELINE=...` - change something, re-run, ask whether it helped.
- New `build` record, stamped at compile time (`make BUILD_ID=...`), so a diff can
  separate "the probe changed" from "the platform changed".
- `tools/test-tools.sh` - pins the exit codes of both tools, in `make check` and CI.
- CI stamps the commit into every report.

**Verified.** 8/8 tooling tests. Diffed a real pair of runs - the libm and no-libm
host builds - and it reported exactly the truth: 7 improved, 0 regressed, 70
unchanged, 20 symbols newly present, `tally: pass +7, skip -7`. Reversed, it reported
7 regressions and exited 1. Against itself, silence.

**Surprises.**

- **The format had been justified by a tool that did not exist.** Every awkward choice
  on the C side - stable identifiers, one record per line, raw numbers instead of prose
  - was argued for on the grounds that an agent would diff runs. Nothing could diff
  runs. The cost was being paid and the benefit was theoretical.

- **Defining "regression" was the whole design.** The obvious reading - a check that is
  failing - is useless here, because under an early emulator almost everything fails
  and would report as a permanent regression. It has to mean *got worse*, which makes a
  run where everything fails and nothing changed a clean exit. Correct, and it looks
  wrong until you think about when the tool is used.

- **`skip` ranks below `fail`, not above it.** A check that stopped running tells you
  less than one that ran and failed. Without that ordering, losing coverage reads as
  neutral - and deleting an inconvenient check would read as an improvement.

- **Testing the tools against a degraded copy of a real report, not a fixture.** A
  fixture drifts from the format while its tests keep passing, which is precisely the
  failure being guarded against.

- **PowerShell ate `$?` before bash saw it**, so two rounds of "exit code" output were
  reporting PowerShell truth values rather than the tools. The exit codes were only
  actually verified once a script inside the VM asserted them - which is the reason
  test-tools.sh exists rather than a series of ad-hoc invocations.

**Not done.** The census is still my recollection of what matters rather than a real
title import table. Struct layouts, NID verification and running under an emulator all
remain blocked.

