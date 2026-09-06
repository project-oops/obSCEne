# 14. Measurements orbistoun is blocked on

Written from the consuming side, and **kept complete on purpose**: everything orbistoun cannot
decide from the captures it has belongs here, so one hardware sweep can close as much of it as
possible rather than a console day answering four questions.

## How to regenerate this

The bulk of the list is not hand-curated. orbistoun tracks every unverified claim in its
knowledge base and ranks them by how often a guest actually calls the function:

```bash
orbistoun-cli questions
```

**744 open questions across 502 functions** as of 2026-09-04, re-derived rather than
carried forward - the count moves every time an assumption is written or retired, and 719/505
stood here from an earlier tick of the same day.

**Read it with `--premises`.** The raw list repeats itself badly: the 744 questions are only
**143 distinct sentences**, and 28 of those carry 629 of the total (D538). One sweep answering a
shared premise retires every question quoting it, so the grouped view is the one that ranks work
by what a console day actually buys:

```bash
orbistoun-cli questions --premises
``` Each question is a specific
caveat - *"modelled on the POSIX call of the same shape"*, *"which errno values the target
returns is unverified"*, *"nothing about this entry has been established"* - and a probe that
calls the function and reports what it did retires it.

**One ask has been withdrawn, 2026-09-04 (D541).** `scePthreadMutexUnlock` was in this list
asking whether unlocking an unheld mutex returns `0x80020001`. **This repository already
answered that** - `015-sync/mutex-unlock-unheld` returned it, and D398 confirmed the
`0x8002_0000 | errno` shape across seven failures in five families. The entry had the result
written into its own open-question list, in capitals, so the queue kept asking. Do not spend
console time on it.

That was the only instance: a sweep of the knowledge base found no other assumption announcing
a measurement, and a guard now refuses one.

**A correction, 2026-09-04 (D540).** Nine entries in this list used to say their semantics
*"follow the POSIX analogue of the same name"* - the event-flag and semaphore families,
`sceKernelCreateEventFlag`, `sceKernelWaitSema` and their siblings. **POSIX has no function of
any of those names.** If a sweep was planning to check them against a standard, there is nothing
to check them against; they are now recorded as inferred from the name, the argument shape and
the rest of the family, and the specific asks under them are new:

- **`sceKernelWaitSema(semaphore, need, timeout)`** - orbistoun ignores `need` and the timeout
  entirely (it takes one unit and blocks forever). What a `need` above one asks for, and what
  the third argument does - its unit, and whether zero means poll - is unestablished. Same for
  `sceKernelPollSema`'s count.
- **`sceKernelPollEventFlag`** - only bit 0 of the mode word is modelled (every bit of the
  pattern must be present). What the remaining bits select is unestablished.
- **`sceKernelCreateEventFlag`** - the attribute word and the fifth argument are not modelled.
- **A bad handle answers ESRCH (`0x80020003`) in the event-flag family** - which this repository
  measured in `015-sync/event-flag-rejects-bad-handle` - **and orbistoun's placeholder
  (`0x7fff_0003`) in the semaphore family.** Whether the two families agree on the target is
  unestablished, because the measurement covered only event flags. That is a cheap addition to
  any sync sweep.

**745 of those questions are 140 sentences** (2026-09-04; it was 713 across 135 before the nine
false claims were replaced by true ones - **the count went up because a wrong sentence was
hiding real unknowns**).

**713 of those questions were 135 sentences** (2026-09-04, after D539 - it was 719 across 275
before the duplicate wordings were merged).

The single largest one is now **149 functions in `libScePosix`** asking one thing: whether a
POSIX spelling and the vendor-named function it resolves to are the same behaviour on the target,
or merely similar. It used to be 149 separate questions because each named its own target inside
the sentence.

And the ask that replaced the heaviest misfiling is worth a probe on its own, because it is one
call:

> **Which convention do the platform's POSIX-named exports use on failure?** POSIX's own - a
> returned errno, or -1 with errno set - or the vendor encoding `0x8002_0000 | errno` that D398
> measured on their *vendor-named* twins. D398 provoked seven failures across five families of
> vendor-named calls and every one came back `0x8002_0000 | errno`. **No POSIX-named export has
> ever been measured.** One provoked failure - `pthread_mutex_lock` on a destroyed mutex,
> `close` on a bad descriptor - settles it for that call, and a second in another family settles
> whether it is a convention at all. 24 entries and ~101k calls rest on it.

**719 of those questions were 275 sentences.** `orbistoun-cli questions --premises` groups the
list by the premise its entries share - word-for-word, never by similarity - and forty premises
carry 484 of the 719. The heaviest is one sentence under fourteen functions across two
libraries:

```text
872904 calls   14 functions across 2 libraries
    ? Modelled on the POSIX call of the same shape. The correspondence is inferred from
      the name and the guest's usage, never verified against the target library.
      sceKernelClose, sceKernelOpen, sceKernelRead, sceKernelWrite, scePthreadCreate,
      scePthreadJoin, scePthreadMutexInit, scePthreadMutexLock, scePthreadMutexUnlock,
      scePthreadMutexattrDestroy, scePthreadMutexattrInit, scePthreadMutexattrSetprotocol,
      scePthreadMutexattrSettype, scePthreadSelf
```

That changes what a sweep is being asked for. A premise shared by fourteen functions is
answerable by **sampling** it - two or three of the fourteen, chosen for how cheap they are to
call - and the result speaks to the shared claim rather than to one entry. Enumerating fourteen
identical asks was never the shape of the work.

