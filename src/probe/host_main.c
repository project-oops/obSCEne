/*
 * The host-build entry point.
 *
 * Exists so the harness can be run on an ordinary machine against the stubs in
 * host_stubs.c. The expected outcome is a full sheet of red in the right order with
 * the dependency skips in the right places - if that is not what appears, the fault
 * is in this program rather than in anything it measures.
 */

#include <stdio.h>

#include <string.h>

#include "obscene/harness.h"
#include "obscene/net.h"
#include "obscene/platform.h"
#include "obscene/sections.h"

/*
 * Print which library each imported symbol comes from, one per line.
 *
 * The module build needs this association and cannot get it from the linked ELF: a
 * `.dynsym` entry records that a symbol is undefined, not which library resolves it.
 * The check tables already carry it, and this is the only place they exist in a form
 * something can read - so the module build asks the host build rather than a second
 * copy of the mapping being kept in step by hand.
 *
 * That makes the host build a prerequisite of the module build. It already was, in
 * principle (D001); now it is in the Makefile too.
 */
static void obs_print_one(const char *library, const char *symbol) {
    printf("%s %s\n", library, symbol);
}

static void obs_print_symbol_manifest(void) {
    /* Two sources, because there are two ways an import gets declared: the census
     * macros in surface.h, and everything else. Between them they must cover every
     * undefined symbol in the link - `mkmodule` fails the build if they do not. */
    obs_surface_each_symbol(obs_print_one);
    obs_platform_each_symbol(obs_print_one);
}

/*
 * The manifest without the census, for a build that does not link it.
 *
 * The census is 339 of the 352 libraries and none of them is called by name - it takes
 * each name's address to ask whether it resolved, which imports the name and requires
 * its library. An eboot is built with `OBS_CENSUS_LINKED=0` for that reason (D227), so
 * its undefined symbols are only the ones the checks actually call, and this is the
 * list that covers them.
 *
 * Two lists rather than one filtered afterwards: `mkmodule` fails on a symbol the
 * manifest does not claim, so the manifest and the link have to be produced from the
 * same statement about what is compiled in. A filter would be a third place for that to
 * be decided.
 */
static void obs_print_platform_manifest(void) {
    obs_platform_each_symbol(obs_print_one);
}

int main(int argc, char **argv) {
    if (argc > 1 && strcmp(argv[1], "--symbols") == 0) {
        obs_print_symbol_manifest();
        return 0;
    }

    if (argc > 1 && strcmp(argv[1], "--symbols-no-census") == 0) {
        obs_print_platform_manifest();
        return 0;
    }

    /* Serve the command protocol instead of running the suite once.
     *
     * The same code path a console will run, on a machine that has neither console nor
     * handheld attached. That is the whole reason the protocol is specified rather than
     * improvised: `docs/PROTOCOL.md` plus the captured exchanges are enough to build
     * and test a driver here, months before there is any hardware to point it at.
     *
     * A port may follow, so a second probe can be run alongside without a collision. */
    if (argc > 1 && strcmp(argv[1], "--serve") == 0) {
        unsigned short port = OBS_NET_PORT;
        if (argc > 2) {
            /* Parsed here rather than with `atoi`.
             *
             * `platform.h` declares `atoi` weak, because it is one of the platform
             * functions this program measures. Pulling in the host's definition as well
             * makes the two declarations disagree, and the build refuses it. The probe
             * measuring a function is a good reason not to also depend on it. */
            long value = 0;
            for (const char *p = argv[2]; *p != '\0'; p++) {
                if (*p < '0' || *p > '9' || value > 65535) {
                    fprintf(stderr, "--serve takes a port between 1 and 65535\n");
                    return 2;
                }
                value = value * 10 + (*p - '0');
            }
            if (value <= 0 || value > 65535) {
                fprintf(stderr, "--serve takes a port between 1 and 65535\n");
                return 2;
            }
            port = (unsigned short)value;
        }
        /* Generated before the socket opens, so there is never a window where the probe
         * is listening without one. A failure is reported and serving continues: the
         * alternative is refusing to run at all on a platform with no entropy, which
         * would take away a working instrument to enforce a control that platform
         * cannot have. */
        if (obs_net_secret_generate() < 0) {
            fprintf(stderr,
                    "could not generate a session secret: serving unauthenticated\n");
        } else {
            fprintf(stderr, "session secret: %s\n", obs_net_secret_text());
        }
        if (obs_net_serve(port) < 0) {
            fprintf(stderr, "could not listen on port %u\n", (unsigned)port);
            return 1;
        }
        return 0;
    }

    obs_tally tally = obs_run_all();
    /* Non-zero on any failure, so this is usable as a CI gate against a real target
     * later. Partial results do not fail the run: amber means "worth looking at",
     * and a build that goes red on it would train everyone to ignore it. */
    return tally.fail > 0 ? 1 : 0;
}
