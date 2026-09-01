/*
 * The target-platform entry point.
 *
 * Named rather than `_start`, and pointed at by the linker with `-e obscene_start`,
 * so the entry recorded in the container header is this function and nothing has to
 * infer it.
 *
 * # Returning from here is not correct, and it looked like a crash in the last check
 *
 * This used to return, on the reasoning that a loader calls the entry as an ordinary
 * function and regains control afterwards - so a probe should hand control back rather
 * than terminate, and stay runnable twice in one session.
 *
 * That is not what happens. The module is an executable and its entry is where the
 * process *starts*; nothing is waiting for a return. Returning pops whatever the
 * loader happened to leave on the stack and jumps to it:
 *
 *     Unhandled Exception code 0xc0000005 at 0x1
 *
 * The expensive part is where that lands. It arrives after a complete and successful
 * run, immediately after the last check's platform call, so it reads as that check
 * crashing the process - and the last check is a different thing to go looking at than
 * the exit path. It cost a while to find that every section had already run.
 *
 * # What it does instead
 *
 * Ends the process, and spins if it cannot. Both are honest and they are
 * distinguishable, which is the point:
 *
 * - **Exits.** The report is complete and the run ended on purpose.
 * - **Hangs.** The platform has no `exit` this could resolve. The report is still
 *   complete - everything was written before this point - and the hang says something
 *   about the platform rather than about the probe.
 *
 * What it must not do is return. That is the one outcome indistinguishable from
 * having crashed.
 */

#include "obscene/harness.h"
#include "obscene/net.h"
#include "obscene/report.h"
#include "obscene/runtime.h"
#include "obscene/screen.h"
#include "obscene/sections.h"
#include "obscene/sink.h"
#include "obscene/sysinfo.h"
#include "obscene/platform.h"

/* Kept in a global so a debugger, or a loader inspecting the image after the run, can
 * read the outcome without parsing the report stream. Written before the process ends,
 * so it is there either way. */
obs_tally obscene_last_tally;

/* A boot breadcrumb on the kernel log - Principle 1 applied to the boot sequence itself.
 *
 * The eboot's report goes to a socket or a file, and neither survives the process faulting.
 * Launched as a foreground app that is exactly what happened: the console mounted and started
 * it, it took over the display, and it took the whole system down with nothing on record to
 * say how far it got. `sceKernelDebugOutText` writes to the system log that a homebrew klog
 * reader already tails, so a breadcrumb here is legible after the fault even when the report
 * channel never opened - the one place a `try` with no `res` can still be read.
 *
 * The signature is the one already tried in `src/min.c`: the least confident declaration here,
 * proven far enough to trust for a breadcrumb. Guarded like every platform call - a loader
 * without it resolves the weak symbol to null and the breadcrumb is simply skipped, never a
 * jump to zero. */
OBS_WEAK int sceKernelDebugOutText(int channel, const char *text);

/* Where a boot note goes on disk, in the order `sink.c` tries its own candidates and for the
 * same reason: which directory is writable depends on the platform and on how the title was
 * launched, so the list is guesses and the one that works is the one used.
 *
 * A **separate file from the report**, deliberately. `obs_sink_open` opens `O_TRUNC`, so a
 * driver later asking for a full run would wipe exactly the record of the boot that preceded
 * it. */
static const char *const obs_boot_paths[] = {
    "/mnt/usb0/obscene/boot.txt",
    "/mnt/usb0/obscene-boot.txt",
    "/mnt/usb1/obscene/boot.txt",
    "/mnt/usb1/obscene-boot.txt",
    "/data/obscene/boot.txt",
    "/data/obscene-boot.txt",
    "/download0/obscene-boot.txt",
    "obscene-boot.txt",
};

/* The open boot log, or negative. `obs_boot_tried` stops a platform that cannot open any of
 * them from attempting the whole list again on every note. */
static int obs_boot_fd = -1;
static int obs_boot_tried;

/* A boot breadcrumb, to the kernel log **and** to disk.
 *
 * Principle 1 applied to the boot sequence itself, and the disk half is the half that survives.
 *
 * The report channel cannot do this job here: `obs_sink_open` is called inside `obs_run_all`,
 * so a build that serves on start - which is now the one that goes near a console - never opens
 * it at all, and a fault before any driver connects would leave nothing anywhere. The kernel log
 * is read over a socket by a payload that dies with the machine, so it survives a *process*
 * fault and not a system one. Three consoles went down with the log lost that way.
 *
 * So each note is opened-if-needed and written immediately: a syscall per note, no buffering,
 * on the same reasoning `sink.c` states for the report - a buffer discards precisely the records
 * that matter, because the crash is what discarded it. Every call is guarded, because a platform
 * without these resolves them to null and jumping to zero would end the run this exists to
 * explain. */
static void obs_boot_note(const char *text) {
    if (text == NULL) return;
    size_t len = obs_strlen(text);
    if (len == 0) return;

    unsigned long base = obs_libkernel_base();
    if (base != 0) {
        (void)obs_invoke_syscall(601, 7, (long)text, 0, 0, 0, 0);
    }

    if (obs_boot_fd < 0 && !obs_boot_tried) {
        obs_boot_tried = 1;
        (void)obs_sink_backend_mkdir("/mnt/usb0/obscene");
        (void)obs_sink_backend_mkdir("/mnt/usb1/obscene");
        (void)obs_sink_backend_mkdir("/data/obscene");
        for (unsigned int i = 0; i < OBS_COUNT(obs_boot_paths); i++) {
            int fd = obs_sink_backend_open(obs_boot_paths[i]);
            if (fd >= 0) {
                obs_boot_fd = fd;
                break;
            }
        }
    }
    if (obs_boot_fd >= 0) {
        (void)obs_sink_backend_write(obs_boot_fd, text, len);
    }
}

