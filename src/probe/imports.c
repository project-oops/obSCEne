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
    {"libkernel", "sceKernelGetDirectMemorySize"},
    {"libkernel", "sceKernelAllocateDirectMemory"},
    {"libkernel", "sceKernelAllocateMainDirectMemory"},
    {"libkernel", "sceKernelVirtualQuery"},
    {"libkernel", "sceKernelReleaseDirectMemory"},
    {"libkernel", "sceKernelMapDirectMemory"},
    {"libkernel", "sceKernelMunmap"},
    {"libkernel", "sceKernelUsleep"},
    {"libkernel", "sceKernelIsNeoMode"},
    {"libkernel", "scePthreadSelf"},
    {"libkernel", "scePthreadCreate"},
    {"libkernel", "scePthreadJoin"},
    {"libkernel", "sceKernelLoadStartModule"},
    {"libkernel", "sceKernelDlsym"},
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
    {"libSceVencCore", "sceVencCoreCreateEncoder"},
    {"libSceVencCore", "sceVencCoreGetAuData"},
    {"libSceVencCore", "sceVencCoreQueryMemorySize"},
    {"libSceAudioOut", "sceAudioOutInit"},
    {"libSceAudioOut", "sceAudioOutOpen"},
    {"libSceAudioOut", "sceAudioOutClose"},
    {"libScePad", "scePadInit"},
    {"libScePad", "scePadOpen"},
    {"libScePad", "scePadClose"},
    {"libScePad", "scePadReadState"},
    {"libSceKeyboard", "sceKeyboardInit"},
    {"libSceKeyboard", "sceKeyboardOpen"},
    {"libSceKeyboard", "sceKeyboardReadState"},
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
    {"libSceLibcInternal", "_Getpctype"},
    {"libSceLibcInternal", "_Getptolower"},
    {"libSceLibcInternal", "_Getptoupper"},

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

    /* The GPU command-builders (src/sections/gnm.c). Only the two whose arity two independent
     * reimplementations confirm; the rest of libSceGnmDriver stays in the census, uncalled. */
    {"libSceGnmDriver", "sceGnmDispatchInitDefaultHardwareState"},
    {"libSceGnmDriver", "sceGnmDispatchDirect"},
    /* Called by checks and, until now, declared only by the census.
     *
     * That worked because the census imports every name it lists, so the association existed
     * - in the wrong file. A build that does not link the census (D227) has these as undefined
     * symbols nothing claims, and `mkmodule` refuses it by name, which is how they were found:
     *
     *     error: 4 imported symbol(s) have no library ... sceGnmDrawIndex, sceGnmSubmitDone
     *
     * A symbol a check *calls* belongs here whether or not the census also lists it. The
     * libraries are the census's own, not a guess: `libSceGnmDriver` from the `graphics` group
     * and `libSceAgc` from `agc`. */
    {"libSceGnmDriver", "sceGnmDrawIndex"},
    {"libSceGnmDriver", "sceGnmSubmitCommandBuffers"},
    {"libSceGnmDriver", "sceGnmSubmitDone"},

    /* Address-probed by the HUD (src/sysinfo.c), never called - its struct layout is
     * unconfirmed. Listed so mkmodule knows the library the presence probe imports from. */
    {"libSceNetCtl", "sceNetCtlInit"},
    {"libSceNetCtl", "sceNetCtlGetInfo"},

    /* Calls that fill a buffer. See src/sections/layout.c. */
    {"libkernel", "sceKernelDirectMemoryQuery"},
    {"libkernel", "sceKernelGetSystemSwVersion"},
    {"libSceVideoOut", "sceVideoOutGetResolutionStatus"},
    {"libSceVideoOut", "sceVideoOutGetFlipStatus"},

    /* Escaping the sandbox to write the report where ftp can read it (mkdir a path outside
     * the jail; a timestamp for it). Added for the second thread's disk-escape work; libkernel
     * by the sceKernel* prefix, unambiguous. */
    {"libkernel", "sceKernelMkdir"},
    {"libkernel", "sceKernelGettimeofday"},

    /* The measuring instrument. See src/sections/measure.c. */
    {"libkernel", "sceKernelReadTsc"},
    {"libkernel", "sceKernelGetProcessTimeCounterFrequency"},

    /* The same platform under its POSIX names. See src/sections/posix.c. */
    {"libScePosix", "posix_pthread_rwlock_init"},
    {"libScePosix", "posix_pthread_rwlock_destroy"},
    {"libScePosix", "posix_pthread_rwlock_tryrdlock"},
    {"libScePosix", "posix_pthread_rwlock_trywrlock"},
    {"libScePosix", "posix_pthread_rwlock_unlock"},
    {"libScePosix", "posix_sigemptyset"},
    {"libScePosix", "posix_sigfillset"},
    {"libScePosix", "posix_sigaddset"},
    {"libScePosix", "posix_sigdelset"},
    {"libScePosix", "posix_sigismember"},
    {"libScePosix", "posix_getpagesize"},
    {"libScePosix", "posix_usleep"},

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
    /* Named `libkernel` by `900-surface` and by every call site in `src/sections/sysctl.c`.
     *
     * It reached `platform.h` without reaching here, and the mined corpus carried the
     * association instead - so the build worked while `corpus.h` still listed the name, and
     * broke the moment that header was regenerated and dropped it as a duplicate. A fact held
     * only in a generated file is held nowhere. (D243) */
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

    /* Ends the run. See src/start.c. */
    {"libSceLibcInternal", "exit"},

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
};

/* Calls `fn` once per import declared outside the census. */
void obs_platform_each_symbol(void (*fn)(const char *library, const char *symbol)) {
    const size_t count = sizeof(obs_platform_imports) / sizeof(obs_platform_imports[0]);
    for (size_t i = 0; i < count; i++) {
        fn(obs_platform_imports[i].library, obs_platform_imports[i].symbol);
    }
}
