# D333 - the suspend lifecycle is a resolution census, and button-bits records edges

**assumed** - 2026-09-23

Two open bus requests are answered by the same session, both about a peripheral or a
lifecycle the probe can name but not fully exercise inside itself.

## The suspend surface (141-suspend), for REQ-...-5e8c

A real title - neverball, an SDL2 game on oops-gl - does not quiesce when the dashboard's
Close or rest-mode asks the system to suspend it. It dies with
`0xa0d0c00f CPU_FAULT_SUSPENDPOINT_TIMEOUT_IN_SUSPEND_ASYNC`, "No suspendPoint for
100sec": the CPU/suspend sibling of the GPU suspend-point fault this project already
recorded from a wedged pipe (`0xa0d0c00c`, 166-agc). The request asks what a title has to
*do* so the kernel can reach a suspend point in it - which is a question about a surface,
and this section answers it as one.

`141-suspend` resolves, and never calls, the four families a cooperating title would use:

- **libSceSystemServiceSuspend** - the explicit cooperation calls. The corpus names
  exactly three and marks all three callable; `sceSystemServiceDeclareReadyForSuspend`
  is the one a title invokes to say it has reached a quiescent point and may be frozen,
  the Enable/Disable pair governs whether a notification precedes it. This is the prime
  candidate for "what reaches the suspend point".
- **libSceSystemService** - the event pump (`sceSystemServiceReceiveEvent`,
  `GetStatus`): how a title learns a suspend is coming, and per the request may have to
  acknowledge.
- **libSceSysCore** - the `sceApplication` lifecycle (IsSuspendable / Suspend / Resume /
  SystemSuspend), the higher-level surface the same freeze runs through.
- **libSceAgc** - `sceAgcSuspendPoint`, the GPU command-stream suspend point, whose
  presence is the evidence for the request's arm 1: a pipe has an explicit point the
  stream must reach, and an in-flight submit that never reaches one is a candidate for
  holding the process out of suspend.

**Resolution only, for two reasons beyond the usual (D008).** The arities are
unconfirmed, and `sceSystemServiceDeclareReadyForSuspend` would - if it does what its
name says - declare *this probe* ready to be frozen mid-run. So nothing here is called,
and no name moves from the census into `platform.h`: every symbol is passed as a string
to `obs_module_open` / `obs_module_symbol`, the same way 106-encoder resolves its
compression surface. The section adds no census entries and needs none of the five steps
in `CLAUDE.md` that a *calling* check does.

**What it cannot answer, and says so.** The behavioural half of arm 1 - whether a specific
unretired fence or bound context is what times the suspend out - is not observable by an
inert probe, because a probe cannot watch its own suspension. The section emits a
`gpu-drain-required / needs-live-suspend` marker rather than inventing a verdict; settling
it needs a live close comparing a title that drains the GPU against one that does not.

Placed at 141, beside the oracle: it asks the platform about itself rather than testing an
answer, so it reads with the sections that do the same.

## button-bits records edges, not only the union (REQ-...-d1c4)

The `100-input/button-bits` probe (D328) reported the OR of every button word seen. The OR
cannot map a button to a bit - pressing all of them yields one merged mask - which is
exactly what left d1c4 open after an automated sweep: read-extent and stick-trigger-range
settled, button-bits could not, because it needs the buttons pressed *and* told apart.

It now also records a rising edge: the first sample a bit appears in emits an `edge` row
carrying that bit's index, in press order (the report preserves emission order). An
operator pressing in a documented order reads those rows off in the same order, so a
single manual run maps each face button to its bit and offset - the request's acceptance -
where before it took one run per button. The window grew from ~4s to ~8s to fit them, and
the `button-or` total is unchanged, so no historical row moves.

Both requests still need a run this side cannot make on its own: a controller with a human
pressing buttons for d1c4, and a live suspend on hardware for 5e8c's behavioural arm. The
code is what makes each run answer in one pass.
