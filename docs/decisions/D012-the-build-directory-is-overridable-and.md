# D012 - The build directory is overridable, and has to be

**assumed** · 2026-08-19

`make BUILD=/tmp/obs`.

Development happens on Windows with the repository mounted into a Linux VM. That
mount cannot carry the execute bit, so a binary built into the tree is built and then
cannot be run. Building to a VM-local path is the fix, and it needs to be a documented
knob rather than folklore rediscovered every session.