**A sample is not a proof for the rest**, and the grouping does not pretend otherwise: measuring
`sceKernelWrite` settles `sceKernelWrite`. What it settles for the other thirteen is the
premise's *credibility*, which is what "never verified against the target library" is asking
about. A single counter-example refutes the group outright, which is the cheaper outcome to
look for.

**The wording of the heaviest entries changed on 2026-09-04 and the tables below have not been
regenerated since.** Forty entries used to say only *"nothing about this entry has been
established"*, and 431,213 calls sat behind them - not because the functions were unknown, but
because orbistoun's reasoning lived in its code and never reached its records. Five records now
carry it, and 86 calls remain behind an empty one.

For a sweep that matters here: the two heaviest of those five are `sceVideoOutGetFlipStatus` and
`sceVideoOutSubmitFlip`, and their questions are now specific - *is the completed count the head
of `SceVideoOutFlipStatus` at offset 0?* That is the same structure the equeue ask below needs,
so one flip-path capture answers three entries rather than one.

The three tiers below are that list joined against what this repository can call today.
Regenerate the join by comparing `questions` output against `include/obscene/platform.h`
(callable) and the `OBS_SURFACE_*` lists in `include/obscene/surface.h` (censused). A name
cannot be in both, which is the check that says the join is right.

## Tier 1 - a check can be written today (137 functions, 1,256,209 recorded calls)

These already have a signature in `platform.h`, so nothing blocks a check but writing it.
**This is the tier a sweep should clear first** - highest answer-per-unit-effort in the
project, and it covers the functions guests call most.

| calls | library | function | shape |
|---|---|---|---|
| 806,669 | libkernel_fs | `sceKernelWrite` | returns count, 3 args |
| 200,168 | libSceVideoOut | `sceVideoOutGetFlipStatus` | returns status, 2 args |
| 141,731 | libkernel | `sceKernelDlsym` | shape unrecorded |
| 52,406 | libkernel_fs | `sceKernelRead` | returns count, 3 args |
| 26,025 | libkernel | `sceKernelUsleep` | returns status, 1 args |
| 8,236 | libc | `rand` | returns count, 0 args |
| 6,630 | libc | `strtok` | returns pointer, 2 args |
| 4,602 | libkernel | `scePthreadMutexUnlock` | returns status, 1 args |
| 2,203 | libkernel | `sceKernelVirtualQuery` | returns status, 4 args |
| 1,391 | libkernel | `sceKernelLoadStartModule` | returns status, 6 args |
| 1,275 | libkernel | `scePthreadMutexInit` | returns status, 3 args |
| 654 | libkernel | `sceKernelCreateEventFlag` | returns status, 5 args |
| 295 | libkernel | `scePthreadSelf` | returns pointer, 0 args |
| 279 | libkernel | `scePthreadCreate` | returns status, 5 args |
| 247 | libkernel | `scePthreadJoin` | returns status, 2 args |
| 218 | libkernel | `sceKernelDirectMemoryQuery` | returns status, 4 args |
| 188 | libkernel | `sceKernelAvailableFlexibleMemorySize` | returns status, 1 args |
| 146 | libc | `sysctlbyname` | shape unrecorded |
| 122 | libkernel | `scePthreadMutexattrSettype` | shape unrecorded |
| 109 | libkernel_fs | `sceKernelOpen` | returns handle, 3 args |
| 107 | libkernel | `sceKernelGetProcessTime` | returns count, 0 args |
| 107 | libkernel | `scePthreadMutexDestroy` | returns status, 1 args |
| 104 | libkernel | `scePthreadMutexattrInit` | shape unrecorded |
| 103 | libkernel | `scePthreadMutexattrDestroy` | shape unrecorded |
| 91 | libkernel | `sceKernelDeleteEventFlag` | returns status, 1 args |
| 85 | libkernel | `sceKernelCreateSema` | shape unrecorded |
| 85 | libSceVideoOut | `sceVideoOutSubmitFlip` | returns status, 4 args |
| 84 | libkernel | `sceKernelAllocateMainDirectMemory` | returns status, 4 args |
| 58 | libkernel | `sceKernelGetModuleInfo` | returns status, 2 args |
| 54 | libkernel | `sceKernelAllocateDirectMemory` | returns status, 6 args |
| 50 | libkernel | `scePthreadCondInit` | returns status, 3 args |
| 48 | libkernel | `sceKernelPollSema` | returns status, 2 args |
| 46 | libkernel | `sceKernelPollEventFlag` | returns status, 5 args |
| 45 | libkernel_fs | `sceKernelClose` | returns status, 1 args |
| 40 | libkernel | `scePthreadCondDestroy` | returns status, 1 args |
| 35 | libkernel | `posix_sigismember` | returns status, 2 args |
| 35 | libSceSystemService | `sceUserServiceGetInitialUser` | returns status, 1 args |
| 34 | libc | `pow` | returns count, 2 args |
| 34 | libc | `strtod` | returns count, 2 args |
| 31 | libkernel | `sceKernelReleaseDirectMemory` | returns status, 2 args |
| 30 | libc | `llabs` | returns count, 1 args |
| 30 | libkernel | `sceKernelDeleteSema` | returns status, 1 args |
| 29 | libc | `acos` | returns count, 1 args |
| 29 | libkernel | `scePthreadMutexattrGettype` | returns status, 2 args |
| 28 | libc | `asin` | returns count, 1 args |
| 28 | libkernel_fs | `sceKernelLseek` | returns count, 3 args |
| 28 | libc | `tan` | returns count, 1 args |
| 27 | libc | `atan` | returns count, 1 args |
| 27 | libc | `atan2` | returns count, 2 args |
| 27 | libc | `log2` | returns count, 1 args |
| 25 | libkernel | `posix_pthread_rwlock_unlock` | returns status, 1 args |
| 25 | libkernel | `scePthreadRwlockUnlock` | returns status, 1 args |
| 24 | libc | `cos` | returns count, 1 args |
| 24 | libkernel | `sceKernelSignalSema` | returns status, 2 args |
| 22 | libc | `log10` | returns count, 1 args |
| 22 | libc | `powf` | returns count, 2 args |
| 22 | libkernel | `sceKernelMapDirectMemory` | returns status, 6 args |
| 22 | libkernel | `scePthreadAttrInit` | returns status, 1 args |
| 21 | libkernel | `scePthreadAttrDestroy` | returns status, 1 args |
| 20 | libc | `abs` | returns count, 1 args |
| 20 | libc | `isalpha` | returns count, 1 args |
| 20 | libc | `isdigit` | returns count, 1 args |
| 20 | libc | `labs` | returns count, 1 args |
| 20 | libkernel | `posix_pthread_rwlock_tryrdlock` | returns status, 1 args |
| 20 | libkernel | `scePthreadRwlockTryrdlock` | returns status, 1 args |
| 20 | libc | `tolower` | returns count, 1 args |
| 20 | libc | `toupper` | returns count, 1 args |
| 20 | libc | `wcslen` | returns count, 1 args |
| 19 | libc | `cosf` | returns count, 1 args |
| 19 | libc | `exp` | returns count, 1 args |
| 19 | libkernel | `scePthreadAttrSetdetachstate` | returns status, 2 args |
| 18 | libc | `sin` | returns count, 1 args |
| 18 | libc | `sprintf` | returns count, 2 args |
| 17 | libc | `expf` | returns count, 1 args |
| 17 | libc | `log` | returns count, 1 args |
| 17 | libc | `logf` | returns count, 1 args |
| 17 | libc | `sinf` | returns count, 1 args |
| 17 | libc | `strtof` | returns count, 2 args |
| 17 | libc | `tanf` | returns count, 1 args |
| 16 | libSceVideoOut | `sceVideoOutOpen` | returns handle, 4 args |
| 15 | libc | `isalnum` | returns count, 1 args |
| 15 | libc | `islower` | returns count, 1 args |
| 15 | libc | `isprint` | returns count, 1 args |
| 15 | libc | `ispunct` | returns count, 1 args |
| 15 | libkernel | `posix_pthread_rwlock_trywrlock` | returns status, 1 args |
| 15 | libkernel | `sceKernelSetEventFlag` | returns status, 2 args |
| 15 | libkernel | `scePthreadCondSignal` | returns status, 1 args |
| 15 | libkernel | `scePthreadRwlockTrywrlock` | returns status, 1 args |
| 13 | libSceVideoOut | `sceVideoOutClose` | returns status, 1 args |
| 13 | libc | `srand` | returns status, 1 args |
| 12 | libc | `puts` | shape unrecorded |
| 12 | libkernel | `sceKernelMunmap` | returns status, 2 args |
| 12 | libkernel | `scePthreadRwlockInit` | returns status, 3 args |
| 11 | libc | `bsearch` | returns pointer, 5 args |
| 10 | libc | `isspace` | returns count, 1 args |
| 10 | libc | `isupper` | returns count, 1 args |
| 10 | libkernel | `posix_pthread_rwlock_destroy` | returns status, 1 args |
| 10 | libkernel | `posix_pthread_rwlock_init` | returns status, 2 args |
| 10 | libkernel | `posix_sigemptyset` | returns status, 1 args |
| 10 | libkernel | `sceKernelClearEventFlag` | returns status, 2 args |
| 10 | libkernel | `sceKernelIsStack` | returns status, 1 args |
| 10 | libScePad | `scePadClose` | returns status, 1 args |
| 10 | libScePad | `scePadInit` | returns status, 0 args |
| 10 | libkernel | `scePthreadAttrGetdetachstate` | returns status, 2 args |
| 10 | libkernel | `scePthreadBarrierWait` | returns status, 1 args |
| 10 | libkernel | `scePthreadRwlockDestroy` | returns status, 1 args |
| 9 | libSceSystemService | `sceUserServiceInitialize` | returns status, 1 args |
| 8 | libSceSysmodule | `sceSysmoduleLoadModule` | returns status, 1 args |
| 7 | libkernel | `sceKernelMapFlexibleMemory` | returns status, 4 args |
| 7 | libScePad | `scePadOpen` | returns handle, 4 args |
| 6 | libc | `qsort` | returns status, 4 args |
| 6 | libSceAudioOut | `sceAudioOutInit` | returns status, 0 args |
| 6 | libkernel | `scePthreadCondWait` | returns status, 2 args |
| 6 | libSceVideoOut | `sceVideoOutSetFlipRate` | returns status, 2 args |
| 5 | libkernel | `posix_getpagesize` | returns status, 0 args |
| 5 | libkernel | `posix_sigaddset` | returns status, 2 args |
| 5 | libkernel | `posix_sigdelset` | returns status, 2 args |
| 5 | libkernel | `posix_sigfillset` | returns status, 1 args |
| 5 | libkernel | `posix_usleep` | returns status, 1 args |
| 5 | libkernel | `sceKernelGetModuleList` | returns status, 3 args |
| 5 | libkernel | `sceKernelIsCex` | returns count, 0 args |
| 5 | libkernel | `sceKernelReleaseFlexibleMemory` | returns status, 2 args |
| 5 | libkernel | `scePthreadBarrierDestroy` | returns status, 1 args |
| 5 | libkernel | `scePthreadBarrierInit` | returns status, 4 args |
| 5 | libkernel | `scePthreadCondBroadcast` | returns status, 1 args |
| 5 | libSceVideoOut | `sceVideoOutRegisterBuffers2` | returns status, 6 args |
| 4 | libSceGnmDriver | `sceGnmDispatchDirect` | returns count, 6 args |
| 4 | libSceGnmDriver | `sceGnmDispatchInitDefaultHardwareState` | returns count, 2 args |
| 4 | libkernel | `sceKernelGetSystemSwVersion` | returns status, 1 args |
| 4 | libkernel | `sceKernelIsDevkit` | returns count, 0 args |
| 4 | libkernel | `sceKernelIsNeoMode` | returns count, 0 args |
| 4 | libSceSysmodule | `sceSysmoduleIsLoaded` | returns status, 1 args |
| 3 | libSceVideoOut | `sceVideoOutRegisterBuffers` | returns status, 6 args |
| 2 | libSceVideoOut | `sceVideoOutGetResolutionStatus` | returns status, 2 args |
| 1 | libkernel | `sceKernelConfiguredFlexibleMemorySize` | 1 args |
| 0 | libSceSystemService | `sceUserServiceTerminate` | returns status, 0 args |
| 0 | libScePosix | `write` | returns count, 3 args |

