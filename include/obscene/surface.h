/*
 * The known symbol surface.
 *
 * # What this is for
 *
 * The behavioural sections answer "does this function work?" and cost a confident
 * signature each. This answers the cheaper question - "does this function exist at
 * all?" - which costs only a name, and so scales to the whole platform.
 *
 * That distinction matters because the honest headline for an emulator is the ratio
 * of implemented surface to needed surface. One commercial title imports around
 * 1,400 symbols. Fifty behavioural checks will never measure that; a census can.
 *
 * # These are declared as data, and that is deliberate
 *
 * Every name below is declared `const char`, not as a function. Only its **address**
 * is ever read, never called - and calling a function whose signature is unknown is
 * exactly the mistake D008 exists to prevent. Declaring them as data means the type
 * system refuses the call outright, so the rule is enforced by the compiler rather
 * than by anyone remembering it. A census of several hundred symbols would otherwise
 * be several hundred chances to get it wrong.
 *
 * A weak undefined symbol resolves to a null address rather than failing the link,
 * so an absent function is reported rather than fatal.
 *
 * # A wrong name here is harmless
 *
 * If a name in this list does not exist on the platform, the census reports it
 * absent. That is a false negative: visible, harmless, and correctable. Contrast a
 * wrong *arity* in the behavioural checks, which corrupts the stack and crashes
 * somewhere unrelated. The asymmetry is why this list can cast a much wider net than
 * platform.h is allowed to.
 *
 * Names come from public interface documentation and open-source homebrew
 * toolchains. See ACKNOWLEDGEMENTS.md.
 *
 * Regenerate with obscene-tool surface.
 */

#ifndef OBSCENE_SURFACE_H
#define OBSCENE_SURFACE_H

/* clang-format off
 *
 * These lists are data, not code. clang-format reads `X(name) X(name)` as a chain of
 * calls and reflows it into a cascading indent that is unreadable and - worse -
 * unstable: running it twice does not converge. Fenced off, and written one symbol
 * per line so adding or removing one is a one-line diff. */

/* The kernel: POSIX-shaped calls, then the vendor additions.
 *
 * Much of this half has a documented FreeBSD analogue, which is the strongest lawful
 * reference available for what these are supposed to do. */
#define OBS_SURFACE_LIBKERNEL(X) \
    X(sceKernelPread) \
    X(sceKernelPwrite) \
    X(sceKernelStat) \
    X(sceKernelFstat) \
    X(sceKernelMkdir) \
    X(sceKernelRmdir) \
    X(sceKernelUnlink) \
    X(sceKernelRename) \
    X(sceKernelTruncate) \
    X(sceKernelFtruncate) \
    X(sceKernelFsync) \
    X(sceKernelSync) \
    X(sceKernelGetdirentries) \
    X(sceKernelChmod) \
    X(sceKernelDup) \
    X(sceKernelDup2) \
    X(sceKernelFcntl) \
    X(sceKernelIoctl) \
    X(sceKernelSelect) \
    X(sceKernelPoll) \
    X(sceKernelUnlinkat) \
    X(sceKernelOpenat) \
    X(sceKernelMmap) \
    X(sceKernelMprotect) \
    X(sceKernelMsync) \
    X(sceKernelMlock) \
    X(sceKernelMunlock) \
    X(sceKernelMtypeprotect) \
    X(sceKernelQueryMemoryProtection) \
    X(sceKernelAvailableDirectMemorySize) \
    X(sceKernelMapNamedDirectMemory) \
    X(sceKernelMapNamedFlexibleMemory) \
    X(sceKernelCheckedReleaseDirectMemory) \
    X(sceKernelSetVirtualRangeName) \
    X(sceKernelReserveVirtualRange) \
    X(sceKernelGetDirectMemoryType) \
    X(sceKernelBatchMap) \
    X(sceKernelBatchMap2) \
    X(sceKernelGetpid) \
    X(sceKernelGetppid) \
    X(sceKernelExit) \
    X(sceKernelSleep) \
    X(sceKernelNanosleep) \
    X(sceKernelGettimeofday) \
    X(sceKernelClockGettime) \
    X(sceKernelClockGetres) \
    X(sceKernelGettimezone) \
    X(sceKernelGetCpumode) \
    X(sceKernelGetCurrentCpu) \
    X(sceKernelGetProcessType) \
    X(sceKernelGetProcParam) \
    X(sceKernelWaitSema) \
    X(sceKernelCancelSema) \
    X(sceKernelWaitEventFlag) \
    X(sceKernelCancelEventFlag) \
    X(sceKernelCreateEqueue) \
    X(sceKernelDeleteEqueue) \
    X(sceKernelWaitEqueue) \
    X(sceKernelAddUserEvent) \
    X(sceKernelAddTimerEvent) \
    X(sceKernelDeleteTimerEvent) \
    X(sceKernelAddReadEvent) \
    X(sceKernelTriggerUserEvent) \
    X(sceKernelGetEventData) \
    X(sceKernelGetEventId) \
    X(sceKernelGetEventFilter) \
    X(sceKernelGetEventUserData) \
    X(sceKernelStopUnloadModule) \
    X(sceKernelGetModuleInfoFromAddr) \
    X(sceKernelUuidCreate) \
    X(sceKernelGetSystemSwVersion2)

