/*
 * Porthole - self-test for the wire contract.
 *
 * The payload is a scaffold, but the bytes on the two sockets are a *decision* both
 * ends honour, so the one thing worth testing today is that the record layout and its
 * decoder agree - before either end is written against a shape that turns out wrong.
 * Runs on an ordinary host, like the probe's `make host`: a contract nobody can
 * exercise is not a contract.
 */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "porthole.h"

/* The record is 24 bytes with its fields exactly where VIDEO.md says. If the compiler
 * padded it, every offset below the pad is wrong and both ends would disagree silently.
 */
_Static_assert(sizeof(porthole_pad) == PORTHOLE_PAD_BYTES,
               "the pad record must be 24 bytes");
_Static_assert(offsetof(porthole_pad, version) == 4, "version at 4");
_Static_assert(offsetof(porthole_pad, slot) == 6, "slot at 6");
_Static_assert(offsetof(porthole_pad, buttons) == 8, "buttons at 8");
_Static_assert(offsetof(porthole_pad, sticks) == 12, "sticks at 12");
_Static_assert(offsetof(porthole_pad, triggers) == 16, "triggers at 16");
_Static_assert(offsetof(porthole_pad, sequence) == 20, "sequence at 20");

/* A well-formed record: slot 2, a couple of buttons, sticks off-centre, R2
 * half-pressed, seq 7. */
static void good_record(uint8_t b[PORTHOLE_PAD_BYTES]) {
    memset(b, 0, PORTHOLE_PAD_BYTES);
    b[0] = 'P';
    b[1] = 'P';
    b[2] = 'A';
    b[3] = 'D';
    b[4] = 1;    /* version 1 */
    b[6] = 2;    /* slot 2 */
    b[8] = 0x05; /* buttons: bits 0 and 2 */
    b[12] = 128;
    b[13] = 200; /* LX centre, LY pushed */
    b[14] = 128;
    b[15] = 128; /* RX/RY centre */
    b[17] = 127; /* R2 half */
    b[20] = 7;   /* sequence 7 */
}

int main(void) {
    unsigned int failures = 0;
    uint8_t bytes[PORTHOLE_PAD_BYTES];
    porthole_pad pad;

    /* A good record decodes, and the fields land where they were put. */
    good_record(bytes);
    if (porthole_pad_decode(bytes, &pad) != PORTHOLE_OK) {
        printf("FAIL: a well-formed record was rejected\n");
        failures++;
    } else if (pad.version != 1 || pad.slot != 2 || pad.buttons != 0x05u ||
               pad.sticks[1] != 200 || pad.triggers[1] != 127 || pad.sequence != 7) {
        printf("FAIL: a field decoded to the wrong value\n");
        failures++;
    }

    /* Bad magic is refused - a stray connection writing anything must not read as a
     * pad. */
    good_record(bytes);
    bytes[2] = 'X';
    if (porthole_pad_decode(bytes, &pad) != PORTHOLE_BAD_RECORD) {
        printf("FAIL: a record with wrong magic was accepted\n");
        failures++;
    }

    /* A slot beyond the fourth is refused, not clamped onto pad three. */
    good_record(bytes);
    bytes[6] = 4;
    if (porthole_pad_decode(bytes, &pad) != PORTHOLE_BAD_RECORD) {
        printf("FAIL: a fifth-pad record was accepted\n");
        failures++;
    }

    /* A non-zero reserved byte is refused, so a version bump into that room is a clean
     * upgrade. */
    good_record(bytes);
    bytes[7] = 0xFF;
    if (porthole_pad_decode(bytes, &pad) != PORTHOLE_BAD_RECORD) {
        printf("FAIL: a record with a dirty reserved byte was accepted\n");
        failures++;
    }

    /* The scaffold is honest about being a scaffold: the three real operations refuse,
     * and the go/no-go refuses with the specific "no encoder" that the obSCEne probe
     * exists to resolve. */
    if (porthole_encoder_open() != PORTHOLE_NO_ENCODER) {
        printf("FAIL: the encoder stub should report NO_ENCODER until the probe says "
               "otherwise\n");
        failures++;
    }
    if (porthole_run() != PORTHOLE_NO_ENCODER) {
        printf("FAIL: run() should refuse cleanly while the encoder is unreachable\n");
        failures++;
    }

    if (failures == 0) {
        printf("porthole selftest: ok (wire contract holds; payload correctly "
               "scaffolded)\n");
        return 0;
    }
    printf("porthole selftest: %u failure(s)\n", failures);
    return 1;
}