## Tier 2 - censused, but no signature (63 functions, 27,511 recorded calls)

The census proves these resolve; nothing establishes an arity. **A lawful reference unblocks
these, not a console run** - the FreeBSD or POSIX analogue confirmed against the vendor form.
Adding one to `platform.h` with a guessed arity corrupts the stack and crashes somewhere
unrelated, which is D008 and principle 2.

| calls | library | function | shape orbistoun assumes |
|---|---|---|---|
| 22,561 | libkernel | `scePthreadGetthreadid` | returns count, 0 args |
| 4,523 | libkernel | `scePthreadMutexLock` | returns status, 1 args |
| 115 | libc | `vsnprintf` | shape unrecorded |
| 77 | libkernel | `sceKernelMapNamedDirectMemory` | returns status, 6 args |
| 54 | libkernel | `scePthreadMutexattrSetprotocol` | shape unrecorded |
| 46 | libc | `printf` | shape unrecorded |
| 18 | libkernel_fs | `sceKernelMkdir` | returns status, 2 args |
| 16 | libkernel | `scePthreadAttrSetstacksize` | returns status, 2 args |
| 15 | libkernel | `scePthreadAttrSetinheritsched` | 2 args |
| 15 | libkernel | `scePthreadAttrSetschedpolicy` | 2 args |
| 14 | libkernel | `scePthreadAttrSetaffinity` | 2 args |
| 14 | libkernel | `scePthreadAttrSetschedparam` | returns status, 2 args |
| 12 | libkernel | `sceKernelWaitEventFlag` | returns status, 5 args |
| 11 | libSceAgc | `sceAgcCreateShader` | 4 args |
| 8 | libkernel | `sceKernelReserveVirtualRange` | shape unrecorded |
| 3 | libc | `fgets` | 3 args |
| 3 | libkernel | `sceKernelMprotect` | returns status, 3 args |
| 2 | libSceSystemService | `sceSystemServiceParamGetInt` | returns status, 2 args |
| 1 | libc | `localtime` | 1 args |
| 1 | libkernel | `sceKernelSetVirtualRangeName` | 3 args |
| 1 | libkernel | `sceKernelWaitSema` | returns status, 3 args |
| 1 | libkernel | `scePthreadCondattrInit` | 1 args |
| 0 | libc | `__cxa_pure_virtual` | 0 args |
| 0 | libc | `fprintf` | shape unrecorded |
| 0 | libc | `gmtime` | 1 args |
| 0 | libScePosix | `posix_clock_gettime` | 2 args |
| 0 | libScePosix | `posix_ftruncate` | 2 args |
| 0 | libScePosix | `posix_gettimeofday` | 2 args |
| 0 | libScePosix | `posix_kevent` | 6 args |
| 0 | libScePosix | `posix_kqueue` | 0 args |
| 0 | libScePosix | `posix_mmap` | 6 args |
| 0 | libScePosix | `posix_mprotect` | 3 args |
| 0 | libScePosix | `posix_munmap` | 2 args |
| 0 | libScePosix | `posix_nanosleep` | 2 args |
| 0 | libScePosix | `posix_pthread_key_delete` | 1 args |
| 0 | libkernel | `posix_pthread_rwlock_rdlock` | returns status, 1 args |
| 0 | libScePosix | `posix_pthread_rwlock_timedrdlock` | 2 args |
| 0 | libScePosix | `posix_pthread_rwlock_timedwrlock` | 2 args |
| 0 | libkernel | `posix_pthread_rwlock_wrlock` | returns status, 1 args |
| 0 | libScePosix | `posix_pthread_rwlockattr_destroy` | 1 args |
| 0 | libScePosix | `posix_pthread_rwlockattr_getpshared` | 2 args |
| 0 | libScePosix | `posix_pthread_rwlockattr_gettype_np` | 2 args |
| 0 | libScePosix | `posix_pthread_rwlockattr_init` | 1 args |
| 0 | libScePosix | `posix_pthread_rwlockattr_setpshared` | 2 args |
| 0 | libScePosix | `posix_pthread_rwlockattr_settype_np` | 2 args |
| 0 | libScePosix | `posix_pwrite` | 4 args |
| 0 | libScePosix | `posix_signal` | 2 args |
| 0 | libScePosix | `posix_sleep` | 1 args |
| 0 | libSceAgc | `sceAgcAcbDispatchIndirect` | shape unrecorded |
| 0 | libSceAgc | `sceAgcDcbDrawIndex` | shape unrecorded |
| 0 | libSceAgc | `sceAgcDcbSetIndexBuffer` | shape unrecorded |
| 0 | libSceAgc | `sceAgcDriverSubmitAcb` | shape unrecorded |
| 0 | libSceAgc | `sceAgcDriverSubmitDcb` | shape unrecorded |
| 0 | libkernel | `sceKernelMmap` | returns pointer, 6 args |
| 0 | libScePad | `scePadOpenExt` | returns handle, 4 args |
| 0 | libScePad | `scePadSetLightBar` | returns status, 2 args |
| 0 | libScePad | `scePadSetVibration` | returns status, 2 args |
| 0 | libkernel | `scePthreadAttrGetstacksize` | returns status, 2 args |
| 0 | libkernel | `scePthreadRwlockRdlock` | returns status, 1 args |
| 0 | libkernel | `scePthreadRwlockWrlock` | returns status, 1 args |
| 0 | libSceSysmodule | `sceSysmoduleUnloadModule` | returns status, 1 args |
| 0 | libSceSystemService | `sceUserServiceGetUserName` | returns status, 3 args |
| 0 | libc | `wcscmp` | 2 args |