/* The threading half of the kernel library. POSIX threads with vendor naming, which
 * makes the pthread specification the reference for nearly all of it. */
#define OBS_SURFACE_PTHREAD(X) \
    X(scePthreadAttrSetstacksize) \
    X(scePthreadAttrGetstacksize) \
    X(scePthreadAttrSetinheritsched) \
    X(scePthreadAttrSetschedparam) \
    X(scePthreadAttrSetschedpolicy) \
    X(scePthreadAttrSetaffinity) \
    X(scePthreadAttrGetaffinity) \
    X(scePthreadDetach) \
    X(scePthreadExit) \
    X(scePthreadEqual) \
    X(scePthreadCancel) \
    X(scePthreadSetName) \
    X(scePthreadYield) \
    X(scePthreadGetprio) \
    X(scePthreadSetprio) \
    X(scePthreadGetaffinity) \
    X(scePthreadSetaffinity) \
    X(scePthreadOnce) \
    X(scePthreadKeyCreate) \
    X(scePthreadKeyDelete) \
    X(scePthreadGetspecific) \
    X(scePthreadSetspecific) \
    X(scePthreadGetthreadid) \
    X(scePthreadRename) \
    X(scePthreadMutexLock) \
    X(scePthreadMutexTimedlock) \
    X(scePthreadMutexattrSetprotocol) \
    X(scePthreadCondTimedwait) \
    X(scePthreadCondattrInit) \
    X(scePthreadCondattrDestroy) \
    X(scePthreadRwlockRdlock) \
    X(scePthreadRwlockWrlock)

/* POSIX under its own names, from libScePosix.
 *
 * The twelve that could be called without assuming a struct layout are declared in
 * platform.h and checked in 017-posix. These are the rest, and the reason they are here
 * rather than there is uniform: every one takes a `timespec`, a `sigaction`, a
 * `sockaddr` or an `mmap` argument, and a wrong layout produces a call that succeeds and
 * does the wrong thing (D008).
 *
 * Presence is the honest claim about them, and it is not a small one: this library is a
 * second implementation path onto the same kernel, so which half of it an emulator has
 * bothered with is worth knowing. */
