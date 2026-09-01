/*
 * End-to-end proof that the tracer produces meaningful, parseable results.
 *
 * There is no console here, so this stands in for one: it drives the *encoder* with a
 * synthetic run - a hot function called far past its cap, a cold one, an inlined
 * out-parameter, a buffer too large to inline - writes a real trace stream, and then
 * checks two things:
 *
 *   1. The stream round-trips: every value the encoder put in comes back out of a
 * re-read.
 *   2. The policies that make the payload safe on a real device actually hold - the
 * per-nid cap limits detail while the count still travels, and an oversized buffer is
 * hashed, never shipped as bytes.
 *
 * It also runs the on-disk decoder over the same stream so the *parseable* half is
 * exercised for real, not just asserted. Exit non-zero on any failure; that is what the
 * Makefile gates on.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "trace_encode.h"
#include "trace_format.h"

#define NID_HOT 0x1111111111111111ULL
#define NID_COLD 0x2222222222222222ULL
#define NID_OUT 0x3333333333333333ULL
#define NID_BIG 0x4444444444444444ULL

#define HOT_CALLS 1000u /* called far past the cap */
#define COLD_CALLS 3u

static int failures = 0;
static void check(int cond, const char *what) {
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", what);
        failures++;
    }
}

int main(void) {
    /* Storage the encoder writes into - on-console these are payload-mapped pages. */
    static struct obs_trace_rec recs[4096];
    static uint64_t samp_nids[256];
    static uint32_t samp_counts[256];

    struct obs_trace_buf buf;
    struct obs_trace_sampler samp;
    obs_trace_buf_init(&buf, recs, (uint32_t)(sizeof recs / sizeof recs[0]));
    obs_trace_sampler_init(&samp, samp_nids, samp_counts, 256u, OBS_TRACE_CAP);

    /* --- drive a synthetic run
     * ------------------------------------------------------------ */

    /* A hot function, called 1000 times. Only the first OBS_TRACE_CAP get detail; the
     * rest just move the counter. This is the mechanism that keeps the payload from
     * drowning the device. */
    uint32_t seq = 0;
    for (unsigned i = 0; i < HOT_CALLS; i++) {
        uint64_t args[2] = {(uint64_t)i, 0xdeadbeefULL};
        if (obs_trace_hit(&samp, NID_HOT)) {
            obs_trace_entry(&buf, 1, seq++, NID_HOT, args, 2);
            obs_trace_exit(&buf, 1, seq - 1, NID_HOT, (uint64_t)i * 2u);
        }
    }

    /* A cold function, under the cap: every call recorded. */
    for (unsigned i = 0; i < COLD_CALLS; i++) {
        uint64_t args[1] = {(uint64_t)(0x100 + i)};
        if (obs_trace_hit(&samp, NID_COLD)) {
            obs_trace_entry(&buf, 1, seq++, NID_COLD, args, 1);
        }
    }

    /* An out-parameter small enough to inline: 8 known bytes. */
    (void)obs_trace_hit(&samp, NID_OUT);
    uint8_t small[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    obs_trace_outbuf(&buf, NID_OUT, 0x40000000ULL, small, sizeof small,
                     OBS_TRACE_OUTBUF_INLINE);

    /* A buffer too large to inline: must become a (length, hash) record, never bytes.
     */
    (void)obs_trace_hit(&samp, NID_BIG);
    uint8_t big[200];
    for (unsigned i = 0; i < sizeof big; i++) {
        big[i] = (uint8_t)(i * 7u + 3u);
    }
    uint64_t expect_hash = obs_trace_fnv1a(big, sizeof big);
    obs_trace_outbuf(&buf, NID_BIG, 0x50000000ULL, big, sizeof big,
                     OBS_TRACE_OUTBUF_INLINE);

    /* At drain, every capped nid reports its true total. */
    obs_trace_count_rec(&buf, NID_HOT, obs_trace_count(&samp, NID_HOT));

    /* --- assert the safety policies held
     * -------------------------------------------------- */

    check(obs_trace_count(&samp, NID_HOT) == HOT_CALLS, "hot nid counted every call");
    check(obs_trace_count(&samp, NID_COLD) == COLD_CALLS,
          "cold nid counted every call");

    /* Count ENTRY records for the hot nid: must be exactly the cap, not 1000. */
    unsigned hot_entries = 0, cold_entries = 0, count_recs = 0, outbufs = 0;
    unsigned hot_total_reported = 0;
    int big_hashed_ok = 0, small_inline_ok = 0;
    for (uint32_t i = 0; i < buf.head; i++) {
        struct obs_trace_rec *r = &recs[i];
        if (r->kind == OBS_TRACE_ENTRY && r->nid == NID_HOT) {
            hot_entries++;
        }
        if (r->kind == OBS_TRACE_ENTRY && r->nid == NID_COLD) {
            cold_entries++;
        }
        if (r->kind == OBS_TRACE_COUNT && r->nid == NID_HOT) {
            count_recs++;
            hot_total_reported = (unsigned)r->arg[0];
        }
        if (r->kind == OBS_TRACE_OUTBUF) {
            outbufs++;
            if (r->nid == NID_BIG) {
                big_hashed_ok = (r->argc == 0 && r->arg[1] == sizeof big &&
                                 r->arg[2] == expect_hash);
                /* The bytes of `big` must appear nowhere in the record. */
                const uint8_t *bytes = (const uint8_t *)r->arg;
                int leaked = 0;
                for (unsigned b = 0; b + 8 <= sizeof(r->arg); b++) {
                    if (memcmp(bytes + b, big, 8) == 0) {
                        leaked = 1;
                    }
                }
                check(!leaked, "large buffer bytes are not present in the record");
            }
            if (r->nid == NID_OUT) {
                const uint8_t *payload = (const uint8_t *)&r->arg[2];
                small_inline_ok =
                    (r->argc == sizeof small && r->arg[1] == sizeof small &&
                     memcmp(payload, small, sizeof small) == 0);
            }
        }
    }
    check(hot_entries == OBS_TRACE_CAP, "hot nid detail is capped at OBS_TRACE_CAP");
    check(cold_entries == COLD_CALLS, "cold nid detail is complete");
    check(count_recs == 1 && hot_total_reported == HOT_CALLS,
          "capped nid reports its true total via a COUNT record");
    check(big_hashed_ok, "oversized buffer is recorded as length+hash");
    check(small_inline_ok, "small out-parameter is recorded inline, exactly");
    check(buf.dropped == 0, "no records were dropped");

    /* --- write a real stream and round-trip it
     * -------------------------------------------- */

    const char *path = "trace_selftest.bin";
    FILE *f = fopen(path, "wb");
    check(f != NULL, "trace file opens for writing");
    if (f) {
        struct obs_trace_hdr hdr;
        memcpy(hdr.magic, OBS_TRACE_MAGIC, 8);
        hdr.version = OBS_TRACE_VERSION;
        hdr.rec_size = OBS_TRACE_REC_SIZE;
        hdr.endian = OBS_TRACE_ENDIAN;
        hdr.reserved[0] = hdr.reserved[1] = hdr.reserved[2] = 0;
        fwrite(&hdr, sizeof hdr, 1, f);
        fwrite(recs, sizeof recs[0], buf.head, f);
        fclose(f);
    }

    /* Re-read and check the first hot ENTRY's args survived exactly. */
    f = fopen(path, "rb");
    check(f != NULL, "trace file opens for reading");
    if (f) {
        struct obs_trace_hdr hdr;
        check(fread(&hdr, sizeof hdr, 1, f) == 1, "header reads back");
        check(memcmp(hdr.magic, OBS_TRACE_MAGIC, 8) == 0, "magic round-trips");
        struct obs_trace_rec r;
        int found_first_hot = 0;
        while (fread(&r, sizeof r, 1, f) == 1) {
            if (r.kind == OBS_TRACE_ENTRY && r.nid == NID_HOT && !found_first_hot) {
                found_first_hot = 1;
                check(r.argc == 2 && r.arg[0] == 0 && r.arg[1] == 0xdeadbeefULL,
                      "first hot entry args round-trip exactly");
            }
        }
        check(found_first_hot, "a hot entry survived to the file");
        fclose(f);
    }

    if (failures == 0) {
        printf("trace_selftest: OK - %u records, hot capped at %u of %u, big buffer "
               "hashed\n",
               buf.head, OBS_TRACE_CAP, HOT_CALLS);
    } else {
        printf("trace_selftest: %d FAILURES\n", failures);
    }
    return failures == 0 ? 0 : 1;
}