## Tier 3 - not known to this repository at all (303 functions, 221,203 recorded calls)

Guests call these and obSCEne has never heard of them. **The cheap first move is censusing
them** - a name in `data/surface.txt` costs nothing and a wrong one is a harmless false
negative (D014), where a wrong arity is not. Grouped by the library orbistoun sees them under.


**libkernel_fs** (1)

> `sceKernelDebugOutText`

**libc** (94)

> `_Getpctype`, `atan2f`, `sincosf`, `snprintf_s`, `memalign`, `_Mtx_init`, `_Cnd_init`, `_ZSt14_Random_devicev`, `wcsrchr`, `_ZSt13_Execute_onceRSt9once_flagPFiPvS1_PS1_ES1_`, `asctime`, `0x70031e6feb6b6b36`, `0x92f57c2dc704346f`, `_Assert`, `_Atomic_compare_exchange_weak_4`, `_Atomic_fetch_add_4`, `_Atomic_fetch_sub_4`, `_Atomic_load_4`, `_Cnd_broadcast`, `_Cnd_destroy`, `_Cnd_signal`, `_Cnd_timedwait`, `_Cnd_wait`, `_Getptolower`, `_Getptoupper`, `_Lockfilelock`, `_Locksyslock`, `_Mtx_destroy`, `_Mtx_lock`, `_Mtx_trylock`, `_Mtx_unlock`, `_Stoul`, `_Stoull`, `_Thrd_sleep`, `_Unlockfilelock`, `_Unlocksyslock`, `_Unwind_Resume`, `_Xtime_get_ticks`, `_ZSt11_Xbad_allocv`, `_ZSt14_Throw_C_errori`, `_ZSt14_Xlength_errorPKc`, `_ZSt14_Xout_of_rangePKc`, `_ZSt15get_new_handlerv`, `_ZSt16_Throw_Cpp_errori`, `_ZSt18_Xinvalid_argumentPKc`, `_ZSt19_Xbad_function_callv`, `_ZSt9terminatev`, `_ZdlPvSt11align_val_t`, `_ZnwmRKSt9nothrow_t`, `_ZnwmSt11align_val_t`, `__error`, `acosf`, `asinf`, `atanf`, `cbrtf`, `chmod`, `exp2`, `exp2f`, `frexp`, `getcwd`, `getifaddrs`, `getopt`, `getpid`, `hypotf`, `iscntrl`, `isgraph`, `isxdigit`, `ldexp`, `ldexpf`, `log10f`, `log2f`, `memcpy_s`, `memmove_s`, `memset_s`, `modf`, `modff`, `nanosleep`, `nearbyintf`, `setenv`, `signal`, `sincos`, `stat`, `strcat_s`, `strerror`, `strlcpy`, `strncat_s`, `strncpy_s`, `strnstr`, `strtoimax`, `strtoumax`, `sysctl`, `tanhf`, `wcsncpy`, `wcsncpy_s`

**libkernel** (8)

> `scePthreadAttrSetguardsize`, `scePthreadAttrGetschedparam`, `posix_pthread_barrier_init`, `sceKernelIsDevelopmentMode`, `sceKernelIsTestKit`, `sceKernelSendNotificationRequest`, `scePthreadMutexattrGetprotocol`, `vendor_system_version`

**libSceUlt** (11)

> `_sceUltMutexCreate`, `_sceUltConditionVariableCreate`, `_sceUltUlthreadCreate`, `_sceUltConditionVariableDestroy`, `_sceUltConditionVariableSignal`, `_sceUltConditionVariableSignalAll`, `_sceUltConditionVariableWait`, `_sceUltMutexDestroy`, `_sceUltMutexLock`, `_sceUltMutexTryLock`, `_sceUltMutexUnlock`

**libScePosix** (186)

