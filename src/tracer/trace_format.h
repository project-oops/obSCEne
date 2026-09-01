/*
 * The tracer wire format - the one interface between the on-console payload and the
 * host.
 *
 * This is deliberately a separate, self-contained header. The on-console *encoder*
 * includes it and nothing else from obSCEne; the host *decoder* includes it and nothing
 * else. It has no dependency on the probe, the runtime, or the tooling, so it can be
 * built and tested on its own (see the Makefile and trace_selftest.c) long before any
 * injection code exists.
 *
 * # Two rules carried from obSCEne
 *
 * 1. **The format is a contract.** Field order and meaning do not change without
 * bumping TRACE_VERSION. New kinds append; existing ones are frozen. A decoder checks
 * the version and refuses a record size it does not recognise. (obSCEne principle 3.)
 *
 * 2. **Fixed-size records, binary, no formatting on the hot path.** A stub that fires
 * on a game's every call has nanoseconds. It copies a fixed struct into a ring and
 * returns; all interpretation happens off-console in the decoder. Every field here is a
 * raw machine value, never a string built at call time.
 *
 * # Endianness
 *
 * The payload runs on x86-64 and so does the decoder, so records are stored in native
 * little-endian with no per-field serialisation. The header still carries an endian
 * marker so a decoder on a different-endian host refuses the stream rather than reading
 * it backwards - the same "a wrong reading is worse than a refusal" stance the probe
 * takes.
 */
#ifndef OBS_TRACE_FORMAT_H
#define OBS_TRACE_FORMAT_H

#include <stdint.h>

#define OBS_TRACE_MAGIC "OBSTRACE"
#define OBS_TRACE_VERSION 1u
#define OBS_TRACE_REC_SIZE 64u
#define OBS_TRACE_ENDIAN 0x01020304u

/* How many integer arguments a record can carry. SysV passes the first six in registers
 * (rdi, rsi, rdx, rcx, r8, r9); an entry record captures exactly those. */
#define OBS_TRACE_ARGS 6u

/* Inline payload capacity for an out-parameter chunk, in bytes. A chunk holds addr and
 * offset in arg[0]/arg[1] and up to this many bytes packed into arg[2..5]. Anything
 * larger is not inlined: it becomes a (length, hash) record instead (see the provenance
 * note in the README).
 *
 * There is no spare byte in the record for a flag, so the two OUTBUF forms are told
 * apart by inspection, which is unambiguous: a hashed chunk has argc==0 and a non-zero
 * length in arg[1] (a large buffer always has length >= 1), while an inlined chunk
 * carries its length in argc. The only argc==0 inline case is a genuinely empty buffer,
 * which also has arg[1]==0. */
#define OBS_TRACE_OUTBUF_INLINE 32u

enum obs_trace_kind {
    OBS_TRACE_ENTRY = 1u, /* a call happened: nid, plus argc values in arg[] */
    OBS_TRACE_EXIT = 2u,  /* a call returned: arg[0] is the return value  */
    OBS_TRACE_OUTBUF =
        3u, /* out-parameter bytes: arg[0]=addr, arg[1]=offset, bytes in arg[2+]*/
    OBS_TRACE_NAME =
        4u, /* a nid->name hint the payload happened to resolve; 48 name bytes  */
    OBS_TRACE_COUNT =
        5u, /* total calls seen for a nid: arg[0]=count (emitted at drain)      */
};

/* Exactly 64 bytes, cache-line sized, no padding. The static assert below is
 * load-bearing: the decoder reads records by size, so a layout change that altered this
 * would silently misread every field after the change. */
struct obs_trace_rec {
    uint8_t kind; /* enum obs_trace_kind                              */
    uint8_t argc; /* ENTRY: number of valid args; OUTBUF: payload len */
    uint16_t tid; /* thread id, for per-thread ordering              */
    uint32_t seq; /* per-thread call sequence / correlation id       */
    uint64_t nid; /* function identity (the import's NID hash)        */
    uint64_t arg[OBS_TRACE_ARGS];
};

/* 32-byte stream header, once at the front of a trace. */
struct obs_trace_hdr {
    char magic[8];        /* OBS_TRACE_MAGIC, not NUL-terminated */
    uint32_t version;     /* OBS_TRACE_VERSION                   */
    uint32_t rec_size;    /* OBS_TRACE_REC_SIZE                  */
    uint32_t endian;      /* OBS_TRACE_ENDIAN, byte-order probe  */
    uint32_t reserved[3]; /* zero; room to grow the header       */
};

/* C11 static assertions. If either fires, the format drifted and every decoder is
 * wrong. */
#ifdef __STDC_VERSION__
#if __STDC_VERSION__ >= 201112L
_Static_assert(sizeof(struct obs_trace_rec) == OBS_TRACE_REC_SIZE,
               "trace record must be exactly 64 bytes");
_Static_assert(sizeof(struct obs_trace_hdr) == 32u,
               "trace header must be exactly 32 bytes");
#endif
#endif

#endif /* OBS_TRACE_FORMAT_H */
