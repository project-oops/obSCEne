# D075 - The current generation's graphics interface is censused, and its first run is the best evidence yet that presence means little


Status: decided, closing BACKLOG §1.

That item said "no source for the names that satisfies D018 - the obvious lists come from
dumping decrypted libraries, and inventing plausible names would produce a census full of
confident-looking absences that mean nothing". True when written, and it stopped being true
when the emulator toolkit arrived.

**87 names across `libSceAgc` and `libSceAgcDriver`**, taken from a current-generation
emulator that records them as structured export attributes - identifier, name and library
in one place - and corroborated for 81 by an independent identifier database. Both public.

**Every name is checked against this project's own hash, and that rejected five.** Two were
placeholders with the identifier embedded in the name, `sceAgcDriverUnknown_KRzWekV120`
being a project marking a function it could not name. Censusing that would have added a
symbol that cannot exist, reported absent forever, indistinguishable from a real gap - the
exact failure D053 found once already. The other three pair a plausible name with an
identifier our hash does not reproduce, so one of the two is wrong and neither is worth
having.

Marked `OBS_CURRENT`, so absence on previous-generation hardware reads as correct rather
than as a gap.

### And then all 87 reported present on a previous-generation emulator

shadPS4 is a previous-generation emulator with no implementation of this interface at all.
It resolves every one of the 87 through its generic stub, so the census reports the whole
of the current generation's graphics interface as **present**.

This project has said several times that presence measures the loader's stubbing policy as
much as the platform (D061). This is the clearest demonstration available: an entire
interface, from a console generation the emulator does not target, reported present by a
census that is working exactly as designed.

The `availability` field is what saves it - those records carry `current`, and the run
detected a previous-generation platform - so the information needed to discount them is in
the report. A reader who ignores it will draw a badly wrong conclusion, and this is worth
knowing before anyone quotes a coverage figure.

**Still presence-only.** Nothing here is called. A graphics interface needs command buffers
and struct layouts to exercise, which is BACKLOG §2 and genuinely blocked.

