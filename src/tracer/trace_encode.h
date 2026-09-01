/*
 * The recording core - what runs inside the traced process.
 *
 * Header-only and freestanding: it includes only <stdint.h> and trace_format.h,
 * allocates nothing, and calls no libc. All storage is caller-provided, because
 * on-console this lives in pages the payload mapped itself and the hot path must not
 * reach for an allocator. That also makes it trivially testable on the host - the
 * selftest hands it ordinary arrays.
 *
 * # The volume problem, solved by the sampler
 *
 * A game calls millions of functions a second. Recording every one fills storage in
 * seconds and perturbs timing enough to break the game. So detail is capped **per
 * nid**: the first OBS_TRACE_CAP calls of a function are recorded in full, and after
 * that only a counter moves. The 400,000th memcpy teaches nothing the first told us;
 * the count still travels, as a COUNT record at drain, so nothing is silently dropped -
 * a capped function is *known* to have been called that many times.
 *
 * # Reentrancy
 *
 * Nothing here calls a function the payload might have hooked, and nothing blocks. A
 * stub that fired mid-record and re-entered this code would still only touch its own
 * thread's buffer and sampler, which is why on-console each thread owns one of each.
 * (obSCEne's freestanding discipline is the reason this is even possible: no hidden
 * libc call to deadlock on.)
 */
#ifndef OBS_TRACE_ENCODE_H
#define OBS_TRACE_ENCODE_H

#include <stdint.h>

#include "trace_format.h"

/* Full detail for the first this-many calls of each distinct nid; counter only after.
 */
#ifndef OBS_TRACE_CAP
#define OBS_TRACE_CAP 64u
#endif

/* ---- the append buffer
 * -------------------------------------------------------------------
 *
 * A plain bump buffer of fixed records. On-console the real thing is a wrapping single-
 * producer/single-consumer ring drained by a low-priority thread; the buffer here is
 * the same record stream without the wrap, which is what a drain would see and what the
 * decoder reads. Kept deliberately simple so the *format* is what is under test, not a
 * lock-free ring.
 */
struct obs_trace_buf {
    struct obs_trace_rec *recs; /* caller-provided storage        */
    uint32_t cap;               /* capacity in records            */
    uint32_t head;              /* next write index               */
    uint32_t dropped;           /* records lost to a full buffer  */
};

static inline void obs_trace_buf_init(struct obs_trace_buf *b,
                                      struct obs_trace_rec *storage, uint32_t cap) {
    b->recs = storage;
    b->cap = cap;
    b->head = 0;
    b->dropped = 0;
}

/* Append one record. A full buffer drops, and counts the drop, rather than wrapping - a
 * drop is a fact the decoder reports, never a silent gap. Returns 1 on write, 0 on
 * drop. */
static inline int obs_trace_buf_push(struct obs_trace_buf *b,
                                     const struct obs_trace_rec *r) {
    if (b->head >= b->cap) {
        b->dropped++;
        return 0;
    }
    b->recs[b->head] = *r;
    b->head++;
    return 1;
}

/* ---- the per-nid sampler
 * -----------------------------------------------------------------
 *
 * An open-addressed table of {nid, count}, linear-probed, power-of-two capacity.
 * `obs_trace_hit` increments a nid's count and returns 1 while the count is at or below
 * the cap (record it), 0 once past (counter only). A full table treats every further
 * nid as over-cap: on-console it is sized past the count of distinct imports, and
 * protecting the device beats recording detail for a nid that overflowed the table.
 */
struct obs_trace_sampler {
    uint64_t *nids;   /* caller-provided, capacity slots; 0 means empty */
    uint32_t *counts; /* caller-provided, capacity slots                */
    uint32_t cap;     /* MUST be a power of two                         */
    uint32_t cap_n;   /* detail cap per nid                            */
    uint32_t full;    /* nids that did not fit the table               */
};

static inline void obs_trace_sampler_init(struct obs_trace_sampler *s, uint64_t *nids,
                                          uint32_t *counts, uint32_t cap,
                                          uint32_t cap_n) {
    s->nids = nids;
    s->counts = counts;
    s->cap = cap;
    s->cap_n = cap_n;
    s->full = 0;
    for (uint32_t i = 0; i < cap; i++) {
        s->nids[i] = 0;
        s->counts[i] = 0;
    }
}

/* A cheap integer mix so consecutive nids do not cluster in the table. Splitmix64
 * finaliser. */