#define OBS_SURFACE_POSIX(X) \
    X(posix_clock_getres) \
    X(posix_clock_gettime) \
    X(posix_clock_settime) \
    X(posix_ftruncate) \
    X(posix_gettimeofday) \
    X(posix_kevent) \
    X(posix_kqueue) \
    X(posix_mmap) \
    X(posix_mprotect) \
    X(posix_msync) \
    X(posix_munmap) \
    X(posix_nanosleep) \
    X(posix_pthread_cleanup_push) \
    X(posix_pthread_key_delete) \
    X(posix_pthread_kill) \
    X(posix_pthread_rwlock_rdlock) \
    X(posix_pthread_rwlock_timedrdlock) \
    X(posix_pthread_rwlock_timedwrlock) \
    X(posix_pthread_rwlock_wrlock) \
    X(posix_pthread_rwlockattr_destroy) \
    X(posix_pthread_rwlockattr_getpshared) \
    X(posix_pthread_rwlockattr_gettype_np) \
    X(posix_pthread_rwlockattr_init) \
    X(posix_pthread_rwlockattr_setpshared) \
    X(posix_pthread_rwlockattr_settype_np) \
    X(posix_pthread_setprio) \
    X(posix_pthread_sigmask) \
    X(posix_pwrite) \
    X(posix_settimeofday) \
    X(posix_sigaction) \
    X(posix_sigaltstack) \
    X(posix_signal) \
    X(posix_sigpending) \
    X(posix_sigprocmask) \
    X(posix_sleep) \
    X(posix_sysconf) \
    X(sys_accept) \
    X(sys_bind) \
    X(sys_connect) \
    X(sys_getpeername) \
    X(sys_getsockname) \
    X(sys_getsockopt) \
    X(sys_listen) \
    X(sys_recv) \
    X(sys_recvfrom) \
    X(sys_recvmsg) \
    X(sys_send) \
    X(sys_sendmsg) \
    X(sys_sendto) \
    X(sys_setsockopt) \
    X(sys_socket)

/* The C runtime. The largest single block of what a title actually imports, and the
 * only library here whose behaviour can be checked with total confidence - every
 * signature is ISO C.
 *
 * memcpy, memset and memmove are absent deliberately: this program defines its own
 * because the compiler emits calls to them regardless of -ffreestanding, and a local
 * definition would win over the import, so a check would be testing us rather than
 * the platform. Section 035-libc exercises the rest for behaviour, not presence. */
#define OBS_SURFACE_LIBC(X) \
    X(div) \
    X(ldiv) \
    X(vsnprintf) \
    X(vsprintf) \
    X(printf) \
    X(vprintf) \
    X(fprintf) \
    X(fputs) \
    X(fopen) \
    X(fclose) \
    X(fread) \
    X(fwrite) \
    X(fseek) \
    X(ftell) \
    X(fflush) \
    X(fgets) \
    X(feof) \
    X(ferror) \
    X(setvbuf) \
    X(abort) \
    X(atexit) \
    X(setjmp) \
    X(longjmp) \
    X(time) \
    X(clock) \
    X(gmtime) \
    X(localtime) \
    X(mktime) \
    X(strftime) \
    X(difftime) \
    X(wcscmp) \
    X(mbstowcs) \
    X(wcstombs) \
    X(__cxa_atexit) \
    X(__cxa_guard_acquire) \
    X(__cxa_guard_release) \
    X(__cxa_pure_virtual) \
    X(__stack_chk_fail)

/* The current generation's graphics interface.
 *
 * # Why this was absent until now
 *
 * The census carried `sceGnm*`, which is the *previous* generation's driver, and nothing
 * for the current one - the interface the emulator this exists to serve actually targets.
 * The blocker was provenance: the obvious name lists come from dumping decrypted
 * libraries, and inventing plausible names would fill the census with confident-looking
 * absences that mean nothing.
 *
 * # What closed it
 *
 * A current-generation emulator carries these as structured export attributes - NID, name
 * and library together - and an independent NID database carries most of the same names.
 * Both are public and neither required anything decrypted.
 *
 * # Every one is checked against our own hash
 *
 * A name is censused only where hashing it reproduces the identifier the source records.
 * That is not a formality: it rejected five, two of which were placeholders with the
 * identifier embedded in the name - a project marking a function it could not name, which
 * would have entered the census as a symbol that cannot exist.
 *
 * Marked OBS_CURRENT. Absence on previous-generation hardware is correct rather than a
 * gap, and counting it as one is the mistake that availability exists to prevent. */
