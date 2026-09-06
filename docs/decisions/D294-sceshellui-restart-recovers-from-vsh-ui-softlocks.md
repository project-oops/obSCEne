# D294 - SceShellUI restart recovers from VSH UI softlocks

**Status**: [decided]
**Date**: 2026-09-02
**Context**: Hardware testing on PS5 with fake-signed native titles and homebrew loaders.

---

## Context

When launching native homebrew or experiencing title crashes, the PS5 VSH (Shell UI) often enters a modal error scene (FsReadErrorScene or AppClosingScene) where the UI softlocks or hangs, blocking further title execution and modal transitions.

Previously, recovering from this state required a full hardware reboot (which takes significant time and loses the jailbreak/kstuff state).

Attempts to kill SceShellCore (PID 58/60) proved catastrophic: SceShellCore is critical kernel-linked infrastructure, and terminating it triggers ICC/Syscon watchdog panics and forces a hard console shutdown. Similarly, running manual umount -f on active sandbox mounts causes SceShellCore SceLncTerminateMountRootThread to abort with error 0x80020016.

## Decision

To recover from VSH UI softlocks cleanly and safely without rebooting:
1. Target **SceShellUI exclusively**. Terminating SceShellUI causes its parent daemon SceSysCore to cleanly respawn the UI process in ~2 seconds, resetting the VSH scene stack and dismissing any active modal dialogs.
2. obscene-tool provides hw restart-ui (and ./bin/obscene restart-ui / reset-ui), which inspects running processes via procstat -a over shsrv (port 2323), resolves SceShellUI current PID dynamically, and sends a termination signal (kill <pid>).
3. The command strictly validates that the target process is SceShellUI and explicitly guards against touching SceShellCore or any other system process.

## Consequences

* Fast UI softlock recovery in ~2 seconds without requiring a reboot or re-jailbreak.
* Preserves system stability and keeps background services (elfldr, kstuff, ftpsrv, klogsrv, shsrv, pldmgr) running.
