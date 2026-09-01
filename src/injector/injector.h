/*
 * Injector Entry Point Interface.
 */

#ifndef OBSCENE_INJECTOR_INJECTOR_H
#define OBSCENE_INJECTOR_INJECTOR_H

#include "common/krw.h"

int injector_start(payload_args_t *args);

void klog_write(const char *msg);
void klog_write_hex(const char *prefix, uint64_t val);
void klog_write_num(const char *prefix, int64_t num);

#endif /* OBSCENE_INJECTOR_INJECTOR_H */