#define OBS_SURFACE_AGC(X) \
    X(sceAgcAcbAcquireMem) \
    X(sceAgcAcbDispatchIndirect) \
    X(sceAgcAcbDmaData) \
    X(sceAgcAcbDmaDataGetSize) \
    X(sceAgcAcbEventWrite) \
    X(sceAgcAcbPopMarker) \
    X(sceAgcAcbPushMarker) \
    X(sceAgcAcbResetQueue) \
    X(sceAgcAcbWaitRegMem) \
    X(sceAgcAcbWriteData) \
    X(sceAgcCbDispatch) \
    X(sceAgcCbNop) \
    X(sceAgcCbReleaseMem) \
    X(sceAgcCbSetShRegisterRangeDirect) \
    X(sceAgcCbSetShRegistersDirect) \
    X(sceAgcCreatePrimState) \
    X(sceAgcCreateShader) \
    X(sceAgcDcbAcquireMem) \
    X(sceAgcDcbDispatchIndirect) \
    X(sceAgcDcbDmaData) \
    X(sceAgcDcbDmaDataGetSize) \
    X(sceAgcDcbDrawIndex) \
    X(sceAgcDcbDrawIndexAuto) \
    X(sceAgcDcbDrawIndexIndirect) \
    X(sceAgcDcbDrawIndexIndirectGetSize) \
    X(sceAgcDcbDrawIndexOffset) \
    X(sceAgcDcbEventWrite) \
    X(sceAgcDcbGetLodStats) \
    X(sceAgcDcbGetLodStatsGetSize) \
    X(sceAgcDcbJump) \
    X(sceAgcDcbPopMarker) \
    X(sceAgcDcbPushMarker) \
    X(sceAgcDcbResetQueue) \
    X(sceAgcDcbSetBaseIndirectArgs) \
    X(sceAgcDcbSetCxRegistersIndirect) \
    X(sceAgcDcbSetFlip) \
    X(sceAgcDcbSetIndexBuffer) \
    X(sceAgcDcbSetIndexCount) \
    X(sceAgcDcbSetIndexCountGetSize) \
    X(sceAgcDcbSetIndexSize) \
    X(sceAgcDcbSetNumInstances) \
    X(sceAgcDcbSetPredication) \
    X(sceAgcDcbSetShRegistersIndirect) \
    X(sceAgcDcbSetUcRegistersIndirect) \
    X(sceAgcDcbStallCommandBufferParser) \
    X(sceAgcDcbStallCommandBufferParserGetSize) \
    X(sceAgcDcbWaitRegMem) \
    X(sceAgcDcbWaitUntilSafeForRendering) \
    X(sceAgcDcbWriteData) \
    X(sceAgcDmaDataPatchSetDstAddressOrOffset) \
    X(sceAgcDmaDataPatchSetSrcAddressOrOffsetOrImmediate) \
    X(sceAgcDriverGetDefaultOwner) \
    X(sceAgcDriverGetResourceRegistrationMaxNameLength) \
    X(sceAgcDriverInitResourceRegistration) \
    X(sceAgcDriverQueryResourceRegistrationUserMemoryRequirements) \
    X(sceAgcDriverRegisterDefaultOwner) \
    X(sceAgcDriverRegisterOwner) \
    X(sceAgcDriverRegisterResource) \
    X(sceAgcDriverUnregisterResource) \
    X(sceAgcGetRegisterDefaults2) \
    X(sceAgcGetRegisterDefaults2Internal) \
    X(sceAgcQueueEndOfPipeActionPatchAddress) \
    X(sceAgcQueueEndOfPipeActionPatchData) \
    X(sceAgcQueueEndOfPipeActionPatchGcrCntl) \
    X(sceAgcQueueEndOfPipeActionPatchType) \
    X(sceAgcSetCxRegIndirectPatchAddRegisters) \
    X(sceAgcSetCxRegIndirectPatchSetAddress) \
    X(sceAgcSetPacketPredication) \
    X(sceAgcSetShRegIndirectPatchAddRegisters) \
    X(sceAgcSetShRegIndirectPatchSetAddress) \
    X(sceAgcSetUcRegIndirectPatchAddRegisters) \
    X(sceAgcSetUcRegIndirectPatchSetAddress) \
    X(sceAgcSuspendPoint) \
    X(sceAgcWaitRegMemPatchAddress) \
    X(sceAgcWaitRegMemPatchCompareFunction) \
    X(sceAgcWaitRegMemPatchMask) \
    X(sceAgcWaitRegMemPatchReference) \
    X(sceAgcWriteDataPatchSetAddressOrOffset) \
    X(sceAgcWriteDataPatchSetCachePolicy) \
    X(sceAgcWriteDataPatchSetDst)