> `open`, `pthread_mutex_init`, `pthread_mutexattr_destroy`, `pthread_mutexattr_init`, `pthread_mutexattr_settype`, `pthread_self`, `_close`, `_open`, `_read`, `accept`, `close`, `creat`, `fdatasync`, `fsync`, `getpagesize`, `getpeername`, `htonl`, `htons`, `inet_pton`, `listen`, `lseek`, `madvise`, `mprotect`, `munmap`, `ntohl`, `ntohs`, `posix_close`, `posix_fstat`, `posix_fsync`, `posix_getpid`, `posix_lseek`, `posix_mkdir`, `posix_open`, `posix_pread`, `posix_pthread_attr_destroy`, `posix_pthread_attr_getdetachstate`, `posix_pthread_attr_getguardsize`, `posix_pthread_attr_getinheritsched`, `posix_pthread_attr_getschedparam`, `posix_pthread_attr_getschedpolicy`, `posix_pthread_attr_getscope`, `posix_pthread_attr_getstacksize`, `posix_pthread_attr_init`, `posix_pthread_attr_setdetachstate`, `posix_pthread_attr_setguardsize`, `posix_pthread_attr_setinheritsched`, `posix_pthread_attr_setschedparam`, `posix_pthread_attr_setschedpolicy`, `posix_pthread_attr_setscope`, `posix_pthread_attr_setstacksize`, `posix_pthread_cond_broadcast`, `posix_pthread_cond_destroy`, `posix_pthread_cond_init`, `posix_pthread_cond_reltimedwait_np`, `posix_pthread_cond_signal`, `posix_pthread_cond_timedwait`, `posix_pthread_cond_wait`, `posix_pthread_condattr_destroy`, `posix_pthread_condattr_getclock`, `posix_pthread_condattr_getpshared`, `posix_pthread_condattr_init`, `posix_pthread_condattr_setclock`, `posix_pthread_condattr_setpshared`, `posix_pthread_create`, `posix_pthread_detach`, `posix_pthread_equal`, `posix_pthread_exit`, `posix_pthread_getspecific`, `posix_pthread_join`, `posix_pthread_key_create`, `posix_pthread_mutex_destroy`, `posix_pthread_mutex_init`, `posix_pthread_mutex_lock`, `posix_pthread_mutex_timedlock`, `posix_pthread_mutex_trylock`, `posix_pthread_mutex_unlock`, `posix_pthread_mutexattr_destroy`, `posix_pthread_mutexattr_getprioceiling`, `posix_pthread_mutexattr_getprotocol`, `posix_pthread_mutexattr_getpshared`, `posix_pthread_mutexattr_gettype`, `posix_pthread_mutexattr_init`, `posix_pthread_mutexattr_setprioceiling`, `posix_pthread_mutexattr_setprotocol`, `posix_pthread_mutexattr_setpshared`, `posix_pthread_mutexattr_settype`, `posix_pthread_once`, `posix_pthread_self`, `posix_pthread_setspecific`, `posix_pthread_yield`, `posix_pwritev`, `posix_read`, `posix_rename`, `posix_rmdir`, `posix_select`, `posix_sem_destroy`, `posix_sem_getvalue`, `posix_sem_init`, `posix_sem_post`, `posix_sem_reltimedwait_np`, `posix_sem_timedwait`, `posix_sem_trywait`, `posix_sem_wait`, `posix_stat`, `posix_write`, `preadv`, `pthread_attr_destroy`, `pthread_attr_getdetachstate`, `pthread_attr_getguardsize`, `pthread_attr_getinheritsched`, `pthread_attr_getschedparam`, `pthread_attr_getschedpolicy`, `pthread_attr_getscope`, `pthread_attr_getstacksize`, `pthread_attr_init`, `pthread_attr_setdetachstate`, `pthread_attr_setguardsize`, `pthread_attr_setinheritsched`, `pthread_attr_setschedparam`, `pthread_attr_setschedpolicy`, `pthread_attr_setscope`, `pthread_attr_setstacksize`, `pthread_barrier_destroy`, `pthread_barrier_init`, `pthread_barrier_wait`, `pthread_barrierattr_destroy`, `pthread_barrierattr_getpshared`, `pthread_barrierattr_init`, `pthread_barrierattr_setpshared`, `pthread_cond_broadcast`, `pthread_cond_destroy`, `pthread_cond_init`, `pthread_cond_reltimedwait_np`, `pthread_cond_signal`, `pthread_cond_timedwait`, `pthread_cond_wait`, `pthread_condattr_destroy`, `pthread_condattr_getclock`, `pthread_condattr_getpshared`, `pthread_condattr_init`, `pthread_condattr_setclock`, `pthread_condattr_setpshared`, `pthread_create`, `pthread_equal`, `pthread_exit`, `pthread_getconcurrency`, `pthread_join`, `pthread_mutex_destroy`, `pthread_mutex_lock`, `pthread_mutex_timedlock`, `pthread_mutex_trylock`, `pthread_mutex_unlock`, `pthread_mutexattr_getprioceiling`, `pthread_mutexattr_getprotocol`, `pthread_mutexattr_getpshared`, `pthread_mutexattr_gettype`, `pthread_mutexattr_setprioceiling`, `pthread_mutexattr_setprotocol`, `pthread_mutexattr_setpshared`, `pthread_once`, `pthread_rwlock_destroy`, `pthread_rwlock_init`, `pthread_rwlock_rdlock`, `pthread_rwlock_timedrdlock`, `pthread_rwlock_timedwrlock`, `pthread_rwlock_tryrdlock`, `pthread_rwlock_trywrlock`, `pthread_rwlock_unlock`, `pthread_rwlock_wrlock`, `pthread_rwlockattr_destroy`, `pthread_rwlockattr_getpshared`, `pthread_rwlockattr_gettype_np`, `pthread_rwlockattr_init`, `pthread_rwlockattr_setpshared`, `pthread_rwlockattr_settype_np`, `pthread_setconcurrency`, `pthread_yield`, `pwritev`, `read`, `readv`, `sched_yield`, `sem_getvalue`, `sem_reltimedwait_np`, `sem_timedwait`, `setsockopt`, `writev`

