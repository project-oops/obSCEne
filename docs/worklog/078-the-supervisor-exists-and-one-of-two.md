# The supervisor exists, and one of two open items closes


`docs/HARDWARE-PROBE.md` named the hard part of running on hardware before any hardware
existed:

> **A faulting command kills the payload.** The first bad pointer ends the session, and if the
> exploit is not persistent it ends the *afternoon*. This is the real engineering cost.

It offered two ways out - a supervisor that survives a faulting command, or an executor that
validates arguments before dereferencing. **The first is built**, in prosperous:
`pros supervise <module.elf>` watches the serving port and re-sends through the loader when
nothing answers.

It lives there rather than here because that project is already the one that reaches a
console, and because a supervisor and the sender being one program is what makes the restart
unattended. The doc has been updated where the item was raised.

The second option stays open, and it is the better one: it would stop the crash rather than
recover from it.

**The reboot item did not close and must not look like it did.** Re-sending needs a loader
already listening; a power cycle takes the loader with it. That still needs a person, and a
supervisor that appeared to survive reboots would be trusted through one.

