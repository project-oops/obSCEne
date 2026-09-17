/*
 * Which library each imported symbol comes from.
 *
 * # Why this exists
 *
 * A module encodes each import as a NID plus a library id and a module id. An id with
 * no declared library resolves to nothing, so the loader needs to be told - and a
 * `.dynsym` entry does not carry it. It records that a symbol is undefined, not who
 * is expected to define it.
 *
 * The census in surface.h already carries the association for the several hundred
 * names it lists, and publishes it through `obs_surface_each_symbol`. This covers the
 * other two kinds of import: the behavioural declarations in platform.h, and the
 * handful declared ad hoc inside a section file.
 *
 * # It cannot silently go stale
 *
 * `obscene-tool mkmodule` refuses to build a module when an undefined symbol has no
 * library here. Adding an import and forgetting this file fails the build and names
 * the symbol, rather than producing a module that half-resolves - which is the same
 * outcome as not building one, discovered much later.
 *
 * The initial contents were extracted from platform.h's group comments, which is why
 * the order matches that file. The comments are no longer what anything reads.
 */

#include <stddef.h>

#include "oops/target.h"
#include "obscene/platform.h"

typedef struct obs_import {
    const char *library;
    const char *symbol;
} obs_import;

static const obs_import obs_platform_imports[] = {
    {"libkernel", "sceKernelGetProcessTime"},
    {"libkernel", "sceKernelGetProcessTimeCounter"},
    {"libkernel", "sceKernelGetTscFrequency"},
    {"libkernel", "sceKernelWrite"},
    {"libkernel", "sceKernelDebugOutText"},
    {"libkernel", "sceKernelRead"},
    {"libkernel", "sceKernelOpen"},
    {"libkernel", "sceKernelClose"},
    {"libkernel", "sceKernelGetdents"},
    {"libkernel", "sceKernelLseek"},
    {"libkernel", "sceKernelJitCreateSharedMemory"},
    {"libkernel", "sceKernelJitMapSharedMemory"},
    {"libkernel", "sceKernelJitCreateAliasOfSharedMemory"},
    {"libkernel", "sceKernelGetDirectMemorySize"},
    {"libkernel", "sceKernelAllocateDirectMemory"},
    {"libkernel", "sceKernelAllocateMainDirectMemory"},
    {"libkernel", "sceKernelVirtualQuery"},
    {"libkernel", "sceKernelReleaseDirectMemory"},
    {"libkernel", "sceKernelMapDirectMemory"},
    {"libkernel", "sceKernelMunmap"},
    {"libkernel", "sceKernelProtectDirectMemory"},
    {"libkernel", "sceKernelBatchMap"},
    {"libkernel", "sceKernelDirectMemoryQuery"},
    {"libkernel", "sceKernelReserveVirtualRange"},
    {"libkernel", "sceKernelUsleep"},
    {"libkernel", "sceKernelIsNeoMode"},
    {"libkernel", "scePthreadSelf"},
    {"libkernel", "scePthreadCreate"},
    {"libkernel", "scePthreadJoin"},
    {"libkernel", "scePthreadExit"},
    {"libkernel", "scePthreadAttrSetstacksize"},
    {"libkernel", "scePthreadAttrSetaffinity"},
    {"libkernel", "scePthreadAttrSetschedparam"},
    {"libkernel", "scePthreadGetprio"},
    {"libkernel", "scePthreadGetaffinity"},
    {"libkernel", "_sigaction"},
    {"libkernel", "_sigprocmask"},
    {"libkernel", "sceKernelLoadStartModule"},
    {"libkernel", "sceKernelDlsym"},
    {"libkernel", "__error"},
    {"libkernel", "__sys_socketex"},
    {"libkernel", "_sendto"},
    {"libkernel", "_setsockopt"},
    {"libkernel", "accept"},
    {"libkernel", "bind"},
    {"libkernel", "close"},
    {"libkernel", "connect"},
    {"libkernel", "listen"},
    {"libkernel", "recv"},
    {"libSceLibcInternal", "strlen"},
    {"libSceLibcInternal", "strcmp"},
    {"libSceLibcInternal", "strncmp"},
    {"libSceLibcInternal", "strchr"},
    {"libSceLibcInternal", "strrchr"},
    {"libSceLibcInternal", "strncpy"},
    {"libSceLibcInternal", "strcat"},
    {"libSceLibcInternal", "strstr"},
    {"libSceLibcInternal", "memcmp"},
    {"libSceLibcInternal", "memchr"},
    {"libSceLibcInternal", "malloc"},
    {"libSceLibcInternal", "calloc"},
    {"libSceLibcInternal", "realloc"},
    {"libSceLibcInternal", "free"},
    {"libSceLibcInternal", "strcpy"},
    {"libSceLibcInternal", "strspn"},
    {"libSceLibcInternal", "strcspn"},
    {"libSceLibcInternal", "strtok"},
    {"libSceLibcInternal", "snprintf"},
    {"libSceLibcInternal", "atoi"},
    {"libSceLibcInternal", "strtol"},
    {"libSceLibcInternal", "strtoul"},
    {"libSceLibcInternal", "abs"},
    {"libSceLibcInternal", "qsort"},
    {"libSceLibcInternal", "bsearch"},
    {"libSceLibcInternal", "rand"},
    {"libSceLibcInternal", "srand"},
    {"libSceLibcInternal", "toupper"},
    {"libSceLibcInternal", "tolower"},
    {"libSceLibcInternal", "isdigit"},
    {"libSceLibcInternal", "isalpha"},
    {"libSceLibcInternal", "isspace"},
    {"libSceLibcInternal", "isupper"},
    /* The ctype table accessors the isxxx/toxxx macros expand to - undefined references
     * the checks above pull in by using ctype, resolved from the same library. The
     * mined corpus places them here. (D290) */
    {"libSceLibcInternal", "_Getpctype"},
    {"libSceLibcInternal", "_Getptolower"},
    {"libSceLibcInternal", "_Getptoupper"},
    {"libSceLibcInternal", "sqrt"},
    {"libSceLibcInternal", "pow"},
    {"libSceLibcInternal", "fabs"},
    {"libSceLibcInternal", "floor"},
    {"libSceLibcInternal", "ceil"},
    {"libSceLibcInternal", "fmod"},
    {"libSceLibcInternal", "sin"},
    {"libSceLibcInternal", "cos"},
    {"libSceLibcInternal", "sqrtf"},
    {"libSceLibcInternal", "fabsf"},
    {"libSceSysmodule", "sceSysmoduleLoadModule"},
    {"libSceSysmodule", "sceSysmoduleIsLoaded"},
    {"libSceUserService", "sceUserServiceInitialize"},
    {"libSceUserService", "sceUserServiceGetInitialUser"},
    {"libSceUserService", "sceUserServiceTerminate"},
    {"libSceVideoOut", "sceVideoOutOpen"},
    {"libSceVideoOut", "sceVideoOutClose"},
    {"libSceVideoOut", "sceVideoOutSetFlipRate"},
    {"libSceVideoRecording", "sceVideoRecordingQueryMemSize"},
    {"libSceVideoRecording", "sceVideoRecordingClose"},
    {"libSceVideoRecording", "sceVideoRecordingStop"},
    {"libSceVideoRecording", "sceVideoRecordingGetStatus"},
    {"libSceAudioOut", "sceAudioOutInit"},
    {"libSceAudioOut", "sceAudioOutOpen"},
    {"libSceAudioOut", "sceAudioOutClose"},
    {"libSceAudioOut", "sceAudioOutOutput"},
    {"libSceAudioOut", "sceAudioOutSetVolume"},
    {"libSceAudioOut", "sceAudioOutGetPortState"},
    {"libScePad", "scePadInit"},
    {"libScePad", "scePadOpen"},
    {"libScePad", "scePadClose"},
    {"libScePad", "scePadReadState"},
    {"libScePad", "scePadRead"},
    {"libScePad", "scePadSetLightBar"},
    {"libScePad", "scePadSetVibration"},
    {"libScePad", "scePadSetTriggerEffect"},
    {"libScePad", "scePadGetTriggerEffectState"},
    {"libScePad", "scePadGetControllerInformation"},
    {"libSceKeyboard", "sceKeyboardInit"},
    {"libSceKeyboard", "sceKeyboardOpen"},
    {"libSceKeyboard", "sceKeyboardClose"},
    {"libSceKeyboard", "sceKeyboardReadState"},
    {"libSceMouse", "sceMouseInit"},
    {"libSceMouse", "sceMouseOpen"},
    {"libSceMouse", "sceMouseClose"},
    {"libSceMouse", "sceMouseRead"},
    {"libSceVideodec2", "sceVideodec2CreateDecoder"},
    {"libSceVideodec2", "sceVideodec2DeleteDecoder"},
    {"libSceVideodec2", "sceVideodec2Decode"},
    {"libSceVideodec2", "sceVideodec2Flush"},
    {"libSceVideodec2", "sceVideodec2Reset"},
    {"libSceVideodec2", "sceVideodec2QueryComputeMemoryInfo"},
    {"libSceVideodec2", "sceVideodec2QueryDecoderMemoryInfo"},
    {"libSceVideodec2", "sceVideodec2AllocateComputeQueue"},
    {"libSceVideodec2", "sceVideodec2ReleaseComputeQueue"},
    {"libSceVideodec2", "sceVideodec2MapDirectMemory"},
    {"libSceVideodec2", "sceVideodec2GetPictureInfo"},
    {"libSceVideodec2", "sceVideodec2GetAvcPictureInfo"},
    {"libSceAudiodec", "sceAudiodecInitLibrary"},
    {"libSceAudiodec", "sceAudiodecTermLibrary"},
    {"libSceAudiodec", "sceAudiodecCreateDecoder"},
    {"libSceAudiodec", "sceAudiodecDeleteDecoder"},
    {"libSceAudiodec", "sceAudiodecDecode"},
    {"libSceAudiodec", "sceAudiodecDecode2"},
    {"libSceAudiodec", "sceAudiodecDecodeWithPriority"},
    {"libSceAudiodec", "sceAudiodecDecode2WithPriority"},
    {"libSceAudiodec", "sceAudiodecClearContext"},
    /* The two extra output channels. See obs_write in runtime.c: an emulator that
     * stubs sceKernelWrite discards the whole report, so there is more than one way
     * out. */
    {"libkernel", "write"},
    {"libSceLibcInternal", "putchar"},
    {"libSceLibcInternal", "puts"},

    /* More of the C runtime, made callable. See src/sections/libc.c. */
    {"libSceLibcInternal", "strncat"},
    {"libSceLibcInternal", "strpbrk"},
    {"libSceLibcInternal", "strcasecmp"},
    {"libSceLibcInternal", "atol"},
    {"libSceLibcInternal", "strtoll"},
    {"libSceLibcInternal", "labs"},
    {"libSceLibcInternal", "islower"},
    {"libSceLibcInternal", "isalnum"},
    {"libSceLibcInternal", "isprint"},
    {"libSceLibcInternal", "ispunct"},
    {"libSceLibcInternal", "wcslen"},
    {"libSceLibcInternal", "getenv"},

    /* The settled C library surface, promoted from the census to real checks.
     * See src/sections/libc.c. */
    {"libSceLibcInternal", "atoll"},
    {"libSceLibcInternal", "strtoull"},
    {"libSceLibcInternal", "llabs"},
    {"libSceLibcInternal", "strncasecmp"},
    {"libSceLibcInternal", "strdup"},
    {"libSceLibcInternal", "sprintf"},

    /* The rest of the maths library. See src/sections/math.c. */
    {"libSceLibcInternal", "round"},
    {"libSceLibcInternal", "trunc"},
    {"libSceLibcInternal", "exp"},
    {"libSceLibcInternal", "log"},
    {"libSceLibcInternal", "log2"},
    {"libSceLibcInternal", "log10"},
    {"libSceLibcInternal", "tan"},
    {"libSceLibcInternal", "asin"},
    {"libSceLibcInternal", "acos"},
    {"libSceLibcInternal", "atan"},
    {"libSceLibcInternal", "atan2"},
    {"libSceLibcInternal", "floorf"},
    {"libSceLibcInternal", "ceilf"},
    {"libSceLibcInternal", "fmodf"},
    {"libSceLibcInternal", "powf"},
    {"libSceLibcInternal", "expf"},
    {"libSceLibcInternal", "logf"},
    {"libSceLibcInternal", "sinf"},
    {"libSceLibcInternal", "cosf"},
    {"libSceLibcInternal", "tanf"},
    {"libSceLibcInternal", "strtod"},
    {"libSceLibcInternal", "strtof"},

    /* Condition variables and barriers. See src/sections/sync.c. */
    {"libkernel", "scePthreadCondInit"},
    {"libkernel", "scePthreadCondDestroy"},
    {"libkernel", "scePthreadCondSignal"},
    {"libkernel", "scePthreadCondBroadcast"},
    {"libkernel", "scePthreadCondWait"},
    {"libkernel", "scePthreadBarrierInit"},
    {"libkernel", "scePthreadBarrierDestroy"},
    {"libkernel", "scePthreadBarrierWait"},

    /* From the emulator gap analysis. See src/sections/os.c. */
    {"libkernel", "sceKernelIsStack"},
    {"libkernel", "scePthreadAttrInit"},
    {"libkernel", "scePthreadAttrDestroy"},
    {"libkernel", "scePthreadAttrSetdetachstate"},
    {"libkernel", "scePthreadAttrGetdetachstate"},
    {"libkernel", "scePthreadAttrGet"},
    {"libkernel", "scePthreadAttrGetstackaddr"},
    {"libkernel", "scePthreadAttrGetstacksize"},
    /* The futex pair (`sceKernelSyncOnAddressWait`/`Wake`) is deliberately NOT a linked
     * import. Its export library `libkernel_sync_on_address` is a namespace inside
     * `libkernel.sprx`, not a loadable module - so declaring it here made the title
     * module demand a `needed_module` for a `.sprx` the loader cannot find, and the
     * title died on load while the payload ran. `032-syncaddr` resolves the pair by
     * name through `libkernel` at run time instead. (D321-adjacent; reverts D322's
     * linked import.) */

    /* Flexible memory. See src/sections/memory.c. */
    {"libkernel", "sceKernelAvailableFlexibleMemorySize"},
    {"libkernel", "sceKernelConfiguredFlexibleMemorySize"},
    {"libkernel", "sceKernelMapFlexibleMemory"},
    {"libkernel", "sceKernelReleaseFlexibleMemory"},

    /* The console socket transport. See src/net_target.c. Moved out of the census in
     * surface.h because they are called rather than merely probed for presence. */
    {"libSceNet", "sceNetInit"},
    {"libSceNet", "sceNetTerm"},
    {"libSceNet", "sceNetSocket"},
    {"libSceNet", "sceNetBind"},
    {"libSceNet", "sceNetListen"},
    {"libSceNet", "sceNetAccept"},
    {"libSceNet", "sceNetRecv"},
    {"libSceNet", "sceNetSend"},
    {"libSceNet", "sceNetSocketClose"},
    {"libSceNet", "sceNetSetsockopt"},
    {"libSceNet", "sceNetConnect"},

#if !OOPS_TARGET_IS_PROSPERO
    /* The GPU command-builders (src/sections/gnm.c). Only on Orbis targets (Orbis/Neo).
     * Excluded on Prospero targets so libSceGnmDriver is not in DT_NEEDED. */
    {"libSceGnmDriver", "sceGnmDispatchInitDefaultHardwareState"},
    {"libSceGnmDriver", "sceGnmDispatchDirect"},
    /* Called by checks and, until now, declared only by the census.
     *
     * That worked because the census imports every name it lists, so the association
     * existed
     * - in the wrong file. A build that does not link the census (D227) has these as
     * undefined symbols nothing claims, and `mkmodule` refuses it by name, which is how
     * they were found:
     *
     *     error: 4 imported symbol(s) have no library ... sceGnmDrawIndex,
     * sceGnmSubmitDone
     *
     * A symbol a check *calls* belongs here whether or not the census also lists it.
     * The libraries are the census's own, not a guess: `libSceGnmDriver` from the
     * `graphics` group and `libSceAgc` from `agc`. */
    {"libSceGnmDriver", "sceGnmDrawIndex"},
    {"libSceGnmDriver", "sceGnmSubmitCommandBuffers"},
    {"libSceGnmDriver", "sceGnmSubmitDone"},
#endif

#if !OOPS_TARGET_IS_ORBIS
    /* libSceAgc: current-generation GPU command builders and shaders */
    {"libSceAgc", "$23LRUSvYu1M"},
    {"libSceAgc", "sceAgcInit"},
    {"libSceAgc", "$BfBDZGbti7A"},
    {"libSceAgc", "sceAgcGetIsTrinityMode"},
    {"libSceAgc", "sceAgcCbNop"},
    {"libSceAgc", "sceAgcCbReleaseMem"},
    {"libSceAgc", "sceAgcDcbDmaData"},
    {"libSceAgc", "sceAgcDcbWaitRegMem"},
    {"libSceAgc", "sceAgcDcbResetQueue"},
    {"libSceAgc", "$f3dg2CSgRKY"},
    {"libSceAgc", "$fYZQG4CU71c"},
    {"libSceAgc", "sceAgcCreateShader"},
    {"libSceAgc", "$nQT5kYLv0cg"},
    {"libSceAgc", "sceAgcGetFusedShaderSize"},
    {"libSceAgc", "$nApJjpKNBl4"},
    {"libSceAgc", "sceAgcFuseShaderHalves"},
    {"libSceAgc", "$Yw0jKSqop+E"},
    {"libSceAgc", "sceAgcDcbDrawIndexAuto"},
    {"libSceAgc", "$LHFXRrlTPD8"},
    {"libSceAgc", "sceAgcDcbSetCxRegisterDirect"},
    {"libSceAgc", "$w4-d0n60hdo"},
    {"libSceAgc", "sceAgcDcbSetUcRegisterDirect"},
    {"libSceAgc", "$n2fD4A+pb+g"},
    {"libSceAgc", "sceAgcCbSetShRegisterRangeDirect"},
    {"libSceAgc", "$D9sr1xGUriE"},
    {"libSceAgc", "sceAgcCreatePrimState"},
    {"libSceAgc", "$pdEV7bI6COI"},
    {"libSceAgc", "sceAgcCreateInterpolantMapping"},
    {"libSceAgc", "$73ZZdojLIgs"},
    {"libSceAgc", "sceAgcDcbSetCfRegisterDirect"},
    {"libSceAgc", "$pFLArOT53+w"},
    {"libSceAgc", "sceAgcDcbSetShRegisterDirect"},
    {"libSceAgc", "$SbuY2jN+axQ"},
    {"libSceAgc", "sceAgcUpdateInterpolantMapping"},
    {"libSceAgc", "$Y3ymLfZ1384"},
    {"libSceAgc", "sceAgcUpdatePrimState"},
    {"libSceAgc", "$MqAdbRMdNz4"},
    {"libSceAgc", "sceAgcLinkShaders"},
    {"libSceAgc", "$l4fM9K-Lyks"},
    {"libSceAgc", "sceAgcDcbSetIndexBuffer"},
    {"libSceAgc", "$GIIW2J37e70"},
    {"libSceAgc", "sceAgcDcbSetIndexSize"},
    {"libSceAgc", "$8N2tmT3jmC8"},
    {"libSceAgc", "sceAgcDcbSetIndexCount"},
    {"libSceAgc", "$q88lQ+GP5Yk"},
    {"libSceAgc", "sceAgcDcbDrawIndex"},
    /* libSceAgcDriver: current-generation GPU driver submission & resource registration
     */
    {"libSceAgcDriver", "sceAgcDriverCreateQueue"},
    {"libSceAgcDriver", "sceAgcDriverDestroyQueue"},
    {"libSceAgcDriver", "sceAgcDriverSubmitDcb"},
    {"libSceAgcDriver", "sceAgcDriverSubmitCommandBuffer"},
    {"libSceAgcDriver", "$AOLcoIkQDgM"},
    {"libSceAgcDriver", "sceAgcDriverQueryResourceRegistrationUserMemoryRequirements"},
    {"libSceAgcDriver", "$F0Y42t-3e18"},
    {"libSceAgcDriver", "sceAgcDriverInitResourceRegistration"},
    {"libSceAgcDriver", "$X-Nm5KLREeg"},
    {"libSceAgcDriver", "sceAgcDriverRegisterOwner"},
    {"libSceAgcDriver", "$W5z4eZrjEas"},
    {"libSceAgcDriver", "sceAgcDriverRegisterResource"},
    {"libSceAgcDriver", "$U9ueyEhSkF4"},
    {"libSceAgcDriver", "sceAgcDriverRegisterDefaultOwner"},
    {"libSceAgcDriver", "$F0ZXt5q0ZTA"},
    {"libSceAgcDriver", "sceAgcDriverGetDefaultOwner"},
#endif

    /* Address-probed by the HUD (src/sysinfo.c), never called - its struct layout is
     * unconfirmed. Listed so mkmodule knows the library the presence probe imports
     * from. */
    {"libSceNetCtl", "sceNetCtlInit"},
    {"libSceNetCtl", "sceNetCtlGetInfo"},

    /* Calls that fill a buffer. See src/sections/layout.c. */
    {"libkernel", "sceKernelDirectMemoryQuery"},
    {"libkernel", "sceKernelGetSystemSwVersion"},
    {"libSceVideoOut", "sceVideoOutGetResolutionStatus"},
    {"libSceVideoOut", "sceVideoOutGetFlipStatus"},

    /* Escaping the sandbox to write the report where ftp can read it (mkdir a path
     * outside the jail; a timestamp for it). Added for the second thread's disk-escape
     * work; libkernel by the sceKernel* prefix, unambiguous. */
    {"libkernel", "sceKernelMkdir"},
    {"libkernel", "sceKernelGettimeofday"},

    /* The measuring instrument. See src/sections/measure.c. */
    {"libkernel", "sceKernelReadTsc"},
    {"libkernel", "sceKernelGetProcessTimeCounterFrequency"},

    /* POSIX synchronisation. See src/sections/sync.c. */
    {"libkernel", "scePthreadMutexattrInit"},
    {"libkernel", "scePthreadMutexattrDestroy"},
    {"libkernel", "scePthreadMutexattrSettype"},
    {"libkernel", "scePthreadMutexattrGettype"},
    {"libkernel", "scePthreadMutexInit"},
    {"libkernel", "scePthreadMutexDestroy"},
    {"libkernel", "scePthreadMutexTrylock"},
    {"libkernel", "scePthreadMutexUnlock"},
    {"libkernel", "scePthreadRwlockInit"},
    {"libkernel", "scePthreadRwlockDestroy"},
    {"libkernel", "scePthreadRwlockTryrdlock"},
    {"libkernel", "scePthreadRwlockTrywrlock"},
    {"libkernel", "scePthreadRwlockUnlock"},
    {"libkernel", "sceKernelCreateSema"},
    {"libkernel", "sceKernelDeleteSema"},
    {"libkernel", "sceKernelSignalSema"},
    {"libkernel", "sceKernelPollSema"},

    /* Event flags, and which machine this is. See src/sections/sync.c. */
    {"libkernel", "sceKernelCreateEventFlag"},
    {"libkernel", "sceKernelDeleteEventFlag"},
    {"libkernel", "sceKernelSetEventFlag"},
    {"libkernel", "sceKernelClearEventFlag"},
    {"libkernel", "sceKernelPollEventFlag"},
    {"libkernel", "sceKernelIsDevkit"},
    {"libkernel", "sceKernelIsCex"},
    /* Named `libkernel` by `900-surface` and by every call site in
     * `src/sections/sysctl.c`.
     *
     * It reached `platform.h` without reaching here, and the mined corpus carried the
     * association instead - so the build worked while `corpus.h` still listed the name,
     * and broke the moment that header was regenerated and dropped it as a duplicate. A
     * fact held only in a generated file is held nowhere. (D243) */
    {"libkernel", "sysctlbyname"},
    {"libkernel", "statfs"},

    /* Enumeration. See src/sections/modules.c. */
    {"libkernel", "sceKernelGetModuleList"},
    {"libkernel", "sceKernelGetModuleInfo"},

    /* The display path. See src/display.c. */
    {"libSceVideoOut", "sceVideoOutSetBufferAttribute"},
    {"libSceVideoOut", "sceVideoOutRegisterBuffers"},
    /* The current generation's forms. See platform.h: the two generations expose
     * different entry points and display.c takes whichever pair resolved. */
    {"libSceVideoOut", "sceVideoOutRegisterBuffers2"},
    {"libSceVideoOut", "sceVideoOutSetBufferAttribute2"},
    {"libSceVideoOut", "sceVideoOutSubmitFlip"},
    {"libSceVideoOut", "sceVideoOutAddFlipEvent"},
    {"libSceVideoOut", "sceVideoOutIsFlipPending"},
    {"libSceVideoOut", "sceVideoOutSetFlipRate"},

    /* Liverpool flip event queue & synchronization */
    {"libkernel", "sceKernelCreateEqueue"},
    {"libkernel", "sceKernelDeleteEqueue"},
    {"libkernel", "sceKernelWaitEqueue"},

    /* Controller orientation reset */
    {"libScePad", "scePadResetOrientation"},

    /* Gnm driver functions used by oops-sdk gnm_display on Orbis */
    {"libSceGnmDriver", "sceGnmDispatchDirect"},
    {"libSceGnmDriver", "sceGnmDispatchInitDefaultHardwareState"},
    {"libSceGnmDriver", "sceGnmDrawIndex"},
    {"libSceGnmDriver", "sceGnmSubmitCommandBuffers"},
    {"libSceGnmDriver", "sceGnmSubmitDone"},

    /* Ends the run. See src/start.c. */
    {"libSceLibcInternal", "exit"},

    /* libSceAgc pre-encoded imports */
    {"libSceAgc", "$aJf+j5yntiU"},
    {"libSceAgc", "$tSBxhAPyytQ"},
    {"libSceAgc", "$t7PlZ9nt5Lc"},
    {"libSceAgc", "$2ccJz9LQI+w"},
    {"libSceAgc", "$mljzuGDZRQ4"},
    {"libSceAgc", "$aP1Ki9G3++4"},
    {"libSceAgc", "$xSAR0LTcRKM"},
    {"libSceAgc", "$VEGu4dixjUg"},
    {"libSceAgc", "$+u6dKSLWM2o"},
    {"libSceAgc", "$-RnpfpxIhec"},
    {"libSceAgc", "$-vnlTPPXPrw"},
    {"libSceAgc", "$57labkp+rSQ"},
    {"libSceAgc", "$KT-hTp-Ch14"},
    {"libSceAgc", "$M0ttm8h7SKA"},
    {"libSceAgc", "$QIXCsbipds0"},
    {"libSceAgc", "$b-oySn+G2tE"},
    {"libSceAgc", "$e1DFTg+Sd8U"},
    {"libSceAgc", "$ewobAQeMo5k"},
    {"libSceAgc", "$hL7C0IRpWZI"},
    {"libSceAgc", "$mStuvI0zOtc"},
    {"libSceAgc", "$r98I08t+LOg"},
    {"libSceAgc", "$rUuVjyR+Rd4"},
    {"libSceAgc", "$t1vNu082-jM"},
    {"libSceAgc", "$u2T2DiA5hRI"},
    {"libSceAgc", "$uZW-mqsxkrM"},
    {"libSceAgc", "$vuSXe69VILM"},
    {"libSceAgc", "$w1KFAHVqpaU"},
    {"libSceAgc", "$ypVBz4uPKcQ"},
    {"libSceAgc", "$zfcxg-ewMK8"},

    /* REQ-20260913T2355Z-a6aa: Remaining 24 command builders and GetSize siblings */
    {"libSceAgc", "$j3EtxFkSIhQ"},
    {"libSceAgc", "$PxKWV2fVAps"},
    {"libSceAgc", "$cFazmnXpJOE"},
    {"libSceAgc", "$Y-5vneiBtzk"},
    {"libSceAgc", "$6mFxkVqdmbQ"},
    {"libSceAgc", "$cpCILPya5Zk"},
    {"libSceAgc", "$JrtiDtKeS38"},
    {"libSceAgc", "$htn36gPnBk4"},
    {"libSceAgc", "$eZ4+17OQz4Q"},
    {"libSceAgc", "$k3GhuSNmBLU"},
    {"libSceAgc", "$Abendgtz+3o"},
    {"libSceAgc", "$UZbQjYAwwXM"},
    {"libSceAgc", "$yUBESvCCJ4I"},
    {"libSceAgc", "$BIPexNBSGog"},
    {"libSceAgc", "$ou16V5hh5sg"},
    {"libSceAgc", "$CtB+A9-VxO0"},
    {"libSceAgc", "$w8HVkEeXPv8"},
    {"libSceAgc", "$B+aG9DUnTKA"},
    {"libSceAgc", "$qMlfB1ZhMDc"},
    {"libSceAgc", "$1q1titRBL6o"},
    {"libSceAgc", "$cxPZ4Wgvdj8"},
    {"libSceAgc", "$H7uZqCoNuWk"},
    {"libSceAgc", "$+kSrjIVxKFE"},
    {"libSceAgc", "$RmaJwLtc8rY"},
    {"libSceAgc", "$BVFg3CWU6Eo"},
    {"libSceAgc", "$ZvwO9euwYzc"},
    {"libSceAgc", "$GBCh3zCihoU"},
    {"libSceAgc", "$YUeqkyT7mEQ"},
    {"libSceAgc", "$bbFueFP+J4k"},
    {"libSceAgc", "$-HOOCn0JY48"},
    {"libSceAgc", "$nNlUtdDDvZ0"},
    {"libSceAgc", "$hvUfkUIQcOE"},
    {"libSceAgc", "$UQGTw4xRlcM"},
    {"libSceAgc", "$MWiElSNE8j8"},
    {"libSceAgc", "$i1jyy49AjXU"},
    {"libSceAgc", "$p9tI+yTvx68"},

    /* REQ-20260914T1730Z-4386 & REQ-20260915T1600Z-a70f: Patch */
    {"libSceAgc", "$d-6uF9sZDIU"},
    {"libSceAgc", "$vcmNN+AAXnY"},
    {"libSceAgc", "$z2duB-hHQSM"},
    {"libSceAgc", "$Qrj4c+61z4A"},
    {"libSceAgc", "$vRoArM9zaIk"},
    {"libSceAgc", "$6lNcCp+fxi4"},
    {"libSceAgc", "$IxYiarKlXxM"},
    {"libSceAgc", "$cdDRpqcFGbU"},
    {"libSceAgc", "$3KDcnM3lrcU"},
    {"libSceAgc", "$0fWWK5uG9rQ"},

    /* REQ-20260915T1600Z-a70f: Builders */
    {"libSceAgc", "$LtTouSCZjHM"},
    {"libSceAgc", "$wr23dPKyWc0"},
    {"libSceAgc", "$WmAc2MEj6Io"},
    {"libSceAgc", "$VmW0Tdpy420"},
    {"libSceAgc", "$TRO721eVt4g"},

    /* REQ-20260914T1558Z-7d41: Device Info */
    {"libSceAgc", "$Tasc5SLczww"},
    {"libSceAgcDriver", "$ZGMnhAlcv9Y"},
    {"libSceGnmDriver", "$GKIlegek0JQ"},
    {"libSceGnmDriver", "$Fwvh++m9IQI"},

    /* ---- declared inside a section file rather than in platform.h ---------------
     *
     * Two names that exist to be asked about rather than called, so they are declared
     * where they are used. They still have to be imported from somewhere. */

    /* The census control: a name chosen because nothing defines it. Imported from a
     * library that does exist, so the lookup genuinely happens and genuinely fails -
     * which is the whole point of the control. Pointing it at a made-up library would
     * prove nothing about the presence test. */
    {"libSceLibcInternal", "obs_census_control_absent"},

    /* The generation probe: present on the newer console, absent on the older one.
     *
     * The library name is the weakest assumption in this file. If it is wrong the
     * probe reports absent everywhere, which reads as "older hardware" rather than as
     * a mistake - so treat a universal absent result as a reason to check this line
     * before believing it. */
    {"libSceUserService", "sceUserServiceGetLoginUserIdList"},
};

/* Calls `fn` once per import declared outside the census. */
void obs_platform_each_symbol(void (*fn)(const char *library, const char *symbol)) {
    const size_t count = sizeof(obs_platform_imports) / sizeof(obs_platform_imports[0]);
    for (size_t i = 0; i < count; i++) {
        fn(obs_platform_imports[i].library, obs_platform_imports[i].symbol);
    }
}
