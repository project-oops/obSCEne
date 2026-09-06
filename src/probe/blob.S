/*
 * Embedded probe ELF payload blob.
 *
 * Links obscene-payload.elf into the injector payload so that a single
 * `hw send` command transmits both injector and probe in one trip.
 */

.section .rodata
.globl __obscene_payload_start
.globl __obscene_payload_end
.align 16

__obscene_payload_start:
#ifdef EMBED_PAYLOAD_PATH
.incbin EMBED_PAYLOAD_PATH
#endif
__obscene_payload_end:

