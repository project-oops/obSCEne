/*
 * Porthole - the target-side payload, scaffolded.
 *
 * This is the shape of the on-console payload from prosperous/docs/VIDEO.md "Part three:
 * Porthole", laid out so that filling it in is an afternoon rather than a fortnight of deciding.
 * **Almost nothing here is implemented on purpose.** The three things Porthole actually does -
 * reach the encoder, capture-and-encode a frame, apply a pad - are stubs that report
 * PORTHOLE_UNIMPLEMENTED, each with a note on what it will do and where its ground truth comes
 * from. The one exception is porthole_pad_decode, which is the wire contract and is real, because
 * a contract nobody can test is not a contract.
 *
 * The single question the whole payload rests on is porthole_encoder_open below: **can an
 * unsigned payload reach the hardware encoder?** That is not answered by guessing here - it is
 * answered by the obSCEne probes (src/sections/record.c and the encoder-reachability section),
 * and this stub returns PORTHOLE_NO_ENCODER until they say otherwise. If they say it cannot,
 * Porthole is not built and the fallback is the raw frame-grabber (VIDEO.md part two).
 *
 * Freestanding, and it stays that way: no libc, no allocation, no float, no variadics. Platform
 * calls are weak externs guarded before use, exactly as the probe's are, so a loader that does
 * not resolve one gets a clean status rather than a jump to zero.
 */
#include "porthole.h"

/* ---- the wire contract, which is real ------------------------------------------------------ */

porthole_status porthole_pad_decode(const uint8_t bytes[PORTHOLE_PAD_BYTES], porthole_pad *out) {
    if (bytes[0] != PORTHOLE_PAD_MAGIC0 || bytes[1] != PORTHOLE_PAD_MAGIC1
        || bytes[2] != PORTHOLE_PAD_MAGIC2 || bytes[3] != PORTHOLE_PAD_MAGIC3) {
        return PORTHOLE_BAD_RECORD;
    }
    /* Assembled a byte at a time rather than read through a cast: the buffer is off the wire with
     * no alignment guarantee, and a misaligned load is undefined even where it happens to work. */
    for (unsigned int i = 0; i < 4; i++) {
        out->magic[i] = bytes[i];
    }
    out->version   = (uint16_t)((uint16_t)bytes[4] | (uint16_t)((uint16_t)bytes[5] << 8));
    out->slot      = bytes[6];
    out->reserved0 = bytes[7];
    out->buttons   = (uint32_t)bytes[8] | ((uint32_t)bytes[9] << 8)
                   | ((uint32_t)bytes[10] << 16) | ((uint32_t)bytes[11] << 24);
    for (unsigned int i = 0; i < 4; i++) {
        out->sticks[i] = bytes[12 + i];
    }
    out->triggers[0] = bytes[16];
    out->triggers[1] = bytes[17];
    out->reserved1   = (uint16_t)((uint16_t)bytes[18] | (uint16_t)((uint16_t)bytes[19] << 8));
    out->sequence    = (uint32_t)bytes[20] | ((uint32_t)bytes[21] << 8)
                     | ((uint32_t)bytes[22] << 16) | ((uint32_t)bytes[23] << 24);

    /* A record that fails its own invariants is a sender believing something untrue; acting on it
     * puts one person's input on another's pad, or presses a button nobody pressed. Refused, not
     * clamped. The reserved bytes are checked so a version bump into that room is a clean upgrade
     * rather than a silent reinterpretation of old fields. */
    if (out->slot > PORTHOLE_PAD_MAX_SLOT || out->reserved0 != 0 || out->reserved1 != 0) {
        return PORTHOLE_BAD_RECORD;
    }
    return PORTHOLE_OK;
}

/* ---- the three things Porthole does, all scaffolded --------------------------------------- */

/*
 * THE GO/NO-GO. Reach the hardware video encoder (the VCE block the console uses for its own
 * recordings) and hold an open session.
 *
 * On hardware this drives `libSceVideoRecording` / `libSceVencCore`. obSCEne's `record.c` already
 * calls the *safe* half of that interface (the handle-only calls); what is unproven - and what
 * everything below depends on - is whether an unsigned payload can `Open`/`SetInfo` a session and
 * pull one encoded access unit. That needs those calls' parameter-struct layouts from a lawful
 * source, and it is precisely obSCEne's kind of question: call it, record what came back, grade
 * it by what it ran on.
 *
 * So this returns PORTHOLE_NO_ENCODER until the probe says the door opens. It is not a guess
 * dressed as success (D008); it is the honest "not established yet".
 */
porthole_status porthole_encoder_open(void) {
    return PORTHOLE_NO_ENCODER; /* TODO: gated on the obSCEne encoder-reachability probe. */
}

/*
 * Capture the composited output and hand back one encoded access unit (Annex-B H.264, as the
 * encoder emits it - no container of ours). `out`/`cap` describe the caller's buffer; `*len`
 * receives how many bytes were written.
 *
 * Depends entirely on porthole_encoder_open having succeeded, and on the display buffer being
 * reachable from an unsigned payload and in a known colour space - the second open question in
 * VIDEO.md. Nothing is captured or encoded here yet.
 */
porthole_status porthole_capture_encode(uint8_t *out, size_t cap, size_t *len) {
    (void)out;
    (void)cap;
    *len = 0;
    return PORTHOLE_UNIMPLEMENTED; /* TODO: capture composited buffer -> hardware encode -> AU. */
}

/*
 * Apply one decoded controller record to the target's pad state. The record has already passed
 * porthole_pad_decode, so `pad` is trusted here.
 *
 * On hardware this writes the target's own pad structure - button bits and stick range are
 * Ghostpad's, confirmed against real hardware, and a trigger must set BOTH its bit and its
 * pressure byte or the press does not register. Nothing is injected here yet.
 */
porthole_status porthole_pad_apply(const porthole_pad *pad) {
    (void)pad;
    return PORTHOLE_UNIMPLEMENTED; /* TODO: write the target pad structure (Ghostpad layout). */
}

/*
 * The payload's whole life: open the two listening sockets (9805 out, 9806 in), reach the
 * encoder, then serve encoded frames to whoever connects on 9805 and apply pad records arriving
 * on 9806 until told to stop. Sockets go through the same `obs_net_backend_*` the probe's
 * transport uses; nothing here duplicates a listener.
 *
 * Scaffolded: the loop is described, not run. The moment porthole_encoder_open answers OK, this
 * becomes the few hundred lines VIDEO.md promises, with nothing in porthole.h to change.
 */
porthole_status porthole_run(void) {
    porthole_status encoder = porthole_encoder_open();
    if (encoder != PORTHOLE_OK) {
        return encoder; /* No encoder, no stream. Refuse cleanly rather than serve zeros. */
    }
    /* TODO, once the door is open:
     *   - listen on PORTHOLE_PORT_VIDEO and PORTHOLE_PORT_INPUT (obs_net_backend_listen)
     *   - on a video connection: loop porthole_capture_encode -> write the AU, byte-exact framing
     *   - on an input connection: loop read 24 bytes -> porthole_pad_decode -> porthole_pad_apply,
     *     newest-supersedes-oldest per slot by the record's sequence number
     *   - audio is a third socket and the same argument, deliberately after these two work
     */
    return PORTHOLE_UNIMPLEMENTED;
}