**libScePad** (3)

> `scePadDisconnectDevice`, `scePadIsValidHandle`, `scePadSetVibrationForce`


## The two layouts that block a whole subsystem each

Named separately because they are not one function's worth of value. In each case orbistoun has
the lifecycle calls implemented and cannot write the one that carries data, because doing so
means inventing a structure layout (its D500).

- **`scePadReadState`** - the pad state structure. orbistoun implements `scePadInit`, `Open`,
  `Close`, `SetVibration` and `SetLightBar`; it cannot report a button press. This one function
  is **519 of the 547 pad calls in orbistoun's whole 65-run corpus**. `libScePad` is censused
  here, so this is tier 2: a signature and a layout, not a console run.
- **`sceAudioOutOutput`** and `sceAudioOutOpen` - the port parameters. `sceAudioOutInit` and
  `Close` are implemented; nothing can open a port or write a sample.

### The layout does not need a reference - only the arity does

Worth splitting, because "needs a struct layout" reads as blocked on a document and most of it
is not. **`obs_report_written` already measures an extent**: it diffs a buffer before and after
a call and reports the last byte that *changed*, precisely so a trailing zeroed field is not
read as absent.

So for `scePadReadState`:

- **The arity is the part that needs a lawful reference**, and it is the part principle 2 is
  strict about - a wrong one corrupts the stack and crashes somewhere unrelated.
- **The layout is then measurable.** Call it with a generously oversized poisoned buffer and
  report the extent: that is the structure's size, taken rather than looked up. Call it twice
  with a control held down and the changed-byte positions are the field offsets, without anyone
  naming a field.

Same shape for `sceAudioOutOpen`, though it is the harder of the two: `Output` needs a handle,
so the parameters of `Open` gate it.

This is the general form of what the encoder out-parameter fix started (D303). A probe that
poisons and reports the extent measures structure without being told the structure, which is
the only way this project is allowed to learn one.

### Two more, both now walls in a live run rather than gaps in a list

Added 2026-09-03, when orbistoun's furthest-progressing run started dying on each in turn.

- **The event-queue event structure** - what `sceKernelWaitEqueue` writes into its output array.
  orbistoun creates queues and registers events against them and **cannot deliver one**, because
  what a caller reads back out of a delivered event is unknown. PPSA02664 calls it **1,177 times
  in a single run**: it submits a flip, registers a flip event, and then waits indefinitely for
  a completion nothing can report. `sceVideoOutAddFlipEvent` is the other half - what identifier
  a flip event carries.
  *Measurable by the extent method above*: create a queue, register a user event, trigger it,
  and `sceKernelWaitEqueue` into a poisoned oversized array. The changed-byte positions are the
  fields; the count answered says how many entries an array holds.

- **The shader object `sceAgcCreateShader` writes** - and this one is sharper than "a layout",
  because orbistoun knows exactly which byte it needs. The call takes a destination, a header
  and the bytecode; the caller then reads the **first quadword out of the destination and
  dereferences it at `+0x50`**. So the destination's first field is a pointer to an object of at
  least `0x51` bytes, and orbistoun cannot invent either level.
  Worth knowing: the caller does not test "did this fail". It tests the answer against
  **`0x8a6c003d`** specifically, so orbistoun's placeholder is read as success and the unwritten
  destination is dereferenced immediately. `0x8a6c` is libSceAgc's error family - the guest's own
  validating wrapper returns `0x8a6c000a` for a null argument and `0x8a6c0002` for a bad one,
  which is where the family comes from rather than from any document.

  **Two fields are now observed, not one, and the extent is measured (orbistoun D556,
  2026-09-04).** Reading the guest's own instructions at the fault: `8b 46 50` takes a dword at
  `+0x50` and `0f b6 d0` keeps only its low byte, so `+0x50` is read wide and used as one byte;
  the next instruction, `48 8b 46 30`, reads a **quadword at `+0x30`**. And arg0 is confirmed a
  pointer-*to*-pointer rather than a buffer - the faulting register was loaded from `[rsp+0x38]`,
  which is byte-for-byte the arg0 the call was given. Planting any readable pointer there carries
  the guest **past this site entirely**, so the unwritten out-parameter is the whole of what
  stands here; nothing else about the call is blocking.

  **This repository cannot answer it as the probe runs today**, and that is worth stating before
  a console day is spent on it. Four captures record **472 `libSceAgc` symbols as `absent`** -
  `OBS|sym|libSceAgc|sceAgcCreateShader|absent|current` in each - and `OBS|sysinfo|modlink/gpu`
  says plainly that no GPU library is mapped in the probe's process. The ask is therefore not
  *another capture of the same kind* but one taken **where libSceAgc is loaded**, which is a
  condition on the probe, not a symbol to add to a list.