#define OBS_SURFACE_AGCDRIVER(X) \
    X(sceAgcDriverAddEqEvent) \
    X(sceAgcDriverDeleteEqEvent) \
    X(sceAgcDriverSetHsOffchipParam) \
    X(sceAgcDriverSetTFRing) \
    X(sceAgcDriverSubmitAcb) \
    X(sceAgcDriverSubmitDcb) \
    X(sceAgcDriverSubmitMultiDcbs)

#define OBS_SURFACE_VIDEOOUT(X) \
    X(sceVideoOutUnregisterBuffers) \
    X(sceVideoOutUnregisterBuffer) \
    X(sceVideoOutAddFlipEvent) \
    X(sceVideoOutAddVblankEvent) \
    X(sceVideoOutDeleteFlipEvent) \
    X(sceVideoOutGetEventId) \
    X(sceVideoOutGetEventData) \
    X(sceVideoOutGetEventCount) \
    X(sceVideoOutIsFlipPending) \
    X(sceVideoOutGetVblankStatus) \
    X(sceVideoOutSetWindowModeMargins) \
    X(sceVideoOutAdjustColor)

#define OBS_SURFACE_AUDIOOUT(X) \
    X(sceAudioOutOutput) \
    X(sceAudioOutOutputs) \
    X(sceAudioOutSetVolume) \
    X(sceAudioOutGetLastOutputTime) \
    X(sceAudioOutGetPortState) \
    X(sceAudioOutGetSystemState) \
    X(sceAudioOutSetMixLevelPadSpk)

#define OBS_SURFACE_PAD(X) \
    X(scePadRead) \
    X(scePadSetLightBar) \
    X(scePadResetLightBar) \
    X(scePadSetVibration) \
    X(scePadGetControllerInformation) \
    X(scePadGetHandle) \
    X(scePadSetMotionSensorState) \
    X(scePadResetOrientation) \
    X(scePadOpenExt) \
    X(scePadSetTiltCorrectionState) \
    X(scePadGetExtControllerInformation)

/* One name from this library is absent here deliberately: it is declared with a real
 * signature in platform.h, and a name cannot be both a function and a const char. Its
 * presence is already implied by the behavioural check that calls it. */
#define OBS_SURFACE_SYSMODULE(X) \
    X(sceSysmoduleUnloadModule) \
    X(sceSysmoduleLoadModuleInternal) \
    X(sceSysmoduleUnloadModuleInternal) \
    X(sceSysmoduleIsLoadedInternal)

/* One name from this library is absent here deliberately: it is declared with a real
 * signature in platform.h, and a name cannot be both a function and a const char. Its
 * presence is already implied by the behavioural check that calls it. */
#define OBS_SURFACE_USERSERVICE(X) \
    X(sceUserServiceGetUserName) \
    X(sceUserServiceGetLoginUserIdList) \
    X(sceUserServiceGetEvent) \
    X(sceUserServiceGetUserColor) \
    X(sceUserServiceGetForegroundUser)

#define OBS_SURFACE_SYSTEMSERVICE(X) \
    X(sceSystemServiceHideSplashScreen) \
    X(sceSystemServiceGetStatus) \
    X(sceSystemServiceParamGetInt) \
    X(sceSystemServiceParamGetString) \
    X(sceSystemServiceLoadExec)

#define OBS_SURFACE_SAVEDATA(X) \
    X(sceSaveDataInitialize) \
    X(sceSaveDataInitialize3) \
    X(sceSaveDataTerminate) \
    X(sceSaveDataMount) \
    X(sceSaveDataUmount) \
    X(sceSaveDataDirNameSearch) \
    X(sceSaveDataDelete) \
    X(sceSaveDataSaveIcon)

#define OBS_SURFACE_TROPHY(X) \
    X(sceNpTrophyCreateContext) \
    X(sceNpTrophyCreateHandle) \
    X(sceNpTrophyRegisterContext) \
    X(sceNpTrophyUnlockTrophy) \
    X(sceNpTrophyDestroyContext) \
    X(sceNpTrophyDestroyHandle)

/* The previous generation graphics driver - the layer with the most public
 * documentation. The current generation equivalent is a different interface and
 * belongs in its own list once its names are confident. */
