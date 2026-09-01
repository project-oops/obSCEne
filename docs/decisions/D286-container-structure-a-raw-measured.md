# D286 - container-structure: a raw measured dump of a real gen-5 container


`048-selfaudit` had two checks: `confirm-table` (do the nine fixed header constants still hold at
this generation) and `metadata-differential` (what does the installed-title population look like).
Neither reports the thing a packager most needs - the *shape* of a real current-generation
container - so building a gen-5 package means inferring header_size, the segment table and the
ex_info/npdrm blocks from the previous generation and hoping the layout held.

Added `048-selfaudit/container-structure`. It locates a real container the same way `confirm-table`
does (vendor roots first, `/app0` last), reads a 4 KiB window of it, and reports as **raw measured
values**:

* the self_header per-file fields - `header_size`, `meta_size`, `file_size`, `segment_count`;
* the segment table - each entry's `flags`, `file_size`, `memory_size`, bounded by the read window
  and a 64-entry cap, announcing a truncation rather than letting a short read read as complete;
* `ex_info` - `paid`, `ptype`, `app_version`, `fw_version`;
* `npdrm` - `type`, and that a content id is present (19 bytes); and
* the installed `sce_sys/` file inventory of the same title - names only.

**It does not decode.** What a segment flag bit or an `ex_info.ptype` value *means* lives in
selfish's `self-format.tsv`; re-encoding it here would give a shared judgement a second home, which
is the thing that moved the format tables to selfish in the first place (D200/D201). So the report
carries the number and the reader turns it back into meaning against that one table. This is the
project's own division applied to a new datum: a measurement stays with the measurer, the format is
shared.

Two honest limits are built in, not papered over:

* **The tail layout is unconfirmed at this generation.** `ex_info`/`npdrm` close the header on the
  previous generation, so they are read at `header_size-0x70` / `-0x30`. Whether that holds now is
  exactly what is not known, so the `ex_info` read is self-checked against the admitted `ptype`
  values: land on a real block and the four fields are reported; land on anything else and the
  block is reported "not located" rather than as four numbers that might be the wrong bytes
  (principle 2 - a wrong value that reads as a finding costs more than a withheld one).

* **The structural offsets are cited, not yet projected.** The nine fixed rows reach the C through
  `self_header.gen.h`, generated from selfish's table and drift-gated. The offsets this check uses
  (`0x0C`, the `0x20` segment stride, `header_size-0x70/-0x30`) are the same table's facts but are
  still written in the check body. Promoting them into the generated header so they cannot drift is
  the correct home and a follow-up; it was not done now because the generator path depends on the
  selfish rescaffold that is mid-flight this session (see D282's note), and hand-editing a generated
  file or duplicating a decode table are both worse than a cited constant with this note against it.

**What it cannot reach, by construction, belongs elsewhere.** From an installed title's vantage the
PFS superblock, the pkg entry table and the key/EKPFS blobs are gone - consumed at install - so they
are not on-console-probeable and are confirmed instead by selfish against a real `.pkg` as an oracle
on a dev machine. That is a boundary of the payload vantage, not a gap in this check.

Provenance `OBS_FROM_DERIVED`, no new imports (it reuses `sceKernelOpen/Read/Close/Getdents`, already
declared and imported by the sibling checks). Verified: `selfaudit.c` compiles clean in the host and
target shapes under the full flag set (`-Werror -Wconversion -Wsign-conversion -Wshadow
-Wstrict-prototypes`). Not run end to end - `make host` still fails in another session's
`src/probe/runtime.c` on undeclared module-enumeration calls, unrelated to this file. Formatting: the
new code exceeds the 88-column limit in places, as the neighbouring `metadata-differential` code
already does; `clang-format` is not installed in this environment so `scripts/format.sh` cannot run,
and a whole-file reformat would collide with the concurrent edits. Left for a format sweep.