**And one that is not a layout at all.** orbistoun models `libSceGnmDriver` - the previous
generation's graphics API. Guests in its corpus call **Agc**: `sceAgcCreateShader`,
`sceAgcDriverGetDefaultOwner`, `sceAgcDriverGetResourceRegistrationMaxNameLength`,
`sceAgcDriverInitResourceRegistration`, `sceAgcDriverQueryResourceRegistrationUserMemoryRequirements`,
`sceAgcDriverRegisterDefaultOwner`. **None of the six is declared or censused anywhere in this
repository**, so they are tier 3 - the cheap first move is putting the names in
`data/surface.txt`, where a wrong one is a harmless false negative (D014).

## Two asks about the *run*, not about a function

Everything above is organised by which symbol to call. These two are not - they are about the
conditions a capture is taken under and the shape of a check, which is a different axis and the
reason they were missing from a list that looked complete.

### A capture taken as application category 0

orbistoun has three measurements from `130-layout/memory-type` it cannot use, and **the reason
is not the memory type**. The capture ran as application category 65536, and the console's
resource arbitrator grants such a process zero bytes of direct memory whatever type it asks
for - so `wb-onion`, `wc-garlic` and `wb-garlic` all record a refusal that says nothing about
onion or garlic.

**This project already knows the difference**: its own D301 measured the same call succeeding
under category 0 at the same privilege. So the ask is a rerun of `130-layout` as a big app, and
three readings that currently describe an arbitrator decision start describing the memory
types they are named after.

Worth doing on the same visit as anything else here, because the category is a property of how
the package is built rather than of any single check - **every measurement in a capture inherits
it**, and a whole run taken under the wrong category has this problem everywhere and says so
nowhere.

### ~~A mutex attribute round-tripped through one object~~ - **already done, and I missed it**

Withdrawn. This check already round-trips through one object and the capture already carries the
result: `default 1`, then `0` refused and `1`-`4` reading back as themselves. Orbistoun has now
claimed all six against it, and fixed itself to refuse type 0 as the console does.

**The ask was written without reading the capture.** The check's own comment says exactly what
each record means - *"`read-back == type` is a clean round-trip, a differing value is a
normalisation, and -1 is a refusal"* - which is the whole answer, and it was in the file the
whole time.

### ~~And a guard word for the handle out-parameter~~ - **also already done**

Withdrawn for the same reason. `018-relational/handle-fits-its-out-parameter` already plants
`0xA5A5A5A5` after an `int handle`, and the capture records it read back as `0x0` with the check
reporting *"the call wrote past the end of the int it was given"*.

That one reversed a decision on the consuming side: orbistoun had narrowed its write to four
bytes from public interface documentation, and now writes eight because the console does.

## The four that are not "call it and see"

Everything above is answered by invoking the function and reporting what happened. These are
not, and each needs its own run designed.

### `scePthreadSetprio` and `scePthreadSetaffinity` - tier 2, but named here

PPSA02664 calls both, once each, in every run, and they are the **only two stub calls left in
that title's whole run** - 2,075 of 2,077 calls reach a real implementation. Tier 2 rather than
tier 1: the arity is unestablished, so writing a check would be inventing a signature.

The unblock is a lawful reference, not a console: FreeBSD's `pthread_setschedprio` and
`pthread_setaffinity_np` are the obvious analogues, and confirming the vendor forms match them
is the whole of the work. Forcing both to succeed was measured to change nothing, so this is
stub reduction rather than a wall.

### What `sceKernelLoadStartModule` does to a module that is already placed

The large one, and it needs hardware.

A title's own module - `/app0/Media/Modules/Il2CppUserAssemblies.prx` - is loaded and relocated
by orbistoun before the guest runs, and the guest then calls `sceKernelLoadStartModule` on it by
full path. orbistoun answers a handle and does nothing else. The module's `.bss` then shows
**two words changed out of 318,492** across a whole run, and the guest faults reading one of the
untouched ones.

Six places that could name code to run at load are all empty in that module: `DT_INIT_ARRAY`
and its size are zero, there is no `DT_PREINIT_ARRAY`, no `module_start`/`module_stop`/
`module_prolog` export, the ELF entry point is `0x0`, and `PT_SCE_MODULE_PARAM` carries SDK
versions. `DT_FINI` **is** a real offset, which is what makes the absence look deliberate rather
than undecoded.

**The run that settles it**: load a module obSCEne built, and report whether anything of the
module's own ran before the caller touched it - a byte written into its `.bss` by its own code,
or a `try` record from a constructor it deliberately contains. Either outcome is worth having;
"nothing ran" is as useful an answer as a name.

### The width of a vendor error return

orbistoun's decision 398 records the `0x8002_0000 | errno` encoding as measured across seven values from
five families, and leaves the *width* unencoded on purpose: every probe reports
`(uint64_t)(uint32_t)x` from a prototype returning `int`, so the upper thirty-two bits are the
cast rather than the console.

**An assembly thunk that reports `rax` verbatim** for one known-failing call would settle it.
Small, and it retires a caveat that has to be repeated at every comparison site - including
against most of tier 1 above, which makes it worth doing *before* the sweep rather than after.

### Out-parameters, which this project has now half-fixed

`106-encoder/path-probe` reported `res` for twenty-four paths after initialising it to zero, so
a platform that never writes the field and one that writes zero produced the same reading -
twenty-four measurements that separate nothing, and orbistoun marked all of them opaque
(its decision 497).

Now poisoned with `0xC7`, this project's own pattern byte (D303). **The general rule applies to
every tier-1 check that reports an out-parameter**: `obs_report_written` already makes this
argument for buffers, and a scalar out-parameter has the same problem for the same reason. Worth
settling before the sweep, for the same reason as the return width - it is a property of how
every check reports, not of any one of them.
