# D006 - Output through the platform's own write, to descriptor 1

**assumed** · 2026-08-19

Rather than an emulator-specific reporting hook.

This program is ordinary homebrew and has to stay that way. A probe that reports
through a channel only one emulator implements is measuring that emulator's opinion
of itself. `write` to descriptor 1 is POSIX, the target kernel is FreeBSD-derived, and
it works on real hardware - so an emulator needs exactly one function implemented to
see the entire report.

