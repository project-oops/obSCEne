# 2026-08-26 - fpPS4: theirs, and one line


`Sleep(INFINITE)` in `print_stub`, the handler bound to every unresolved function import. Not a
crash and not a bug - a debugging aid, with `DebugBreak` and `readln` commented out beside it.
Unattended it costs the whole run silently, because the process stays alive and simply stops
speaking. Every "fpPS4 seems stuck" observation was this.

Nothing on our side could have avoided it: a call that never returns cannot be detected by its
caller, and the dangling `try` naming `strspn` is the mitigation working, not failing.

Patched to return zero - a *function* returning `QWORD`, not a procedure, because the
trampoline hands the guest whatever is left in `rax` and garbage there crashes a pointer-
returning function far from the cause. Same mistake that crashed Kyty at entry once.

36 records and a permanent hang → **36,631 records, 27/27 sections, 515/515 checks, 2 seconds**,
surviving **181** unimplemented-import calls. Stock would have needed ~362 runs to converge at
two per blocker; fpPS4 is now the fastest loader in the sweep. (D196)

