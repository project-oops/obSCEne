# D139 - A platform HUD replaces the static tagline: every system fact, value or `unknown`


Status: derived - rendered on the host build, tiering confirmed.

The screen's "CONFORMANCE PROBE" line is now a live status line of eight fields - NET, IP,
FW, GEN, MEM, VRAM, TEMP, DISK - each showing its value or `unknown`. Built after a good
argument (the operator's, not mine): surfacing "this platform does not implement X" is the
whole point of obSCEne, so a field reading `unknown` because a query is absent is a finding,
not a defect - the census philosophy on a status line. Fabricating a number would be the
sin; `unknown` is the honest opposite and a standing forcing-function for emudev.

The one constraint that survives "just fail open": a value appears only through a **confirmed
signature** (D008). Fail-open covers the result and the absence, not calling blind - a
guessed arity on a struct-filling query crashes the HUD rather than failing open. So each
field is one of three honest states, colour-coded: known (ink), present-but-unwired (accent),
absent (dim).

Wired now with confirmed reads: GEN (which generation's library resolves), MEM
(`sceKernelAvailableFlexibleMemorySize`), VRAM (`sceKernelGetDirectMemorySize` - the shared
pool, the honest figure for a unified-memory console), NET (the listening port, on a HUD-only
screen the serving build draws so the address is readable). Present-but-unwired: IP
(`sceNetCtlGetInfo`) and FW (`sceKernelGetSystemSwVersion`) - both resolve but their struct
layouts are not confirmed. Placeholders: TEMP and DISK - no confirmed query yet, on screen
deliberately as the reminder.

Screen only, never the graded corpus. A probe cannot certify its own machine (D108), so these
are a live readout for the human in the room; they may become `measure` observations but
never self-certified machine provenance.

The serving build draws a HUD-only screen (no suite), which also fits listen-first (D132):
the socket is up in seconds and the port is on screen, with none of the crash-prone suite run
first.

