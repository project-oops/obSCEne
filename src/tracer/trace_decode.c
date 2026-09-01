/*
 * The decoder - off-console, turns a raw trace into the OBS| records the corpus already
 * reads.
 *
 * This is the half that makes results "meaningful and parseable". The payload emits a
 * stream of fixed 64-byte binary records; this reads them and prints one OBS| line per
 * fact, in the same pipe-delimited shape as the probe's report (docs/OUTPUT.md). That
 * is deliberate: a decoded trace and a probe report then land in one corpus and the
 * same diff/mine tooling works on both.
 *
 * Names: a trace carries NID hashes, not names - resolving them is a join against the
 * corpus, done later. This decoder prints the nid as hex, and if the stream carried a
 * NAME hint it uses it. It never invents a name it was not given.
 *
 * Usage: trace_decode < trace.bin        (reads a stream on stdin, writes OBS| lines to
 * stdout)
 *
 * It is an ordinary host program, so it uses stdio freely - none of the freestanding
 * constraints that bind the encoder apply here.
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "trace_format.h"

static void print_nid(uint64_t nid) {
    printf("%016llx", (unsigned long long)nid);
}

/* One decoded record -> one or more OBS| lines. Returns 0 on success. */
static void emit(const struct obs_trace_rec *r) {
    switch (r->kind) {
    case OBS_TRACE_ENTRY: {
        /* call: library is unknown here (resolved off-console), so it prints as '?'. */
        printf("OBS|call|?|");
        print_nid(r->nid);
        printf("|%u|%u", (unsigned)r->tid, (unsigned)r->seq);
        for (unsigned i = 0; i < r->argc && i < OBS_TRACE_ARGS; i++) {
            printf("|%llx", (unsigned long long)r->arg[i]);
        }
        printf("\n");
        break;
    }
    case OBS_TRACE_EXIT: {
        printf("OBS|ret|");
        print_nid(r->nid);
        printf("|%u|%llx\n", (unsigned)r->seq, (unsigned long long)r->arg[0]);
        break;
    }
    case OBS_TRACE_OUTBUF: {
        uint64_t addr = r->arg[0];
        uint64_t len = r->arg[1];
        int hashed = (r->argc == 0 && len != 0);
        if (hashed) {
            /* A buffer too large to inline: length and hash, never the bytes. */
            printf("OBS|outbuf|");
            print_nid(r->nid);
            printf("|%llx|%llu|hash=%016llx\n", (unsigned long long)addr,
                   (unsigned long long)len, (unsigned long long)r->arg[2]);
        } else {
            const uint8_t *payload = (const uint8_t *)&r->arg[2];
            printf("OBS|outbuf|");
            print_nid(r->nid);
            printf("|%llx|%llu|", (unsigned long long)addr, (unsigned long long)len);
            for (unsigned i = 0; i < r->argc && i < OBS_TRACE_OUTBUF_INLINE; i++) {
                printf("%02x", payload[i]);
            }
            printf("\n");
        }
        break;
    }
    case OBS_TRACE_NAME: {
        char name[OBS_TRACE_ARGS * 8u + 1u];
        memcpy(name, &r->arg[0], OBS_TRACE_ARGS * 8u);
        name[OBS_TRACE_ARGS * 8u] = '\0';
        printf("OBS|name|");
        print_nid(r->nid);
        printf("|%s\n", name);
        break;
    }
    case OBS_TRACE_COUNT: {
        printf("OBS|count|");
        print_nid(r->nid);
        printf("|%llu\n", (unsigned long long)r->arg[0]);
        break;
    }
    default:
        /* An unknown kind from a newer payload: report it rather than guess. The record
         * size is fixed, so one unknown kind does not desynchronise the ones after it.
         */
        fprintf(stderr, "trace_decode: unknown record kind %u, skipped\n",
                (unsigned)r->kind);
        break;
    }
}

int main(void) {
    struct obs_trace_hdr hdr;
    if (fread(&hdr, sizeof hdr, 1, stdin) != 1) {
        fprintf(stderr, "trace_decode: stream too short for a header\n");
        return 2;
    }
    if (memcmp(hdr.magic, OBS_TRACE_MAGIC, 8) != 0) {
        fprintf(stderr, "trace_decode: not a trace stream (bad magic)\n");
        return 2;
    }
    if (hdr.endian != OBS_TRACE_ENDIAN) {
        /* A wrong reading is worse than a refusal: rather than silently byte-swap every
         * field, refuse a stream written on a different-endian host. */
        fprintf(stderr, "trace_decode: stream endianness does not match this host\n");
        return 2;
    }
    if (hdr.version != OBS_TRACE_VERSION) {
        fprintf(stderr,
                "trace_decode: unsupported version %u (this decoder speaks %u)\n",
                (unsigned)hdr.version, OBS_TRACE_VERSION);
        return 2;
    }
    if (hdr.rec_size != OBS_TRACE_REC_SIZE) {
        fprintf(stderr, "trace_decode: record size %u, expected %u\n",
                (unsigned)hdr.rec_size, OBS_TRACE_REC_SIZE);
        return 2;
    }

    /* One line so a consumer knows the stream and version without parsing records. */
    printf("OBS|tracemeta|%u|%u\n", (unsigned)hdr.version, (unsigned)hdr.rec_size);

    struct obs_trace_rec r;
    unsigned long long n = 0;
    for (;;) {
        size_t got = fread(&r, 1, sizeof r, stdin);
        if (got == 0) {
            break;
        }
        if (got != sizeof r) {
            fprintf(stderr, "trace_decode: trailing %zu bytes are not a whole record\n",
                    got);
            return 2;
        }
        emit(&r);
        n++;
    }
    fprintf(stderr, "trace_decode: %llu records\n", n);
    return 0;
}
