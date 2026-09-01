# D045 - Checks are chosen from what emulators have actually got wrong


Status: decided, and validated on the first try.

`015-sync` exists because an emulator's release notes said it had just rewritten its
event flags and fixed a thread pool that destroyed threads twice under churn. Both
functions were already in this program's census - reported as existing, never called -
so the suite said nothing about either while both were broken. An existence test cannot
see a behaviour bug, and behaviour bugs are what emulators have.

Written against that release, the checks found:

- **`event-flag-round-trip` fails.** A bit that is set, then cleared, still polls as set.
  In the release that rewrote event flags.
- **`thread-churn` crashes the emulator, about one run in four**, inside its own
  `ExitThread` assertion. Forty create-and-join cycles; two would never have seen it.

Neither needed a user report, a bug tracker, or anyone's source.

**Poll, never wait.** `sceKernelWaitEventFlag` blocks, and a probe that blocks against a
platform whose event flags are broken never returns - taking every check behind it with
it. `sceKernelPollEventFlag` asks the same question and comes back either way. Any check
aimed at something suspected of being broken has to be written this way.

