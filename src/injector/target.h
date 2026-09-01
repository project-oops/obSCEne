/*
 * Target Process Resolver.
 *
 * Discovers and resolves target process IDs (foreground game title,
 * specific process name, or configured title ID).
 */

#ifndef OBSCENE_INJECTOR_TARGET_H
#define OBSCENE_INJECTOR_TARGET_H

#include <stddef.h>
#include <stdint.h>
#include "common/freestd.h"

/**
 * Find process ID by thread / process name.
 */
pid_t target_find_by_name(const char *name);

/**
 * Discover the active foreground application process.
 */
pid_t target_find_foreground_app(void);

/**
 * Resolve target process ID from an optional target specification string.
 * If target_spec is NULL or empty, defaults to the foreground application.
 */
pid_t target_resolve(const char *target_spec);

#endif /* OBSCENE_INJECTOR_TARGET_H */
