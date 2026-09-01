# D020 - The tooling has tests, and they run against real reports

**assumed** · 2026-08-19

`tools/test-tools.sh` pins the exit codes of `verify.py` and `diff.py`, and runs in
`make check`.

Both tools signal through their exit code and both are consumed by CI. An exit code
nobody checks is one that can silently invert - and a diff tool that stopped reporting
regressions would be invisible precisely while doing the most damage, because a clean
run looks like good news.

The tests **degrade a real captured report** rather than using a fixture: take the
current output, turn one pass into a fail, assert the diff notices. A fixture drifts
from the format while the tests keep passing, which is the failure mode this is
guarding against in the first place.