void obscene_start(void);

void obscene_start(void) {
    /* Captured first, while rdi still holds it - a payload is handed payload_args there and it is
     * gone the moment anything else runs. Stashed for the kernel-probe section; harmless on a
     * build that is not a payload, where rdi holds whatever the loader left. */
    unsigned long obs_pargs_at_entry;
    __asm__ volatile("mov %%rdi, %0" : "=r"(obs_pargs_at_entry));
    obs_capture_payload_args(obs_pargs_at_entry);
    /* Bootstrap the output channel from getpid before anything tries to write. Guarded: read
     * word 0 only when payload_args is a plausible, aligned pointer, so a non-payload entry - a
     * title launched by the shell, say - never dereferences a value that is not a struct pointer.
     * The bootstrap itself refuses anything not shaped like a libkernel export (D209). */
    if (obs_pargs_at_entry >= 0x10000UL && obs_pargs_at_entry < 0x0000800000000000UL
        && (obs_pargs_at_entry & 0x7UL) == 0) {
        obs_bootstrap_payload_output(((unsigned long *)obs_pargs_at_entry)[0]);
    }
    /* Dynamically bind weak import symbols across loaded modules in the process */
    obs_bind_dynamic_symbols();
    /* The first thing, before any platform call that could fault: proof the container
     * mounted, the loader transferred control, and the crt reached here. On a foreground-app
     * launch this is the difference between "the package is wrong" and "a check took the
     * system down", and those are looked at in entirely different places. */
    obs_boot_note("obscene: eboot entry reached\n");
#if defined(OBS_SERVE_ON_START)
    obs_boot_note("obscene: checking net backend\n");
    /* Listen first, and run nothing crash-prone before the socket is up.
     *
     * The suite used to run here *before* serving, so the socket's availability was hostage
     * to 146 checks completing - and on an emulator whose stability is not deterministic
     * (shadPS4 has died mid-suite on one launch and completed the next, the same binary
     * both times) that made the interactive endpoint intermittently unreachable for a
     * reason that had nothing to do with the socket. The static report is a different
     * build's job; an interactive build listens immediately.
     *
     * The suite now runs on demand, when a driver sends `report`. A crash during it costs
     * one session and a reconnect - the reboot-and-reconnect shape the protocol was built
     * around - rather than the whole endpoint. The screen is skipped too, which also keeps
     * the run clear of whatever the intermittent mid-suite fault actually is.
     *
     * Looped so a driver can reconnect after a faulting command. A listen that cannot even
     * open (no libSceNet on this loader) returns negative, and the loop stops rather than
     * spinning on an endpoint that will never exist. */
    if (obs_net_backend_available()) {
        obs_boot_note("obscene: net backend up; preparing to serve\n");
        /* Put the port on screen for whoever has to point a driver at it - the "read the
         * address off the screen" the protocol assumes. Drawn once (nothing changes while
         * it waits on accept), and *after* the report record below would go out, so the
         * port is legible on stdout even if opening the display faults. */
        /* Before the port goes on screen, so the HUD draws both together and there is
         * never a run that advertises a port without the secret needed to use it. */
        obs_boot_note("obscene: generating net secret\n");
        if (obs_net_secret_generate() < 0) {
            obs_boot_note("obscene: no secret; serving unauthenticated\n");
            obs_report_net("unauthenticated", 0);
        } else {
            obs_boot_note("obscene: secret generated\n");
            obs_sysinfo_set_secret(obs_net_secret_text());
        }
        obs_sysinfo_set_listening(OBS_NET_PORT);
        obs_report_net("listening", OBS_NET_PORT);
        obs_boot_note("obscene: reading sysinfo\n");
        /* The same readout the HUD is about to draw, into the report too, so a driver that
         * connects before requesting a full run can still read the machine's own account of
         * itself - the port it found on screen, and the memory, VRAM and generation beside
         * it. The on-demand report emits these again (harness), with listening now set. */
        obs_sysinfo_report();
        obs_boot_note("obscene: sysinfo read ok; drawing hud (first display call)\n");
        obs_screen_hud();
        obs_boot_note("obscene: hud drawn; entering serve loop (waiting for a driver)\n");
        for (;;) {
            if (obs_net_serve(OBS_NET_PORT) < 0) {
                obs_report_net("unavailable", OBS_NET_PORT);
                break;
            }
        }
    }
    obs_boot_note("obscene: serve path ended (net backend down or serve returned)\n");
#else
    obs_boot_note("obscene: running full suite\n");
    obscene_last_tally = obs_run_all();
    obs_boot_note("obscene: suite complete\n");

    /* With a display, stay and show it. The report is already complete and on record by
     * this point, so nothing is being held back - what follows is for whoever is looking
     * at the screen, cycling the summary and the detail pages.
     *
     * Without a display this returns at once, and the exit below runs as before. That
     * keeps a headless run finishing, which is what anything automated needs. */
    obs_screen_present();
#endif

    /* Non-zero on any failure, matching the host build, so this is usable as a gate.
     * Partial results do not fail the run: amber means "worth looking at", and a
     * build that went red on it would train everyone to ignore it. */
    obs_boot_note("obscene: reached exit path\n");
    if (obs_address_is_callable((const void *)&exit)) {
        obs_boot_note("obscene: calling exit\n");
        exit(obscene_last_tally.fail > 0 ? 1 : 0);
    }
    obs_boot_note("obscene: returning to host thread\n");
    return;
}
