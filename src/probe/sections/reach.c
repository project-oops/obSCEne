/* ---- 047-reach -------------------------------------------------------------
 *
 * Am I in the sandbox, or out of it? Measured, not assumed.
 *
 * # What this establishes
 *
 * obSCEne runs in one of two very different filesystem contexts, and every other section that
 * touches a path reads differently depending on which. As a sandboxed application it sees its
 * own `/app0` mount and its writable `/data`, and little else. As a payload with the exploit's
 * privileges it sees the wider system. Whether a later probe's file access *should* have worked
 * depends entirely on which of those is true, and until now nothing said which.
 *
 * So this asks the question directly: it attempts to open a small set of paths that span the
 * boundary, and reports for each whether the open succeeded. A path inside the app's own jail
 * that opens, and a path in the system namespace that does not, is a process that is sandboxed.
 * The system path opening too is a process that has escaped.
 *
 * # Behaviour, not contents
 *
 * This reads nothing. It opens a path, learns whether the namespace exposes it, and closes it
 * again. The result is the open's own verdict - reachable or not - which is a measurement of
 * *this process's view*, not of anybody's file. Nothing is copied off, nothing is interpreted;
 * the bytes behind these paths never enter the report. That is the line this section is on: it
 * measures what a call does, the way the whole probe does. (selfish#D086)
 */

#include "obscene/harness.h"
#include "obscene/platform.h"
#include "obscene/report.h"
#include "obscene/sections.h"
#include "obscene/status.h"

/* One path to test, and which side of the sandbox boundary it sits on. */
typedef struct {
    const char *path;
    /* 1 if reaching it means the process is outside its own jail, 0 if it is the jail itself. */
    int outside;
    const char *note;
} obs_reach_probe;

/* The paths, chosen to straddle the boundary with as few as tell it apart.
 *
 * The two `/app0` and `/data` entries are the jail a normal application always has. The
 * `/system` entries are outside it - a sandboxed module cannot open them, a payload can.
 * Device nodes `/dev/agc0` and `/dev/gnm` distinguish native PS5 mode from PS4 compat mode. */
static const obs_reach_probe obs_reach_probes[] = {
    {"/app0/eboot.bin", 0, "the application's own executable, inside its mount"},
    {"/system/vsh/app/NPXS40112/eboot.bin", 1, "a system application, outside the jail"},
    {"/system/common/lib/libc.prx", 1, "a system library, outside the jail"},
    {"/dev/agc0", 1, "native PS5 GPU driver interface (RDNA2)"},
    {"/dev/gnm", 0, "PS4 backward-compatibility GPU interface (GCN)"},
    {"/dev/dmem0", 1, "direct memory allocator device node"},
};

/* Open O_RDONLY and close at once. Returns 1 if the path was reachable, 0 if not. Reads nothing:
 * the open's success or failure is the entire result. */
static int obs_reach_open(const char *path) {
    int fd = sceKernelOpen(path, OBS_O_RDONLY, 0);
    if (fd < 0) {
        return 0;
    }
    sceKernelClose(fd);
    return 1;
}

static obs_result check_reachability(void) {
    OBS_REQUIRE(&sceKernelOpen);
    OBS_REQUIRE(&sceKernelClose);

    int outside_reachable = 0;
    int jail_reachable = 0;

    for (unsigned i = 0; i < OBS_COUNT(obs_reach_probes); i++) {
        const obs_reach_probe *p = &obs_reach_probes[i];
        int reachable = obs_reach_open(p->path);
        /* One record per path: what it is, and whether this process could reach it. The path is
         * a fixed string in this program, not content read off the box. */
        obs_report_sysinfo(p->path, reachable ? "reachable" : "blocked", p->note);
        if (reachable) {
            if (p->outside) {
                outside_reachable = 1;
            } else {
                jail_reachable = 1;
            }
        }
    }

    /* The headline verdict, so a reader does not have to reason over the rows. */
    if (outside_reachable) {
        obs_report_sysinfo("reach/verdict", "escaped",
                           "the system namespace is reachable - not confined to the app jail");
        return obs_pass_value(1);
    }
    if (jail_reachable) {
        obs_report_sysinfo("reach/verdict", "sandboxed",
                           "only the application's own mount is reachable");
        return obs_pass_value(0);
    }
    /* Neither reachable is the host build, or a context with no filesystem at all - not a
     * failure of this code, so it reports what it saw and moves on. */
    obs_report_sysinfo("reach/verdict", "none", "no test path was reachable from this process");
    return obs_partial("no test path was reachable");
}

static obs_result check_mode_verdict(void) {
    OBS_REQUIRE(&sceKernelOpen);
    OBS_REQUIRE(&sceKernelClose);

    int agc_reachable = obs_reach_open("/dev/agc0");
    int gnm_reachable = obs_reach_open("/dev/gnm");
    int dmem_reachable = obs_reach_open("/dev/dmem0");

    obs_report_sysinfo("gpu/agc", agc_reachable ? "reachable" : "blocked",
                       "PS5 native GPU device path (/dev/agc0)");
    obs_report_sysinfo("gpu/gnm", gnm_reachable ? "reachable" : "blocked",
                       "PS4 compat GPU device path (/dev/gnm)");
    obs_report_sysinfo("gpu/dmem", dmem_reachable ? "reachable" : "blocked",
                       "direct memory device (/dev/dmem0)");

    if (agc_reachable) {
        obs_report_sysinfo("mode/verdict", "ps5_native",
                           "native Prospero environment with PS5 GPU (AGC) reach");
        return obs_pass_value(2);
    }
    if (gnm_reachable) {
        obs_report_sysinfo("mode/verdict", "ps4_compat",
                           "running in PS4 backward-compatibility container");
        return obs_pass_value(1);
    }

    obs_report_sysinfo("mode/verdict", "unknown", "no GPU interface reached from current namespace");
    return obs_partial("no GPU interface accessible");
}

static const obs_check reach_checks[] = {
    {"047-reach/where-am-i", "libkernel", "sceKernelOpen", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelOpen, check_reachability, OBS_FROM_DERIVED},
    {"047-reach/mode-verdict", "libkernel", "sceKernelOpen", OBS_CAP_NONE, OBS_CAP_NONE,
     (const void *)&sceKernelOpen, check_mode_verdict, OBS_FROM_DERIVED},
};

const obs_section obs_section_reach = {
    "047-reach",
    "Sandbox reach",
    "Whether this process is confined to its own mount or can see the system namespace, from "
    "the verdict of opening paths that straddle the boundary. Behaviour, not contents.",
    reach_checks,
    OBS_COUNT(reach_checks),
};
