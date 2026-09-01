/*
 * Linker script for obscene-injector.elf (plain ET_DYN payload).
 *
 * Freestanding x86-64 payload with 16 KiB page alignment for PS5 runtime.
 */

OUTPUT_FORMAT("elf64-x86-64", "elf64-x86-64", "elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(injector_start)

PHDRS {
    text PT_LOAD FLAGS(5);       /* Read + Execute */
    rodata PT_LOAD FLAGS(4);     /* Read Only */
    data PT_LOAD FLAGS(6);       /* Read + Write */
    dynamic PT_DYNAMIC FLAGS(6); /* Dynamic Table */
}

SECTIONS {
    . = SIZEOF_HEADERS;

    .text : ALIGN(0x4000) {
        *(.text .text.*)
    } :text

    .rodata : ALIGN(0x4000) {
        *(.rodata .rodata.*)
    } :rodata

    .data : ALIGN(0x4000) {
        *(.data .data.*)
        *(.got)
        *(.got.plt)
    } :data

    .dynamic : ALIGN(0x4000) {
        *(.dynamic)
    } :data :dynamic

    .bss : ALIGN(0x4000) {
        *(.bss .bss.*)
        *(COMMON)
    } :data

    /DISCARD/ : {
        *(.comment)
        *(.note.*)
        *(.eh_frame)
        *(.eh_frame_hdr)
    }
}

