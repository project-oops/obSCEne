# D284 - obs_read_header is bounded by the caller's buffer, not by a constant


`obs_read_header` read `OBS_SELFAUDIT_LEN` (0x40) bytes into whatever buffer it was handed, and the
size was a constant rather than a parameter. Both of `048-selfaudit`'s checks call it, and they do
not agree on the buffer:

* `check_confirm_format_table` passes `static unsigned char buf[OBS_SELFAUDIT_LEN]` - correct, and
  static rather than stack.
* `check_metadata_differential` passes `unsigned char app0_hdr[32]` and `unsigned char header[32]`.

So the metadata check overran a 32-byte stack buffer by 32 bytes on every read that returned more
than 32 - which is every `param.json` and every `eboot.bin` - and `header` is declared *inside* the
directory walk, so it happened once per title directory. What lives beside those buffers is the
four counters and the dirent loop state (`pos`, `reclen`, `n`) the walk is keeping at the same time.

**The report is what exposed it, and it exposed it as a contradiction rather than as a crash.** The
hardware run reported `metadata/ps5_param_json|45792` - not a number of titles any console has - and
`metadata/gen5_containers|2672` found by walking `/system/vsh/app` and `/user/app`. But
`confirm-table`, walking those same roots with the same magic test through the correctly-sized
buffer, found nothing readable there at all and fell through to its `/app0` last resort
(`selfaudit/source|own app`). Both cannot be true of one filesystem. The one that came through the
overrun buffer is the one to disbelieve, and `0xb2e0` - the check's own pass value - is just the
corrupted counter read back.

The fix is the boundary, not the buffer sizes: `obs_read_header` takes a `cap` and reads
`min(cap, ...)`, so a short buffer cannot be overrun by a later caller either. `obs_scan_root` grew
a `buf_cap` alongside it rather than being left to assume `OBS_SELFAUDIT_LEN`, because its `buf` is
a pointer parameter - `sizeof` there would silently be 8 and reintroduce the same class of bug one
level down. Every call site now passes `sizeof` of a real array.

**`confirm-table`'s result is unaffected and stands**: `048-selfaudit/confirm-table|pass|0x9`, all
nine fixed header rows matching, read through the correctly-sized static buffer. Because this was a
payload injected into a hijacked host application, `/app0/eboot.bin` is that host's container - a
real vendor gen-5 SELF, not obSCEne's own - so selfish's format table is confirmed against vendor
bytes at the current generation. (The `own app` label is honest about *which path* was read but
undersells it in the payload case; distinguishing payload-host from standalone-package there is
worth doing and is not done here.)

`metadata-differential`'s numbers are **withdrawn, not corrected**: they were measured through
corrupted memory, so `ps5_native_only` is not established and no report should carry it until the
walk is re-run against the bounded reader. The zeros (`ps4_param_sfo|0`) and the `1` may well prove
out - they are the values the run would want - but they came off the same stack and are not evidence
yet.

Verified: `selfaudit.c` compiles clean in both the host and target shapes under the project's full
flag set (`-Werror -Wconversion -Wsign-conversion -Wshadow -Wstrict-prototypes`). `make host` and
`make module` could **not** be completed this session for an unrelated reason in a file this change
does not touch: `src/probe/runtime.c` calls `sceKernelGetModuleList` and `sceKernelGetModuleInfo`
with no declaration (`-Wimplicit-function-declaration`, fatal under `-Werror`), which is another
session's in-flight module-enumeration work. The re-run on hardware is owed once that builds.
