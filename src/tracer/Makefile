# Standalone build for the tracer format, encoder, and decoder.
#
# Deliberately independent of the top-level Makefile: this subtree is new work that touches
# nothing in the probe, so it builds and tests on its own. `make -C tracer check` proves the
# format round-trips and the decoder produces parseable OBS| records.

CC ?= clang
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion \
          -Wstrict-prototypes -Wmissing-prototypes -O1

BUILD ?= build

.PHONY: all check clean

all: $(BUILD)/trace_decode $(BUILD)/trace_selftest

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/trace_decode: trace_decode.c trace_format.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ trace_decode.c

$(BUILD)/trace_selftest: trace_selftest.c trace_encode.h trace_format.h | $(BUILD)
	$(CC) $(CFLAGS) -o $@ trace_selftest.c

# The gate: the encoder/format round-trips and the safety policies hold, then the on-disk
# decoder turns the same stream into OBS| records without error.
check: all
	@echo "--- format + encoder self-test ---"
	@cd $(BUILD) && ./trace_selftest
	@echo "--- decoder over the produced stream ---"
	@$(BUILD)/trace_decode < $(BUILD)/trace_selftest.bin > $(BUILD)/trace_selftest.obs
	@echo "sample of the decoded, parseable output:"
	@grep -E '^OBS\|(tracemeta|count|outbuf)\|' $(BUILD)/trace_selftest.obs | head -6
	@echo "--- record-type tally ---"
	@sed -E 's/^(OBS\|[a-z]+)\|.*/\1/' $(BUILD)/trace_selftest.obs | sort | uniq -c

clean:
	rm -rf $(BUILD)
