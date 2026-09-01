/*
 * In-memory ELF Loader Interface.
 *
 * libelfldr-shaped ELF loader: validates ELF64 binaries, maps PT_LOAD segments,
 * applies R_X86_64_RELATIVE relocations, and configures segment memory protections.
 */

#ifndef OBSCENE_INJECTOR_LOADER_H
#define OBSCENE_INJECTOR_LOADER_H

#include <stddef.h>
#include <stdint.h>
#include "common/freestd.h"
#include "common/krw.h"

/**
 * Validate that in-memory bytes form a valid ELF64 x86_64 object.
 * Returns 0 on success, or -1 on error.
 */
int loader_validate_elf(const uint8_t *elf_data, size_t elf_size);

/**
 * Load an in-memory ELF64 into the address space of the specified target process.
 * Returns the entry point address in the target process, or 0 on failure.
 */
uintptr_t loader_load_into_proc(pid_t pid, const uint8_t *elf_data, size_t elf_size,
                                uintptr_t target_libkernel_base,
                                const obs_kexport_table_t *kexport_table,
                                uintptr_t *out_base_addr, size_t *out_base_size);

#endif /* OBSCENE_INJECTOR_LOADER_H */

