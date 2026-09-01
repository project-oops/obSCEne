# D039 - Imported symbols are rewritten to `STT_FUNC`, and this is what makes a report come out


Status: decided, on evidence - after being wrongly reverted once.

The linker leaves an undefined reference as `STT_NOTYPE`. A console loader matches
imports on the NID **and the symbol type**, against a table where every platform
function is registered as a function, so an import typed `NOTYPE` matches nothing.

It does not fail. It binds to a stub that returns zero. The module loads, runs every
check, and gets a plausible zero back from all of them - including from the write it
reports through. Every run before this produced no output for that reason, and the
"no emulator implements a write path" conclusion in the worklog was wrong: the write
was there and we were not asking for it in a way the loader could match.

**How this was nearly missed.** Typing the imports made the emulator log *fewer*
resolutions and end in a fault, and that was read as a regression and reverted. It was
the opposite. It got much further: the report appeared on the console's own output
channel, ran through fifteen sections, and stopped in a call that genuinely takes the
process down. Fewer resolution lines because it stopped resolving and started running.

The lesson is that "fewer log lines and a crash" and "worse" are not the same thing,
and a probe that announces before it attempts can tell them apart - which is what it
did, once anyone read the output instead of the counters.

Every symbol this program imports is a function. The census declares its names as
`const char` so the type system forbids calling them, but that is a fact about this
source, not about the platform. A genuine data import would need a kind column in the
manifest; there is not one yet.


