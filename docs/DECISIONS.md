# Decision log

Append-only. Every decision that shaped this codebase, with its reasoning, so it can
survive a context compaction and so nobody re-litigates a settled question from first
principles six weeks later.

**Read this file at the start of any working session.**

> **On the Python tools named in early entries.** Entries before D026 refer to `nid.py`,
> `verify.py`, `pretty.py` and `tools/test-tools.sh`. Those are gone - D026 records
> replacing them with the Rust `obscene-tool`, whose subcommands carry the same names.
>
> The references are left as written, because this file is a dated record of what was
> decided when, and editing a decision to match the present destroys the only thing it is
> for. Live documentation points at the current tool; this file points at what existed at
> the time.

**This table is generated.** Edit an entry under `decisions/`, then run
`tools/split-decisions.sh --index obscene`. A number resolves to exactly one file.

| | # | decision | status | date |
|---|---|---|---|---|
| 🟡 | D001 | [Two builds from one source tree](decisions/D001-two-builds-from-one-source-tree.md) | assumed | 2026-08-19 |
| 🟢 | D002 | [Announce before attempting](decisions/D002-announce-before-attempting.md) | decided | 2026-08-19 |
| 🟡 | D003 | [One machine-readable format; colour lives in a script](decisions/D003-one-machine-readable-format-colour.md) | assumed | 2026-08-19 |
| 🟡 | D004 | [Four statuses, not three](decisions/D004-four-statuses-not-three.md) | assumed | 2026-08-19 |
| 🟡 | D005 | [Freestanding C, clang only, no vendor SDK](decisions/D005-freestanding-c-clang-only-no-vendor-sdk.md) | assumed | 2026-08-19 |
| 🟡 | D006 | [Output through the platform's own write, to descriptor 1](decisions/D006-output-through-the-platform-s-own-write.md) | assumed | 2026-08-19 |
| 🟡 | D007 | [Check from the failure side where layouts are unknown](decisions/D007-check-from-the-failure-side-where.md) | assumed | 2026-08-19 |
| 🟢 | D008 | [Nothing is invented; uncertain signatures are omitted](decisions/D008-nothing-is-invented-uncertain.md) | decided | 2026-08-19 |
| 🟡 | D009 | [The section order is one explicit list](decisions/D009-the-section-order-is-one-explicit-list.md) | assumed | 2026-08-19 |
| 🟡 | D010 | [Dependencies are capability bits, not check names](decisions/D010-dependencies-are-capability-bits-not.md) | assumed | 2026-08-19 |
| 🟢 | D011 | [The synthetic error code is 0xDEADBEEF, returned unmodified](decisions/D011-the-synthetic-error-code-is-0xdeadbeef.md) | decided | 2026-08-19 |
| 🟡 | D012 | [The build directory is overridable, and has to be](decisions/D012-the-build-directory-is-overridable-and.md) | assumed | 2026-08-19 |
| 🟡 | D013 | [The entry point returns rather than exiting](decisions/D013-the-entry-point-returns-rather-than.md) | assumed | 2026-08-19 |
| 🟢 | D014 | [Presence and behaviour are separate questions, measured separately](decisions/D014-presence-and-behaviour-are-separate.md) | measured | 2026-08-19 |
| 🟢 | D015 | [A census of absences needs a control in both directions](decisions/D015-a-census-of-absences-needs-a-control-in.md) | decided | 2026-08-19 |
| 🟢 | D016 | [The census lists are fenced off from clang-format](decisions/D016-the-census-lists-are-fenced-off-from.md) | decided | 2026-08-19 |
| 🟢 | D017 | [The C runtime is a first-class section, and it is where positive checks live](decisions/D017-the-c-runtime-is-a-first-class-section.md) | decided | 2026-08-19 |
| 🟡 | D018 | [Prefer documentation over binary extraction when sourcing names](decisions/D018-prefer-documentation-over-binary.md) | assumed | 2026-08-19 |
| 🟢 | D019 | [The diff is the point; the report format is downstream of it](decisions/D019-the-diff-is-the-point-the-report-format.md) | decided | 2026-08-19 |
| 🟡 | D020 | [The tooling has tests, and they run against real reports](decisions/D020-the-tooling-has-tests-and-they-run.md) | assumed | 2026-08-19 |
| 🟡 | D021 | [Symbols carry which console generation they belong to](decisions/D021-symbols-carry-which-console-generation.md) | assumed | 2026-08-19 |
| 🔴 | D022 | [Symbol encoding is a build option, and the suffix is never committed](decisions/D022-symbol-encoding-is-a-build-option-and.md) | superseded | 2026-08-19 |
| 🔴 | D023 | [The suffix is committed and labelled](decisions/D023-the-suffix-is-committed-and-labelled.md) | superseded | 2026-08-19 |
| 🟢 | D024 | [The NID chain is pinned by a published test vector, not by prose](decisions/D024-the-nid-chain-is-pinned-by-a-published.md) | decided | 2026-08-19 |
| 🟢 | D025 | [Unknown format constants are measured from a loader, not read from a parser](decisions/D025-unknown-format-constants-are-measured.md) | measured | 2026-08-19 |
| 🟢 | D026 | [The tooling is Rust, in one binary](decisions/D026-the-tooling-is-rust-in-one-binary.md) | decided | 2026-08-19 |
| 🟢 | D027 | [The vendor dynamic segment is built on every module build, not behind a flag](decisions/D027-the-vendor-dynamic-segment-is-built-on.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D028 | [The tag derivation is a command that runs on every build, not a paragraph](decisions/D028-the-tag-derivation-is-a-command-that.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D029 | [Three loadable segments, because a fourth is refused](decisions/D029-three-loadable-segments-because-a.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D030 | [The string table is the first entry in the dynamic table, and everything that names something comes last](decisions/D030-the-string-table-is-the-first-entry-in.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D031 | [The module is built with hidden visibility and symbolic binding](decisions/D031-the-module-is-built-with-hidden.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D032 | [The report tries more than one way out, and says which one it used](decisions/D032-the-report-tries-more-than-one-way-out.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D033 | [Which library each import comes from is published by the host build](decisions/D033-which-library-each-import-comes-from-is.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D034 | [A finished module has no section headers](decisions/D034-a-finished-module-has-no-section-headers.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D035 | [The entry point ends the process. It does not return](decisions/D035-the-entry-point-ends-the-process-it.md) | decided | ~2026-08-19..08-20 |
| ⚪ | D036 | [`DT_SCE_JMPREL` and `DT_SCE_PLTRELSZ` were swapped, and the derivation could not have caught it](decisions/D036-dt-sce-jmprel-and-dt-sce-pltrelsz-were.md) | unrecorded | ~2026-08-19..08-20 |
| 🟢 | D037 | [`puts` is an output channel, tried before `write`](decisions/D037-puts-is-an-output-channel-tried-before.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D038 | [Libraries and modules declare version 1, and they pack it differently](decisions/D038-libraries-and-modules-declare-version-1.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D039 | [Imported symbols are rewritten to `STT_FUNC`, and this is what makes a report come out](decisions/D039-imported-symbols-are-rewritten-to-stt.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D040 | [Checks can be excluded at build time, and the default list is empty](decisions/D040-checks-can-be-excluded-at-build-time.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D041 | [The report is drawn as well as written](decisions/D041-the-report-is-drawn-as-well-as-written.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D042 | [The display owns the main video output, and the video checks yield to it](decisions/D042-the-display-owns-the-main-video-output.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D043 | [The platform is enumerated, not only asked about](decisions/D043-the-platform-is-enumerated-not-only.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D044 | [Every check records where its expectation came from](decisions/D044-every-check-records-where-its.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D045 | [Checks are chosen from what emulators have actually got wrong](decisions/D045-checks-are-chosen-from-what-emulators.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D046 | [An intermittent failure is only a finding once it has been counted](decisions/D046-an-intermittent-failure-is-only-a.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D047 | [A function is asked whether it reads its arguments, separately from whether it is right](decisions/D047-a-function-is-asked-whether-it-reads.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D048 | [The screen pages through every check, and cycles rather than waiting for input](decisions/D048-the-screen-pages-through-every-check.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D049 | [`obscene-tool crack` recovers names from NIDs by guessing, and says what a miss is worth](decisions/D049-obscene-tool-crack-recovers-names-from.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D050 | [The orchestration is scripts in the repository, not steps somebody remembers](decisions/D050-the-orchestration-is-scripts-in-the.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D051 | [Thirty census names promoted to behavioural checks, all of them settled by a document](decisions/D051-thirty-census-names-promoted-to.md) | decided | ~2026-08-19..08-20 |
| 🔴 | D052 | [`div` and `ldiv` were checked, then withdrawn to the census](decisions/D052-div-and-ldiv-were-checked-then.md) | withdrawn | ~2026-08-19..08-20 |
| ⚪ | D053 | [The census had been carrying a fragment of a symbol name, and now it cannot](decisions/D053-the-census-had-been-carrying-a-fragment.md) | unrecorded | ~2026-08-19..08-20 |
| 🟢 | D054 | [The emulators live in `<emulators>`, source and all, outside this repository](decisions/D054-the-emulators-live-in-emulators-source.md) | decided | ~2026-08-19..08-20 |
| ⚪ | D055 | [The NID implementation now has an independent 42,010-name confirmation](decisions/D055-the-nid-implementation-now-has-an.md) | unrecorded | ~2026-08-19..08-20 |
| ⚪ | D056 | [A value check whose every expected answer is zero cannot tell a stub from a success](decisions/D056-a-value-check-whose-every-expected.md) | unrecorded | ~2026-08-19..08-20 |
| 🟢 | D057 | [`libScePosix` is checked, and it is the best provenance available without a console](decisions/D057-libsceposix-is-checked-and-it-is-the.md) | decided | ~2026-08-19..08-20 |
| ⚪ | D058 | [A check that calls a second library must test that symbol's address itself](decisions/D058-a-check-that-calls-a-second-library.md) | unrecorded | ~2026-08-19..08-20 |
| 🟢 | D059 | [A third kind of record: measurements, which assert nothing](decisions/D059-a-third-kind-of-record-measurements.md) | decided | ~2026-08-19..08-20 |
| 🟡 | D060 | [Which clock counts what is established by experiment, not assumed](decisions/D060-which-clock-counts-what-is-established.md) | assumed | ~2026-08-19..08-20 |
| 🟢 | D061 | [Three loaders, not one, and the second and third each found something the first could not](decisions/D061-three-loaders-not-one-and-the-second.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D062 | [The console generation is a build option, because the loaders disagree and all of them are right](decisions/D062-the-console-generation-is-a-build.md) | decided | ~2026-08-19..08-20 |
| ⚪ | D063 | [The NID implementation agrees with an independent one on 78,372 pairs out of 78,372](decisions/D063-the-nid-implementation-agrees-with-an.md) | unrecorded | ~2026-08-19..08-20 |
| 🟢 | D064 | [A correction to D063: fpPS4's table is derived from ps4libdoc, not independent of it](decisions/D064-a-correction-to-d063-fpps4-s-table-is.md) | derived | ~2026-08-19..08-20 |
| 🟢 | D065 | [The gate could not fail, and that is why everything else survived](decisions/D065-the-gate-could-not-fail-and-that-is-why.md) | done | ~2026-08-19..08-20 |
| ⚪ | D066 | [The rule from D058 is enforced by a script now, because writing it down did nothing](decisions/D066-the-rule-from-d058-is-enforced-by-a.md) | unrecorded | ~2026-08-19..08-20 |
| 🟢 | D067 | [The headline number is a frontier, not a sum](decisions/D067-the-headline-number-is-a-frontier-not-a.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D068 | [`018-relational`: properties instead of values, aimed where no document reaches](decisions/D068-018-relational-properties-instead-of.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D069 | [Volatile facts are generated; static facts are written. The distinction is the whole fix](decisions/D069-volatile-facts-are-generated-static.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D070 | [Documentation that names a command is checked against the commands that exist](decisions/D070-documentation-that-names-a-command-is.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D071 | [The orchestration scripts are `sh`. PowerShell was solving one environment variable](decisions/D071-the-orchestration-scripts-are-sh.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D072 | [`obscene-tool consensus`: agreement between implementations, used where hardware is not available](decisions/D072-obscene-tool-consensus-agreement.md) | hardware | ~2026-08-19..08-20 |
| 🟢 | D073 | [Five more relations, and the one with the worst failure mode](decisions/D073-five-more-relations-and-the-one-with.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D074 | [`derived` is a fifth provenance, because the FreeBSD upgrade was not the one the backlog described](decisions/D074-derived-is-a-fifth-provenance-because.md) | derived | ~2026-08-19..08-20 |
| 🟢 | D075 | [The current generation's graphics interface is censused, and its first run is the best evidence yet that presence means little](decisions/D075-the-current-generation-s-graphics.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D076 | [Condition variables and barriers: the operations that cannot block were available all along](decisions/D076-condition-variables-and-barriers-the.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D077 | [The wakeup path is testable, and the timeout was never the thing that was needed](decisions/D077-the-wakeup-path-is-testable-and-the.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D078 | [The census now states its own limit, in the report, where a coverage figure gets quoted](decisions/D078-the-census-now-states-its-own-limit-in.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D079 | [`scripts/repeat.sh`: an intermittent fault needs a denominator](decisions/D079-scripts-repeat-sh-an-intermittent-fault.md) | decided | ~2026-08-19..08-20 |
| ⚪ | D080 | [Kyty cannot emit a report, and the reason is structural rather than a missing flag](decisions/D080-kyty-cannot-emit-a-report-and-the.md) | unrecorded | ~2026-08-19..08-20 |
| 🟢 | D081 | [`130-layout`: the instrument half. Bytes recorded, nothing interpreted](decisions/D081-130-layout-the-instrument-half-bytes.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D082 | [Two more libkernel checks, and one of them found a real fault](decisions/D082-two-more-libkernel-checks-and-one-of.md) | decided | ~2026-08-19..08-20 |
| ⚪ | D083 | [Why craziiEmu resolves nothing: two bugs, one fixed, and the earlier guess was wrong](decisions/D083-why-craziiemu-resolves-nothing-two-bugs.md) | unrecorded | ~2026-08-19..08-20 |
| ⚪ | D084 | [obSCEne may consult other projects. orbistoun may not. The asymmetry is load-bearing](decisions/D084-obscene-may-consult-other-projects.md) | unrecorded | ~2026-08-19..08-20 |
| ⚪ | D085 | [Before believing a setting works, set it to something that must visibly break](decisions/D085-before-believing-a-setting-works-set-it.md) | unrecorded | ~2026-08-19..08-20 |
| ⚪ | D086 | [Guest output may arrive on stderr, and reading only stdout would have looked like silence](decisions/D086-guest-output-may-arrive-on-stderr-and.md) | unrecorded | ~2026-08-19..08-20 |
| ⚪ | D087 | [`000-boot/stack-alignment`, offered by the orbistoun side and worth taking](decisions/D087-000-boot-stack-alignment-offered-by-the.md) | unrecorded | ~2026-08-19..08-20 |
| 🟢 | D088 | [`140-oracle`: asking the platform what it knows, and recording what it returns](decisions/D088-140-oracle-asking-the-platform-what-it.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D089 | [`150-memory-map`: the map walked, with the walk's own hypothesis checked as it goes](decisions/D089-150-memory-map-the-map-walked-with-the.md) | decided | ~2026-08-19..08-20 |
| 🟢 | D090 | [The compatibility table is generated from reports, because a hand-written one would be stale by tomorrow](decisions/D090-the-compatibility-table-is-generated.md) | decided | ~2026-08-19..08-20 |
| ⚪ | D091 | [fpPS4 builds on a stable compiler, and reads our module perfectly](decisions/D091-fpps4-builds-on-a-stable-compiler-and.md) | unrecorded | ~2026-08-19..08-20 |
| 🟢 | D092 | [Screenshots of each loader, captured on the summary screen](decisions/D092-screenshots-of-each-loader-captured-on.md) | decided | ~2026-08-19..08-20 |
| ⚪ | D093 | [What the current-generation emulators implement that obSCEne does not touch](decisions/D093-what-the-current-generation-emulators.md) | unrecorded | ~2026-08-19..08-20 |
| ⚪ | D094 | [Emulators are built from source. Running a binary that does not match the source read is a method error](decisions/D094-emulators-are-built-from-source-running.md) | unrecorded | 2026-08-20 |
| 🟡 | D095 | [Loaders that share a codebase count once. The compatibility table records lineage](decisions/D095-loaders-that-share-a-codebase-count.md) | assumed | ~2026-08-20..08-26 |
| 🟡 | D096 | [The blind prober calls what it cannot describe, and that is not a breach of D008](decisions/D096-the-blind-prober-calls-what-it-cannot.md) | assumed | ~2026-08-20..08-26 |
| 🟡 | D097 | [Blame for a loader failure is not assigned without a control build](decisions/D097-blame-for-a-loader-failure-is-not.md) | assumed | ~2026-08-20..08-26 |
| 🟢 | D098 | [The missing program headers come from `link/module.ld`, not from the link mode or the packaging. Two hypotheses were tested and both were wrong](decisions/D098-the-missing-program-headers-come-from.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D099 | [Two loadable segments, not three. A third was silently not mapped, and that was the crash](decisions/D099-two-loadable-segments-not-three-a-third.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D100 | [`DT_SCE_NEEDED_MODULE` is not enough. A loader keys its implementations on the ordinary `DT_NEEDED` filename, and obSCEne emits none](decisions/D100-dt-sce-needed-module-is-not-enough-a.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D101 | [obSCEne runs on a second loader. `DT_NEEDED` was the whole of what was missing](decisions/D101-obscene-runs-on-a-second-loader-dt.md) | hardware | ~2026-08-20..08-26 |
| 🟡 | D102 | [The command protocol is specified before it is implemented, and the captured exchanges are part of the contract](decisions/D102-the-command-protocol-is-specified.md) | assumed | ~2026-08-20..08-26 |
| 🟢 | D103 | [The hash is confirmed against a second, independent corpus - 281 of 281. And obSCEne will not discover a symbol it has never heard of, ever](decisions/D103-the-hash-is-confirmed-against-a-second.md) | confirmed | ~2026-08-20..08-26 |
| 🟢 | D104 | [The report is written to a file as well, not instead. And the backend is chosen by which file is compiled](decisions/D104-the-report-is-written-to-a-file-as-well.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D105 | [166,960 names and 1,130,742 unnamed identifiers, from eleven sources across 23 firmware versions](decisions/D105-166-960-names-and-1-130-742-unnamed.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D106 | [The driver, and the one thing that lives only at this end](decisions/D106-the-driver-and-the-one-thing-that-lives.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D107 | [The console socket backend is implemented, and proven inside shadPS4 with no hardware](decisions/D107-the-console-socket-backend-is.md) | hardware | ~2026-08-20..08-26 |
| ⚪ | D108 | [A report and a corpus are different artifacts, and only the corpus carries origin](decisions/D108-a-report-and-a-corpus-are-different.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D109 | [The GPU is probed by executing shaders and reading the result bits, not by calling an API](decisions/D109-the-gpu-is-probed-by-executing-shaders.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D110 | [Generation detection exists and gates nothing. `obs_detected_generation()` has no callers](decisions/D110-generation-detection-exists-and-gates.md) | unrecorded | ~2026-08-20..08-26 |
| 🟢 | D111 | [One source, both generations: the display takes whichever video-out pair resolved](decisions/D111-one-source-both-generations-the-display.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D112 | [GPU device selection prefers real silicon, and the device type is recorded as gradable provenance](decisions/D112-gpu-device-selection-prefers-real.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D113 | [The unary kernel set is widened to the rest of the transcendentals, and the input vector to the edges they strain](decisions/D113-the-unary-kernel-set-is-widened-to-the.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D114 | [The blind prober covered 1% of the census, and pointing it at the rest immediately called a variable](decisions/D114-the-blind-prober-covered-1-of-the.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D115 | [A `gpu` protocol verb dispatches a compiled-in kernel over the socket - the interactive oracle, with no rebuild](decisions/D115-a-gpu-protocol-verb-dispatches-a.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D116 | [A reference oracle, so a GPU diff says *which instructions are approximate* rather than merely that two runs differ](decisions/D116-a-reference-oracle-so-a-gpu-diff-says.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D117 | [A GPU ISA surface census, so "probe every GPU op" has a statement of what every GPU op is](decisions/D117-a-gpu-isa-surface-census-so-probe-every.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D118 | [Closing the three GLSL-reachable gaps the census named, and finding a denormal difference by doing it](decisions/D118-closing-the-three-glsl-reachable-gaps.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D119 | [Integer/bit breadth: a second GPU surface the float census did not map, and the kernels for its first four operations](decisions/D119-integer-bit-breadth-a-second-gpu.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D120 | [Controlled-ISA: pinning the fast SFU path against the correctly-rounded one, through the precision the SPIR-V asks for](decisions/D120-controlled-isa-pinning-the-fast-sfu.md) | derived | ~2026-08-20..08-26 |
| ⚪ | D121 | [Console generation is observed, never asserted from presence](decisions/D121-console-generation-is-observed-never.md) | unrecorded | ~2026-08-20..08-26 |
| 🟢 | D122 | [Protocol completion: the `blob`/`run`/`reset` verbs, with the escape hatch off unless a build asks for it](decisions/D122-protocol-completion-the-blob-run-reset.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D123 | [A golden GPU corpus and a regression check, so a change to a kernel's output is caught even where the reference stays silent](decisions/D123-a-golden-gpu-corpus-and-a-regression.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D124 | [More execution breadth: bitfield ops, an FTZ probe, and a bit-pattern input vector for the kernels that work on bits](decisions/D124-more-execution-breadth-bitfield-ops-an.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D125 | [A gate is not trusted until it has been shown to reject something](decisions/D125-a-gate-is-not-trusted-until-it-has-been.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D126 | [The command socket, and why the console backend is deliberately empty](decisions/D126-the-command-socket-and-why-the-console.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D127 | [obSCEne's display targets the previous generation's video-out interface. That is why it draws nothing on a current-generation loader](decisions/D127-obscene-s-display-targets-the-previous.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D128 | [30,610 names mined from seven emulators. The hash reproduces 30,137 of them, and every exception is the source's naming, not the hash](decisions/D128-30-610-names-mined-from-seven-emulators.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D129 | [`call` and `read` are implemented, proven live over a socket](decisions/D129-call-and-read-are-implemented-proven.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D130 | [The census is 5,909 symbols. Growing it turned "present" from a weak signal into a measurement of the loader](decisions/D130-the-census-is-5-909-symbols-growing-it.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D131 | [GPU kernels are authored in bulk from a generated list, and the GPU capability is gated](decisions/D131-gpu-kernels-are-authored-in-bulk-from-a.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D132 | [A serving build listens first and runs the suite on demand. The report streams over the socket. A named Deck target exists](decisions/D132-a-serving-build-listens-first-and-runs.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D133 | [A consumer's strict decoder found a defective fixture the checker had passed. Both are fixed](decisions/D133-a-consumer-s-strict-decoder-found-a.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D134 | [Multi-operand kernels sweep the operand cross-product, in a new record that leaves the unary one untouched](decisions/D134-multi-operand-kernels-sweep-the-operand.md) | derived | ~2026-08-20..08-26 |
| ⚪ | D135 | [The non-platform corpus is the console's own software, and it is a different target rather than a worthless one](decisions/D135-the-non-platform-corpus-is-the-console.md) | unrecorded | ~2026-08-20..08-26 |
| 🟡 | D136 | [352 libraries and 35,518 imports load and run. The ceiling was far higher than assumed](decisions/D136-352-libraries-and-35-518-imports-load.md) | assumed | ~2026-08-20..08-26 |
| 🟢 | D137 | [The GPU corpus diff, so a hardware run ends in a gap list rather than a pile of records](decisions/D137-the-gpu-corpus-diff-so-a-hardware-run.md) | hardware | ~2026-08-20..08-26 |
| 🟢 | D138 | [A symbol can be imported with no name at all. The identifier is the import; the name only ever existed to compute it](decisions/D138-a-symbol-can-be-imported-with-no-name.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D139 | [A platform HUD replaces the static tagline: every system fact, value or `unknown`](decisions/D139-a-platform-hud-replaces-the-static.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D140 | [A resolved address proves nothing on a loader that stubs what it cannot resolve. The display learned this the hard way](decisions/D140-a-resolved-address-proves-nothing-on-a.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D141 | [Two loose ends closed: the generation had two inferences, and the generated censuses had no gate](decisions/D141-two-loose-ends-closed-the-generation.md) | derived | ~2026-08-20..08-26 |
| ⚪ | D142 | [Seventeen decisions were sharing thirteen numbers, and nothing was checking. The log now gates its own numbering](decisions/D142-seventeen-decisions-were-sharing.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D143 | [The corpus records what it was mined from, so the half of census drift nothing was watching becomes visible](decisions/D143-the-corpus-records-what-it-was-mined.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D144 | [A timeout is not a hang, and two timeouts are. The sweep now measures the difference instead of asking an operator to assert it](decisions/D144-a-timeout-is-not-a-hang-and-two.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D145 | [An exclusion entry with no `/` names a whole section, because a loader can fail a layer rather than a check](decisions/D145-an-exclusion-entry-with-no-names-a.md) | unrecorded | ~2026-08-20..08-26 |
| 🟢 | D146 | [Pack/unpack conversions, and the per-kernel input scheme they needed](decisions/D146-pack-unpack-conversions-and-the-per.md) | derived | ~2026-08-20..08-26 |
| ⚪ | D147 | [The three letters in the middle of the name are marked wherever the project renders it](decisions/D147-the-three-letters-in-the-middle-of-the.md) | unrecorded | ~2026-08-20..08-26 |
| 🟢 | D148 | [`gpustats`: the diff as a distance, so the approximation is a number, not a yes/no](decisions/D148-gpustats-the-diff-as-a-distance-so-the.md) | derived | ~2026-08-20..08-26 |
| ⚪ | D149 | [A censused library is not a load-time dependency, and saying it is stops obSCEne loading on current shadPS4 at all](decisions/D149-a-censused-library-is-not-a-load-time.md) | unrecorded | ~2026-08-20..08-26 |
| 🟢 | D150 | [The hardware day made turnkey: one analysis command and a runbook, so a Deck corpus needs no new thinking](decisions/D150-the-hardware-day-made-turnkey-one.md) | hardware | ~2026-08-20..08-26 |
| ⚪ | D151 | [The blind prober does not belong on the host build, and the harness did not scale to the corpus. Both were found by running it](decisions/D151-the-blind-prober-does-not-belong-on-the.md) | unrecorded | ~2026-08-20..08-26 |
| 🟢 | D152 | [The Gnm command-building API probe: the console GPU's own calls, the other GPU axis - and a correction to having dismissed it](decisions/D152-the-gnm-command-building-api-probe-the.md) | derived | ~2026-08-20..08-26 |
| 🟢 | D153 | [Scoping the Gnm execution axis: the GCN shader is not the blocker; the input struct layouts are](decisions/D153-scoping-the-gnm-execution-axis-the-gcn.md) | derived | ~2026-08-20..08-26 |
| ⚪ | D154 | [Three threading contract probes, added at the sibling project's request](decisions/D154-three-threading-contract-probes-added.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D155 | [One source, as many binaries as the targets need. Single-binary was never the goal and is not a constraint to design around](decisions/D155-one-source-as-many-binaries-as-the.md) | unrecorded | ~2026-08-20..08-26 |
| 🟢 | D156 | [The compatibility table ignored the census control that was in every report, and presented a void count beside a measured one](decisions/D156-the-compatibility-table-ignored-the.md) | measured | ~2026-08-20..08-26 |
| ⚪ | D157 | [The runtime census exists, and its first run settled what the address census could not](decisions/D157-the-runtime-census-exists-and-its-first.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D158 | [Two relational checks had never run anywhere, and the report said so in words that read as a platform limitation](decisions/D158-two-relational-checks-had-never-run.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D159 | [The two new threading relations spin for their child rather than joining it](decisions/D159-the-two-new-threading-relations-spin.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D160 | [The flag sweep: measurable without a console, and the dump answered the wrong call](decisions/D160-the-flag-sweep-measurable-without-a.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D161 | [`bulk-sweep.sh` resumes, because the round budget is the binding constraint and not the size of the surface](decisions/D161-bulk-sweep-sh-resumes-because-the-round.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D162 | [A round that produced nothing is retried once](decisions/D162-a-round-that-produced-nothing-is.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D163 | [Correction: the blind prober walks the corpus, not the curated census](decisions/D163-correction-the-blind-prober-walks-the.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D164 | [The blind prober's classifier keeps privileging `0x8002`, and the facility table stays out of the probe](decisions/D164-the-blind-prober-s-classifier-keeps.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D165 | [The command socket takes a session secret, generated per startup and displayed](decisions/D165-the-command-socket-takes-a-session.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D166 | [A test asserted the opposite of what `sceKernelClearEventFlag` does, and the stub agreed](decisions/D166-a-test-asserted-the-opposite-of-what.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D167 | [The blind prober against a current-generation loader: the two emulators are opposites, and twelve functions hand the guest a host address](decisions/D167-the-blind-prober-against-a-current.md) | unrecorded | ~2026-08-20..08-26 |
| 🟡 | D168 | [A comment inside a table row's braces made a check invisible to every gate, and nothing failed](decisions/D168-a-comment-inside-a-table-row-s-braces.md) | assumed | ~2026-08-20..08-26 |
| ⚪ | D169 | [The provenance ladder gains a rung: `implementations`](decisions/D169-the-provenance-ladder-gains-a-rung.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D170 | [Anchored prose: a document may state one checkable fact about the source beside the passage that depends on it](decisions/D170-anchored-prose-a-document-may-state-one.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D171 | [fpPS4 writes a pointer-sized semaphore handle through an `int *`, and the check that found it described the wrong thing](decisions/D171-fpps4-writes-a-pointer-sized-semaphore.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D172 | [The exclusion list moved from build time to run time, so one ELF runs on every loader](decisions/D172-the-exclusion-list-moved-from-build.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D173 | [The module now carries a `DT_INIT`, because a loader may call one and ours pointed at the ELF header](decisions/D173-the-module-now-carries-a-dt-init.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D174 | [The screen said nothing about what it was doing, and the first fix for that made the probe worse](decisions/D174-the-screen-said-nothing-about-what-it.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D175 | [`e_type` was `0xFE18` - the shared-library type - and the constants were named backwards](decisions/D175-e-type-was-0xfe18-the-shared-library.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D176 | [A loader can be patched to let the probe run, and doing so measures the patch](decisions/D176-a-loader-can-be-patched-to-let-the.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D177 | [The mutex type constants are one-based and are not the POSIX values](decisions/D177-the-mutex-type-constants-are-one-based.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D178 | [The shell's process id is not the loader, and the runner had already learnt that once](decisions/D178-the-shell-s-process-id-is-not-the.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D179 | [Four shapes, two axes, and the filename now says which](decisions/D179-four-shapes-two-axes-and-the-filename.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D180 | [The loading mechanism is coverage surface, so each one is an artifact](decisions/D180-the-loading-mechanism-is-coverage.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D181 | [Runtime resume can lose twenty thousand measurements and still report success](decisions/D181-runtime-resume-can-lose-twenty-thousand.md) | unrecorded | ~2026-08-20..08-26 |
| 🟢 | D182 | [The container format is derived the way NIDs already are: binaries outside, provenance-headed data inside](decisions/D182-the-container-format-is-derived-the-way.md) | derived | ~2026-08-20..08-26 |
| ⚪ | D183 | [A shell exists on the real target after all, and the protocol survives it for a better reason](decisions/D183-a-shell-exists-on-the-real-target-after.md) | unrecorded | ~2026-08-20..08-26 |
| 🟢 | D184 | [Hardware is registered, and its capabilities are measured on every use rather than stored](decisions/D184-hardware-is-registered-and-its.md) | hardware | ~2026-08-20..08-26 |
| ⚪ | D185 | [`mkself` works, and the loader that rejected it first is why](decisions/D185-mkself-works-and-the-loader-that.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D186 | [The module version is per library, and declaring one value for all of them was why nothing appeared on screen](decisions/D186-the-module-version-is-per-library-and.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D187 | ["The display accepted it" and "a frame reached the screen" are different facts, and the report only carried the first](decisions/D187-the-display-accepted-it-and-a-frame.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D188 | [Retail titles render and a CPU-drawn framebuffer does not, and the difference is who wrote the pixels](decisions/D188-retail-titles-render-and-a-cpu-drawn.md) | unrecorded | ~2026-08-20..08-26 |
| 🟢 | D189 | [The half of `hardware.rs` that talked to a console now lives in a shared crate, and this project depends on it by path](decisions/D189-the-half-of-hardware-rs-that-talked-to.md) | hardware | ~2026-08-20..08-26 |
| ⚪ | D190 | [The tool fetches the report off the console itself, rather than telling somebody to go and get it](decisions/D190-the-tool-fetches-the-report-off-the.md) | unrecorded | ~2026-08-20..08-26 |
| 🟢 | D191 | [A check must fail to return on two *consecutive* runs before it is skipped](decisions/D191-a-check-must-fail-to-return-on-two.md) | measured | ~2026-08-20..08-26 |
| 🟢 | D192 | [A project that reads the others is not another vote, and prosper says so in its own comments](decisions/D192-a-project-that-reads-the-others-is-not.md) | hardware | ~2026-08-20..08-26 |
| ⚪ | D193 | [obSCEne writes a dynamic table no retail current-generation module uses, and six real dumps say so](decisions/D193-obscene-writes-a-dynamic-table-no.md) | unrecorded | ~2026-08-20..08-26 |
| 🟢 | D194 | [`grep -q` reading a Windows program deadlocks the harness, and it hid as a timeout bug for hours](decisions/D194-grep-q-reading-a-windows-program.md) | measured | ~2026-08-20..08-26 |
| ⚪ | D195 | [prosper is the sixth loader, the only headless one, and the first to have ground truth to check us against](decisions/D195-prosper-is-the-sixth-loader-the-only.md) | unrecorded | ~2026-08-20..08-26 |
| ⚪ | D196 | [fpPS4 does not crash on an unimplemented import - it sleeps forever, and that is one line](decisions/D196-fpps4-does-not-crash-on-an.md) | unrecorded | ~2026-08-20..08-26 |
| 🟢 | D197 | [A consensus across generations is not a consensus, and mixing them destroyed the signal](decisions/D197-a-consensus-across-generations-is-not-a.md) | hardware | ~2026-08-20..08-26 |
| 🟢 | D198 | [`verify.sh` takes fifty minutes, none of it work, and D012's rule had one more place to apply](decisions/D198-verify-sh-takes-fifty-minutes-none-of.md) | measured | ~2026-08-20..08-26 |
| ⚪ | D199 | [Five scripts were still driving a build environment that had been deleted that morning](decisions/D199-five-scripts-were-still-driving-a-build.md) | unrecorded | 2026-08-26 |
| ⚪ | D200 | [The file formats moved to `selfish`, and this repository now depends on them](decisions/D200-the-file-formats-moved-to-selfish-and.md) | unrecorded | ~2026-08-26..08-27 |
| ⚪ | D201 | [The migration found four things wrong, and three of them were wrong here](decisions/D201-the-migration-found-four-things-wrong.md) | unrecorded | ~2026-08-26..08-27 |
| ⚪ | D202 | [A mined-corpus name that a section wants to call is declared in the section, not in `platform.h`](decisions/D202-a-mined-corpus-name-that-a-section.md) | unrecorded | ~2026-08-26..08-27 |
| ⚪ | D203 | [`105-record`, not `170-record`: the section numbers carry the layering and the report contract enforces it](decisions/D203-105-record-not-170-record-the-section.md) | unrecorded | ~2026-08-26..08-27 |
| ⚪ | D204 | [The layout verdict counted non-zero bytes while its dump counted changed bytes](decisions/D204-the-layout-verdict-counted-non-zero.md) | unrecorded | ~2026-08-26..08-27 |
| 🟢 | D205 | [First contact with a console, and the answer arrived in ninety seconds: elfldr does not resolve our imports](decisions/D205-first-contact-with-a-console-and-the.md) | hardware | 2026-08-27 |
| ⚪ | D206 | [The console runs our payload - the blocker was 16 KB pages, and the loader blocks direct syscalls and passes a bootstrap](decisions/D206-the-console-runs-our-payload-the.md) | unrecorded | 2026-08-27 |
| ⚪ | D207 | [The backtrace-echo readout is defeated by log caching, and the signal ladder is the trustworthy channel](decisions/D207-the-backtrace-echo-readout-is-defeated.md) | unrecorded | ~2026-08-27 |
| ⚪ | D208 | [The payload ABI is fully mapped, and the frontier is precise: word 0 is not a working resolver in the sandbox](decisions/D208-the-payload-abi-is-fully-mapped-and-the.md) | unrecorded | 2026-08-27 |
| 🟢 | D209 | [obSCEne runs on a real PS5 and produces a report with hardware provenance - the first in the project's history](decisions/D209-obscene-runs-on-a-real-ps5-and-produces.md) | hardware | 2026-08-27 |
| ⚪ | D210 | [Video is reachable from the payload, but the display is owned by the system](decisions/D210-video-is-reachable-from-the-payload-but.md) | unrecorded | 2026-08-27 |
| 🟢 | D211 | [A general payload crt0 resolves obSCEne's imports on-device - proven on hardware, and the artifact that moves into selfish](decisions/D211-a-general-payload-crt0-resolves-obscene.md) | hardware | 2026-08-27 |
| ⚪ | D212 | [obSCEne is built by selfish, deployed by prosperous, and runs on the console - the toolchain is real](decisions/D212-obscene-is-built-by-selfish-deployed-by.md) | unrecorded | 2026-08-27 |
| ⚪ | D213 | [The payload path split along selfish's charter: format knowledge in selfish, runtime in obSCEne - and multi-library resolution works](decisions/D213-the-payload-path-split-along-selfish-s.md) | unrecorded | 2026-08-27 |
| 🔴 | D214 | [Rendering is blocked by process context, not by the toolchain - the injected process has no display to own](decisions/D214-rendering-is-blocked-by-process-context.md) | blocked | 2026-08-27 |
| ⚪ | D215 | [selfish is made the explicit, required build path - in the Makefile, in CI, and through a wired `make pkg`](decisions/D215-selfish-is-made-the-explicit-required.md) | unrecorded | ~2026-08-27..08-29 |
| 🟡 | D216 | [obSCEne's end of "the real way" is built and waiting on one selfish command - install and launch via prosperous, into a foreground context](decisions/D216-obscene-s-end-of-the-real-way-is-built.md) | assumed | ~2026-08-27..08-29 |
| 🟢 | D217 | [Import libraries are numbered from zero, needed modules from one](decisions/D217-import-libraries-are-numbered-from-zero.md) | decided | 2026-08-29 |
| 🟢 | D218 | [`MIN_DEFINES`, and what the importless control turned out not to prove](decisions/D218-min-defines-and-what-the-importless.md) | decided | 2026-08-29 |
| ⚪ | D219 | [The process parameters carry three structures, and the libc one is not optional](decisions/D219-the-process-parameters-carry-three.md) | unrecorded | ~2026-08-29..08-30 |
| ⚪ | D220 | [The bundled modules are built here, never copied](decisions/D220-the-bundled-modules-are-built-here.md) | unrecorded | ~2026-08-29..08-30 |
| ⚪ | D221 | [`--kind`, replacing `--fixed`, and the library id base that goes with it](decisions/D221-kind-replacing-fixed-and-the-library-id.md) | unrecorded | ~2026-08-29..08-30 |
| ⚪ | D222 | [What a bundled library must carry, found one refusal at a time](decisions/D222-what-a-bundled-library-must-carry-found.md) | unrecorded | ~2026-08-29..08-30 |
| ⚪ | D223 | [The title id lives in the content id, and a fresh one routes around a stuck title](decisions/D223-the-title-id-lives-in-the-content-id.md) | unrecorded | ~2026-08-29..08-30 |
| ⚪ | D224 | [The bundled stubs must not be named after a library, and the guard was checking the wrong list](decisions/D224-the-bundled-stubs-must-not-be-named.md) | unrecorded | ~2026-08-29..08-30 |
| ⚪ | D225 | [The report goes to the system log first, because a title has no descriptor](decisions/D225-the-report-goes-to-the-system-log-first.md) | unrecorded | ~2026-08-29..08-30 |
| ⚪ | D226 | [An eboot may not require more libraries than a title can load](decisions/D226-an-eboot-may-not-require-more-libraries.md) | unrecorded | ~2026-08-29..08-30 |
| 🟢 | D227 | [The harness needs two libraries; the other 350 are measurements](decisions/D227-the-harness-needs-two-libraries-the.md) | measured | ~2026-08-29..08-30 |
| ⚪ | D228 | [The eboot does not link the census, and the census says so](decisions/D228-the-eboot-does-not-link-the-census-and.md) | unrecorded | ~2026-08-29..08-30 |
| ⚪ | D229 | [The census resolves at run time, so an unloadable library is a finding](decisions/D229-the-census-resolves-at-run-time-so-an.md) | unrecorded | ~2026-08-29..08-30 |
| ⚪ | D230 | [The generation probe asked by linking, and could have killed what it was probing](decisions/D230-the-generation-probe-asked-by-linking.md) | unrecorded | ~2026-08-29..08-30 |
| ⚪ | D231 | [`EBOOT_KIND`, so the same eboot can be put through an emulator first](decisions/D231-eboot-kind-so-the-same-eboot-can-be-put.md) | unrecorded | ~2026-08-29..08-30 |
| ⚪ | D232 | [The run-time census needs a control, because "no" has two meanings](decisions/D232-the-run-time-census-needs-a-control.md) | unrecorded | ~2026-08-29..08-30 |
| ⚪ | D233 | [The system log is a second destination, not a candidate channel](decisions/D233-the-system-log-is-a-second-destination.md) | unrecorded | ~2026-08-29..08-30 |
| ⚪ | D235 | ["Not present on this platform" was a claim the harness could not support](decisions/D235-not-present-on-this-platform-was-a.md) | unrecorded | ~2026-08-29..08-30 |
| 🟢 | D236 | [What a sweep proved crashes belongs in the repository, not in a build directory](decisions/D236-what-a-sweep-proved-crashes-belongs-in.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D237 | [The report's file mode is set for the reader, and the reader is never the writer](decisions/D237-the-report-s-file-mode-is-set-for-the.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D238 | [`/download0` exists only while the title does](decisions/D238-download0-exists-only-while-the-title.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D239 | [A run-time module load does not repair an import the loader left unresolved](decisions/D239-a-run-time-module-load-does-not-repair.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D240 | [`import` records, because the census cannot see this program's own imports](decisions/D240-import-records-because-the-census.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D241 | [An identity word that decodes cleanly is not a well-formed identity word](decisions/D241-an-identity-word-that-decodes-cleanly.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D242 | [`sce_process_param` was almost entirely empty, and one field's comment was evidence-free](decisions/D242-sce-process-param-was-almost-entirely.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D243 | [A fact held only in a generated file is held nowhere](decisions/D243-a-fact-held-only-in-a-generated-file-is.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D244 | [Declaring a real SDK version stops `sceKernelDlsym` answering](decisions/D244-declaring-a-real-sdk-version-stops.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D245 | [The section written to explain D235 committed D235](decisions/D245-the-section-written-to-explain-d235.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D246 | [A placeholder in a symbol field is not a symbol](decisions/D246-a-placeholder-in-a-symbol-field-is-not.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D247 | [An unbound import is unbound in the PLT too, so the gating is right](decisions/D247-an-unbound-import-is-unbound-in-the-plt.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D248 | [Every import was weak, and a loader took that at its word](decisions/D248-every-import-was-weak-and-a-loader-took.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D249 | [The display path threw away every code it was given](decisions/D249-the-display-path-threw-away-every-code.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D250 | [The display held the output while answering that it did not](decisions/D250-the-display-held-the-output-while.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D251 | [A sweep inside the probe, instead of a rebuild per guess](decisions/D251-a-sweep-inside-the-probe-instead-of-a.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D252 | [Two codes from one call are worth more than one code from two runs](decisions/D252-two-codes-from-one-call-are-worth-more.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D253 | [The framebuffer alignment, which was the answer all along](decisions/D253-the-framebuffer-alignment-which-was-the.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D254 | [Double-buffering prevents screen tearing during report rendering](decisions/D254-double-buffering-prevents-screen.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D255 | [GEN in compatibility mode is ps4_mode, not unknown or gen4](decisions/D255-gen-in-compatibility-mode-is-ps4-mode.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D256 | [Bounded flip wait ensures display flip takes effect](decisions/D256-bounded-flip-wait-ensures-display-flip.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D257 | [Frame count advancement confirms presentation](decisions/D257-frame-count-advancement-confirms.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D258 | [Display HUD integration preserves headless execution](decisions/D258-display-hud-integration-preserves.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D259 | [The D-pad pages the results; the auto-cycle is the floor, not the mechanism](decisions/D259-the-d-pad-pages-the-results-the-auto.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D260 | [Per-file objects, so a build recompiles only what changed](decisions/D260-per-file-objects-so-a-build-recompiles.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D261 | [Firmware is the console's, not the compatibility environment's](decisions/D261-firmware-is-the-console-s-not-the.md) | measured | ~2026-08-29..08-30 |
| 🟢 | D262 | [A build with no status bar does not gather what would fill one](decisions/D262-a-build-with-no-status-bar-does-not.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D263 | [The GEN field names the mode when it cannot name the console; the PS4-compat version is its own field](decisions/D263-the-gen-field-names-the-mode-when-it.md) | decided | ~2026-08-29..08-30 |
| 🟢 | D264 | [Export vaddrs are confirmed by behaviour, not read from firmware](decisions/D264-export-vaddrs-are-confirmed-by.md) | confirmed | 2026-08-30 |
| 🟢 | D265 | [`obscene-tool vaddrs`: exports resolved to names, in the tool not a script](decisions/D265-obscene-tool-vaddrs-exports-resolved-to.md) | measured | ~2026-08-30..08-31 |
| 🟢 | D266 | [Three checks staged for the next hardware run: the layout of a memory type, whether a short size bounds a write, and more export candidates](decisions/D266-three-checks-staged-for-the-next.md) | hardware | 2026-08-31 |
| 🟢 | D267 | [The package must stage `sce_module/`, and a rewrite dropped it](decisions/D267-the-package-must-stage-sce-module-and-a.md) | hardware | ~2026-08-31 |
| ⚪ | D268 | [On-console libkernel export table enumeration replaces offline sprx dumps](decisions/D268-on-console-libkernel-export-table.md) | unrecorded | ~2026-08-31 |
| 🟢 | D269 | [One verb per hardware task; the report comes off the system log, in the tool](decisions/D269-one-verb-per-hardware-task-the-report.md) | hardware | ~2026-08-31 |
| 🟢 | D270 | [Real hardware is a pointer from the compatibility table, not a column in it](decisions/D270-real-hardware-is-a-pointer-from-the.md) | hardware | ~2026-08-31 |
| ⚪ | D271 | [The report screen wraps into two columns before it overruns the totals](decisions/D271-the-report-screen-wraps-into-two.md) | unrecorded | ~2026-08-31 |
| ⚪ | D272 | [A GPU field for the driver, DISK wired through `statfs`; TEMP stays honestly unknown](decisions/D272-a-gpu-field-for-the-driver-disk-wired.md) | unrecorded | ~2026-08-31 |
| ⚪ | D273 | [The payload round-trip is a verb too: `./bin/obscene payload`](decisions/D273-the-payload-round-trip-is-a-verb-too.md) | unrecorded | ~2026-08-31 |
| 🟢 | D274 | [The TSC-frequency signature is a band, not an equality; four more export candidates](decisions/D274-the-tsc-frequency-signature-is-a-band.md) | measured | 2026-08-31 |
| 🟢 | D275 | [a measured run-context axis, and the loaded-module link-map that feeds it](decisions/D275-a-measured-run-context-axis-and-the.md) | measured | ~2026-08-31..09-01 |
| 🟡 | D276 | [payload mode is the ps4 backward-compatibility context](decisions/D276-payload-mode-is-the-ps4-backward.md) | assumed | ~2026-08-31..09-01 |
| 🟡 | D277 | [enumerate modules by base+vaddr, the way past a payload's three shut doors](decisions/D277-enumerate-modules-by-base-vaddr-the-way.md) | assumed | ~2026-08-31..09-01 |
| 🟢 | D278 | [obscene-injector: decoupled freestanding ELF injection via session kernel R/W](decisions/D278-obscene-injector-decoupled-freestanding.md) | decided | ~2026-08-31..09-01 |
| 🟢 | D279 | [canonical release artifact naming scheme across CI workflows and frontend](decisions/D279-canonical-release-artifact-naming.md) | decided | ~2026-08-31..09-01 |
| 🟡 | D280 | [Porthole scaffolded, and the encoder reachability made a section (106-encoder)](decisions/D280-porthole-scaffolded-and-the-encoder.md) | assumed | 2026-09-01 |
| 🟢 | D281 | [Projects live under src/, and the encoder section finished its wiring](decisions/D281-projects-live-under-src-and-the-encoder.md) | decided | 2026-09-01 |
| ⚪ | D282 | [dlsym gets its positive: a known symbol through a valid handle](decisions/D282-dlsym-gets-its-positive-a-known-symbol.md) | unrecorded | ~>2026-09-01 |
| 🔴 | D283 | [flexible-configured: the probe orbistoun's allocator work was blocked on](decisions/D283-flexible-configured-the-probe-orbistoun.md) | blocked | ~>2026-09-01 |
| ⚪ | D284 | [obs_read_header is bounded by the caller's buffer, not by a constant](decisions/D284-obs-read-header-is-bounded-by-the.md) | unrecorded | ~>2026-09-01 |
| 🟢 | D285 | [the measured value is reported on every row, not only on a difference](decisions/D285-the-measured-value-is-reported-on-every.md) | measured | ~>2026-09-01 |
| 🟢 | D286 | [container-structure: a raw measured dump of a real gen-5 container](decisions/D286-container-structure-a-raw-measured.md) | measured | ~>2026-09-01 |
| 🟡 | D287 | [the native title carries its eboot and is a ./bin/obscene verb](decisions/D287-the-native-title-carries-its-eboot-and.md) | assumed | 2026-09-01 |

| | meaning |
|---|---|
| 🟢 | settled, and the reasoning rests on something checkable |
| 🟡 | assumed or proposed - made without input, and in the review queue |
| 🔴 | reversed, superseded or blocked |
| ⚪ | no status recorded |

A date with `~` is **not recorded** - it is worked out from the dated entries either
side, because an entry between two of them was written between their dates. `~` alone
is a day both neighbours agree on; `~a..b` is a span, and no day inside it is claimed;
`~>a` and `~<a` are entries with a dated neighbour on only one side. A bare `-` has no
dated entry either side to reason from.
