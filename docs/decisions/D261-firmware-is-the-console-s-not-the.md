# D261 - Firmware is the console's, not the compatibility environment's


*status: measured*

The header showed `FW UNKNOWN`. The first fix read `sceKernelGetSystemSwVersion`, whose version
string this hardware fills with `13.090.001` - and that is a real value, just the wrong one. It
is the version of the **PS4-compatibility environment** the title runs inside, because the
title is a `ps4_game`. The console's actual system software is `12.40`, which the shsrv banner
and the kernel both report and that call does not.

The console's own firmware is in `kern.version`, which `135-sysctl` already captures raw:

```text
r226974/releases/12.40 Nov 27 2025 02:23:38
```

The header reads the token after `releases/`, up to the first space - `12.40` - and reports the
raw string nowhere but the `135-sysctl` records, where measured bytes belong. `kern.osrelease`
is not used: on this platform it is the placeholder `0.0-prototype`.

**Why this was worth getting right rather than convenient.** Both numbers are correct answers to
different questions, and labelling the compat version as the console's firmware is the same
class of error as reporting `GEN 4` for a PS5 in compatibility mode (D255): a value read
correctly and attributed to the wrong thing. A `ps4_game` sees the PS4 side of a hybrid - PS4
userland APIs, a PS4-compat version, the PS4 graphics driver - over a PS5 kernel it cannot
address directly. The firmware field now names the kernel's version, which is the console's.

Extracting a token out of the ident string depends on the kernel keeping that format, which is
more fragile than a struct field, so a string without the `releases/` marker reports unconfirmed
rather than guessing. The raw `sceKernelGetSystemSwVersion` bytes remain in the report under
`130-layout/system-software-version` for anyone who wants the compat version too.

