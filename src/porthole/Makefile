# Standalone build for the Porthole payload scaffold.
#
# Independent of the top-level Makefile, exactly like ../tracer/: this subtree is new work that
# touches nothing in the probe, so it builds and tests on its own. `make -C porthole check`
# proves the wire contract holds on a host; `make -C porthole skeleton` proves the payload
# compiles as freestanding C for the target, without yet linking a real payload.

CC ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion \
          -Wstrict-prototypes -Wmissing-prototypes -O1

# The target skeleton cross-compiles for freebsd, which needs clang specifically - `cc` on a build
# host is often gcc, and gcc does not take `-target`. Kept separate from CC so the host check can
# use whatever `cc` is while the skeleton always uses clang, as the probe's payload build does.
TARGET_CC ?= clang

# Freestanding flags for the target-side skeleton, matching the probe's payload build. The object
# is compiled but not linked: a real payload needs the CRT and net wiring, which is the work the
# encoder go/no-go gates - this proves only that the skeleton is sound freestanding C.
TARGET_CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion \
                 -Wstrict-prototypes -Wmissing-prototypes -Wvla \
                 -target x86_64-unknown-freebsd -ffreestanding -fno-builtin -fPIC \
                 -fno-stack-protector -fvisibility=hidden

BUILD ?= build

.PHONY: all check skeleton clean

all: $(BUILD)/porthole_selftest skeleton

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/porthole_selftest: porthole_selftest.c porthole.c porthole.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ porthole_selftest.c porthole.c

# The gate: the wire contract round-trips and the scaffold is honestly a scaffold.
check: $(BUILD)/porthole_selftest
	@./$(BUILD)/porthole_selftest

# Proves the payload half compiles freestanding for the target, object-only (no CRT/link yet).
skeleton: | $(BUILD)
	$(TARGET_CC) $(TARGET_CFLAGS) -c -o $(BUILD)/porthole.o porthole.c
	@echo "porthole skeleton: compiles freestanding for the target (object only, not linked)"

clean:
	rm -rf $(BUILD)
