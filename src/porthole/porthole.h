/*
 * Porthole - the wire contract.
 *
 * Porthole is the DIY remote-play stand-in: our own video stream out and our own controller
 * state in, over our own payload, so watching and playing a jailbroken target never speaks the
 * vendor's remote-play protocol. The design is prosperous/docs/VIDEO.md "Part three: Porthole";
 * this subtree is the target-side payload, and this header is the half of it that is *decided*
 * rather than *discovered* - the bytes on the two sockets.
 *
 * Two sockets, two directions, no negotiation:
 *   9805  --->  encoded video out (raw Annex-B H.264, start-code delimited, no container of ours)
 *   9806  <---  controller state in (a fixed 24-byte PORTHOLE record, one per update, up to 60/s)
 *
 * Everything here is a decision both ends of the wire honour; nothing here is measured off the
 * platform. What *is* still a question - can an unsigned payload reach the hardware encoder at
 * all - lives in porthole.c behind PORTHOLE_NO_ENCODER, and is what the obSCEne probes answer.
 *
 * Freestanding: no libc. Fixed-width types only, chosen so the payload writes structures it
 * already holds in the platform's own little-endian order rather than swapping bytes it might
 * swap wrongly.
 */
#ifndef PORTHOLE_H
#define PORTHOLE_H

#include <stddef.h>
#include <stdint.h>

/* The two ports, adjacent so they are memorable together, and clear of every port the chain
 * already used as measured on 2026-08-25 (9021 loader, 9022 the frame-grabber, 2121/3232/2323/
 * 8084, 6967 scripted input). A collision is a one-line change and a note in VIDEO.md. */
#define PORTHOLE_PORT_VIDEO 9805u
#define PORTHOLE_PORT_INPUT 9806u

/* The controller record, 24 bytes, little-endian, one per update.
 *
 * Absolute state, never deltas: the newest record supersedes every older one, so a receiver
 * behind by three applies the last and drops two. The button bit layout and the stick range are
 * **not ours to choose** - they are the target's own pad structure, confirmed empirically by the
 * Ghostpad project against real hardware (see obSCEne ACKNOWLEDGEMENTS.md when this lands). This
 * header fixes only the record's *shape*; the meaning of individual button bits is Ghostpad's and
 * belongs beside the code that fills them, not here.
 */
#define PORTHOLE_PAD_MAGIC0 'P'
#define PORTHOLE_PAD_MAGIC1 'P'
#define PORTHOLE_PAD_MAGIC2 'A'
#define PORTHOLE_PAD_MAGIC3 'D'

/* Highest slot a record may name. A record for a fifth pad is a sender believing something
 * untrue, and is refused rather than clamped onto the fourth - delivering it as pad three's
 * input would put one person's controls on another. */
#define PORTHOLE_PAD_MAX_SLOT 3u

typedef struct porthole_pad {
    uint8_t  magic[4];   /* offset 0:  "PPAD" */
    uint16_t version;    /* offset 4 */
    uint8_t  slot;       /* offset 6:  0..PORTHOLE_PAD_MAX_SLOT */
    uint8_t  reserved0;  /* offset 7:  zero, checked */
    uint32_t buttons;    /* offset 8:  one bit each, the target's own layout (Ghostpad) */
    uint8_t  sticks[4];  /* offset 12: LX, LY, RX, RY; unsigned, 128 is centre */
    uint8_t  triggers[2];/* offset 16: L2, R2 pressure; a trigger sets its bit AND its pressure */
    uint16_t reserved1;  /* offset 18: zero, checked - room for gyro/touchpad/rumble via version */
    uint32_t sequence;   /* offset 20: per-slot, so "behind by two" is answerable */
} porthole_pad;

/* The record is exactly 24 bytes with no padding. A parser that finds field boundaries at 250 Hz
 * is a parser that drops inputs, so the layout is fixed and checked, not discovered. */
#define PORTHOLE_PAD_BYTES 24u

/* What a payload operation reports. A non-zero status means the thing did not happen, and each
 * value says why - "it did not work" and "it worked and produced nothing" must never look alike.
 * PORTHOLE_NO_ENCODER is the one that decides whether Porthole is built at all (see porthole.c). */
typedef enum porthole_status {
    PORTHOLE_OK = 0,
    PORTHOLE_UNIMPLEMENTED = 1, /* scaffold: this half is not built yet */
    PORTHOLE_NO_ENCODER = 2,    /* the go/no-go: the hardware encoder could not be reached */
    PORTHOLE_NO_DISPLAY = 3,    /* nothing composited to capture */
    PORTHOLE_NO_PAD = 4,        /* pad injection unavailable */
    PORTHOLE_NET = 5,           /* a socket call refused */
    PORTHOLE_BAD_RECORD = 6     /* an input record failed its own checks (magic/slot/reserved) */
} porthole_status;

/* Reads a 24-byte controller record, checking its magic, slot and reserved bytes, into `out`.
 * Pure and freestanding, so both ends can share it and it can be unit-tested off-hardware.
 * Returns PORTHOLE_OK, or PORTHOLE_BAD_RECORD for a record the payload must not act on. */
porthole_status porthole_pad_decode(const uint8_t bytes[PORTHOLE_PAD_BYTES], porthole_pad *out);

/* The three things Porthole does, and the loop that drives them. Scaffolded in porthole.c: each
 * reports a status until the real implementation lands, all gated on the encoder go/no-go.
 *
 *   porthole_encoder_open    reach the hardware encoder and hold a session (the go/no-go)
 *   porthole_capture_encode  one encoded access unit from the composited output, into the buffer
 *   porthole_pad_apply       apply one already-decoded controller record to the target's pads
 *   porthole_run             open the two sockets and serve video/apply input until stopped
 */
porthole_status porthole_encoder_open(void);
porthole_status porthole_capture_encode(uint8_t *out, size_t cap, size_t *len);
porthole_status porthole_pad_apply(const porthole_pad *pad);
porthole_status porthole_run(void);

#endif /* PORTHOLE_H */