#define OBS_SURFACE_GRAPHICS(X) \
    X(sceGnmSubmitCommandBuffers) \
    X(sceGnmSubmitAndFlipCommandBuffers) \
    X(sceGnmSubmitDone) \
    X(sceGnmAreSubmitsAllowed) \
    X(sceGnmDrawIndex) \
    X(sceGnmDrawIndexAuto) \
    X(sceGnmSetVsShader) \
    X(sceGnmSetPsShader) \
    X(sceGnmSetCsShader) \
    X(sceGnmSetEmbeddedVsShader) \
    X(sceGnmDrawInitDefaultHardwareState) \
    X(sceGnmInsertWaitFlipDone) \
    X(sceGnmFlushGarlic) \
    X(sceGnmGetGpuCoreClockFrequency) \
    X(sceGnmRequestFlipAndSubmitDone) \
    X(sceGnmMapComputeQueue) \
    X(sceGnmDingDong)

#define OBS_SURFACE_DIALOG(X) \
    X(sceCommonDialogInitialize) \
    X(sceMsgDialogInitialize) \
    X(sceMsgDialogOpen) \
    X(sceMsgDialogUpdateStatus) \
    X(sceMsgDialogTerminate) \
    X(sceImeDialogInit) \
    X(sceImeDialogGetStatus)

/* Init/Term/Socket/Bind/Send/Recv/SocketClose have moved to platform.h as real
 * declarations - the console socket transport calls them (src/net_target.c), and a name
 * cannot be both censused (const char, uncallable) and called. Listen and Accept are new,
 * declared there too. What remains here is the part still probed for presence only. */
#define OBS_SURFACE_NET(X) \
    X(sceNetConnect) \
    X(sceNetEpollCreate) \
    X(sceNetResolverCreate) \
    X(sceNetGetsockname) \
    X(sceNetHtons) \
    X(sceNetInetPton)

/* Every list, paired with an identifier tag, the library that provides it, and which
 * console generation it belongs to.
 *
 * The tag exists because a string literal cannot be pasted into an identifier, and
 * the census generates one table and one function per group. Adding a group is one
 * line here; each symbol name still appears exactly once, in its list above. */
#define OBS_SURFACE_LIBRARIES(L) \
    L(kernel, "libkernel", OBS_SHARED, OBS_SURFACE_LIBKERNEL) \
    L(pthread, "libkernel", OBS_SHARED, OBS_SURFACE_PTHREAD) \
    L(posix, "libScePosix", OBS_SHARED, OBS_SURFACE_POSIX) \
    L(libc, "libSceLibcInternal", OBS_SHARED, OBS_SURFACE_LIBC) \
    L(agc, "libSceAgc", OBS_CURRENT, OBS_SURFACE_AGC) \
    L(agcdriver, "libSceAgcDriver", OBS_CURRENT, OBS_SURFACE_AGCDRIVER) \
    L(videoout, "libSceVideoOut", OBS_SHARED, OBS_SURFACE_VIDEOOUT) \
    L(audioout, "libSceAudioOut", OBS_SHARED, OBS_SURFACE_AUDIOOUT) \
    L(pad, "libScePad", OBS_SHARED, OBS_SURFACE_PAD) \
    L(sysmodule, "libSceSysmodule", OBS_SHARED, OBS_SURFACE_SYSMODULE) \
    L(userservice, "libSceUserService", OBS_SHARED, OBS_SURFACE_USERSERVICE) \
    L(systemservice, "libSceSystemService", OBS_SHARED, OBS_SURFACE_SYSTEMSERVICE) \
    L(savedata, "libSceSaveData", OBS_SHARED, OBS_SURFACE_SAVEDATA) \
    L(trophy, "libSceNpTrophy", OBS_SHARED, OBS_SURFACE_TROPHY) \
    L(graphics, "libSceGnmDriver", OBS_PREVIOUS, OBS_SURFACE_GRAPHICS) \
    L(dialog, "libSceCommonDialog", OBS_SHARED, OBS_SURFACE_DIALOG) \
    L(net, "libSceNet", OBS_SHARED, OBS_SURFACE_NET)

/* clang-format on */

#endif /* OBSCENE_SURFACE_H */