static inline uint64_t obs_trace_mix(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

/* Returns the count for a nid without touching it. 0 if never seen. */
static inline uint32_t obs_trace_count(const struct obs_trace_sampler *s,
                                       uint64_t nid) {
    uint32_t mask = s->cap - 1u;
    uint32_t i = (uint32_t)obs_trace_mix(nid) & mask;
    for (uint32_t probe = 0; probe < s->cap; probe++) {
        uint32_t at = (i + probe) & mask;
        if (s->nids[at] == 0) {
            return 0;
        }
        if (s->nids[at] == nid) {
            return s->counts[at];
        }
    }
    return 0;
}

/* Increment a nid's count; return 1 if this call should be recorded in detail. */
static inline int obs_trace_hit(struct obs_trace_sampler *s, uint64_t nid) {
    uint32_t mask = s->cap - 1u;
    uint32_t i = (uint32_t)obs_trace_mix(nid) & mask;
    for (uint32_t probe = 0; probe < s->cap; probe++) {
        uint32_t at = (i + probe) & mask;
        if (s->nids[at] == nid) {
            s->counts[at]++;
            return s->counts[at] <= s->cap_n;
        }
        if (s->nids[at] == 0) {
            s->nids[at] = nid;
            s->counts[at] = 1;
            return 1;
        }
    }
    /* Table full and this nid was not already in it. Protect the device. */
    s->full++;
    return 0;
}

/* ---- emitting records
 * --------------------------------------------------------------------
 *
 * Each helper builds a fixed record and pushes it. They are what a hook stub calls:
 * sample first, and only build a record when the sampler says this call is still
 * interesting.
 */

static inline void obs_trace_entry(struct obs_trace_buf *b, uint16_t tid, uint32_t seq,
                                   uint64_t nid, const uint64_t *args, uint8_t argc) {
    struct obs_trace_rec r;
    r.kind = (uint8_t)OBS_TRACE_ENTRY;
    r.argc = argc > OBS_TRACE_ARGS ? (uint8_t)OBS_TRACE_ARGS : argc;
    r.tid = tid;
    r.seq = seq;
    r.nid = nid;
    for (uint32_t i = 0; i < OBS_TRACE_ARGS; i++) {
        r.arg[i] = i < r.argc ? args[i] : 0;
    }
    obs_trace_buf_push(b, &r);
}

static inline void obs_trace_exit(struct obs_trace_buf *b, uint16_t tid, uint32_t seq,
                                  uint64_t nid, uint64_t ret) {
    struct obs_trace_rec r;
    r.kind = (uint8_t)OBS_TRACE_EXIT;
    r.argc = 0;
    r.tid = tid;
    r.seq = seq;
    r.nid = nid;
    r.arg[0] = ret;
    for (uint32_t i = 1; i < OBS_TRACE_ARGS; i++) {
        r.arg[i] = 0;
    }
    obs_trace_buf_push(b, &r);
}

/* FNV-1a 64. Used to stand in for a buffer too large to inline - a length and a hash
 * carry the fact that bytes were written and let two runs be compared, without shipping
 * the bytes. That is the provenance rule made mechanical: raw bytes only for small
 * out-params, a hash for the rest, so the corpus never redistributes a copyrighted
 * title's data. */
static inline uint64_t obs_trace_fnv1a(const uint8_t *p, uint32_t len) {
    uint64_t h = 1469598103934665603ULL;
    for (uint32_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 1099511628211ULL;
    }
    return h;
}

/* Record out-parameter bytes at a guest address. Small buffers inline; large ones
 * become a (length, hash) record. `threshold` is the inline limit and must be <=
 * OBS_TRACE_OUTBUF_INLINE.
 */
static inline void obs_trace_outbuf(struct obs_trace_buf *b, uint64_t nid,
                                    uint64_t addr, const uint8_t *buf, uint32_t len,
                                    uint32_t threshold) {
    struct obs_trace_rec r;
    r.kind = (uint8_t)OBS_TRACE_OUTBUF;
    r.tid = 0;
    r.seq = 0;
    r.nid = nid;
    if (threshold > OBS_TRACE_OUTBUF_INLINE) {
        threshold = OBS_TRACE_OUTBUF_INLINE;
    }
    if (len <= threshold) {
        r.argc = (uint8_t)len;
        r.arg[0] = addr;
        r.arg[1] = len;
        /* Pack up to 32 bytes into arg[2..5]. */
        uint8_t *payload = (uint8_t *)&r.arg[2];
        for (uint32_t i = 0; i < OBS_TRACE_OUTBUF_INLINE; i++) {
            payload[i] = i < len ? buf[i] : 0;
        }
    } else {
        r.argc = 0;
        r.arg[0] = addr;
        r.arg[1] = len;
        r.arg[2] = obs_trace_fnv1a(buf, len);
        r.arg[3] = 0;
        r.arg[4] = 0;
        r.arg[5] = 0;
    }
    /* The hashed flag lives in the top bit of tid's high byte via a dedicated field
     * would be cleaner, but the record has no spare byte; instead the decoder infers
     * hashed from argc==0 with a non-zero length, which only the hashed path produces.
     * Documented in the decoder. */
    obs_trace_buf_push(b, &r);
}

/* Emit a nid->name hint (up to 47 bytes + NUL) the payload resolved locally. Optional:
 * names usually come from joining nids against the corpus off-console, but a payload
 * that already resolved one can say so. */
static inline void obs_trace_name(struct obs_trace_buf *b, uint64_t nid,
                                  const char *name) {
    struct obs_trace_rec r;
    r.kind = (uint8_t)OBS_TRACE_NAME;
    r.argc = 0;
    r.tid = 0;
    r.seq = 0;
    r.nid = nid;
    uint8_t *dst = (uint8_t *)&r.arg[0];
    uint32_t i = 0;
    for (; i < OBS_TRACE_ARGS * 8u - 1u && name[i] != 0; i++) {
        dst[i] = (uint8_t)name[i];
    }
    for (; i < OBS_TRACE_ARGS * 8u; i++) {
        dst[i] = 0;
    }
    obs_trace_buf_push(b, &r);
}

/* Emit the final count for one nid. Called at drain for every nid whose detail was
 * capped, so a capped function reports how many times it really ran. */
static inline void obs_trace_count_rec(struct obs_trace_buf *b, uint64_t nid,
                                       uint32_t total) {
    struct obs_trace_rec r;
    r.kind = (uint8_t)OBS_TRACE_COUNT;
    r.argc = 0;
    r.tid = 0;
    r.seq = 0;
    r.nid = nid;
    r.arg[0] = total;
    for (uint32_t i = 1; i < OBS_TRACE_ARGS; i++) {
        r.arg[i] = 0;
    }
    obs_trace_buf_push(b, &r);
}

#endif /* OBS_TRACE_ENCODE_H */
