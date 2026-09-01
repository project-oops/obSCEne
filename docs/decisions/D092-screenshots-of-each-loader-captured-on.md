# D092 - Screenshots of each loader, captured on the summary screen


Status: decided.

obSCEne draws its report, and that exists for the case where no text channel works. **Two
loaders are in exactly that case** - Kyty and fpPS4 both run the module and emit no
records - so a screenshot may be the only report those loaders will ever give.

`scripts/screenshot.sh` captures the emulator's window; `run-emulator.sh --shot` wires it
in. Three things had to be got right and each was wrong first:

**`PrintWindow`, not a screen grab.** A desktop grab captures whatever is on top, so
another window silently becomes the evidence - worse than no screenshot, because it looks
like one. `PW_RENDERFULLCONTENT` is required as well, or a GPU-composited window captures
as a blank rectangle of the right size, which is again a convincing wrong answer.

**Match the process by name, not by `$!`.** Under Git Bash that is a *bash* process id and
the Windows tooling that owns window handles knows nothing about it - so it looked correct
and found no window every time.

**And the summary screen needed obSCEne to change, not the capture.** The summary is the
one screen that answers "what happened" alone, and it was held for three seconds before the
detail pages began cycling. No capture timing could catch it reliably: polling for the run
to finish lands several pages late, because an emulator's output reaches a file long after
the guest wrote it - a fixed delay caught page 1, then page 4. The summary is now held for
thirty seconds, which fixes the capture and is better for a person watching.

This one file shells out to PowerShell, and that is not a retreat from D071: window capture
is a Win32 operation with no command-line equivalent, invoked as a platform tool the way
`multipass` is. Elsewhere it reports that it cannot capture and exits zero, because a
missing screenshot must never fail a run.

