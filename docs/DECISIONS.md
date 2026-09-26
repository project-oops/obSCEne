# Decisions

The decisions in force, one file each under `decisions/`. Format and rules are in
[STYLE](https://github.com/project-oops/OOPS/blob/main/docs/STYLE.md#decisions).

**This table is generated.** Edit an entry under `decisions/`, then run
`tools/split-decisions.sh --index obscene`. A number resolves to exactly one file.

| | # | decision | status | date |
|---|---|---|---|---|
| 🟢 | D001 | [A host build beside the target build](decisions/D001-a-host-build-beside-the-target-build.md) | decided | 2026-09-26 |
| 🟢 | D002 | [Announce before attempting](decisions/D002-announce-before-attempting.md) | decided | 2026-09-26 |
| 🟢 | D003 | [One machine-readable format; presentation lives in the tool](decisions/D003-one-machine-readable-format.md) | decided | 2026-09-26 |
| 🟢 | D004 | [Six statuses](decisions/D004-six-statuses.md) | decided | 2026-09-26 |
| 🟢 | D005 | [Freestanding C, clang only, no vendor SDK](decisions/D005-freestanding-c-clang-only-no-vendor-sdk.md) | decided | 2026-09-26 |
| 🟢 | D007 | [Check from the failure side where layouts are unknown](decisions/D007-check-from-the-failure-side-where-layouts-are-unknown.md) | decided | 2026-09-26 |
| 🟢 | D008 | [Nothing is invented; uncertain signatures are omitted](decisions/D008-nothing-is-invented.md) | decided | 2026-09-26 |
| 🟢 | D009 | [The section order is one explicit list](decisions/D009-the-section-order-is-one-explicit-list.md) | decided | 2026-09-26 |
| 🟢 | D010 | [Dependencies are capability bits, not check names](decisions/D010-dependencies-are-capability-bits.md) | decided | 2026-09-26 |
| 🟢 | D012 | [The build directory is overridable and Linux-local](decisions/D012-the-build-directory-is-linux-local.md) | decided | 2026-09-26 |
| 🟢 | D014 | [Presence and behaviour are separate questions](decisions/D014-presence-and-behaviour-are-separate.md) | decided | 2026-09-26 |
| 🟢 | D015 | [A census needs a control in both directions](decisions/D015-a-census-needs-a-control-in-both-directions.md) | decided | 2026-09-26 |
| 🟢 | D017 | [The C runtime is a behavioural section](decisions/D017-the-c-runtime-is-a-behavioural-section.md) | decided | 2026-09-26 |
| 🟢 | D018 | [Derived facts enter the repository as provenance-headed data](decisions/D018-derived-facts-enter-as-provenance-headed-data.md) | decided | 2026-09-26 |
| 🟢 | D019 | [A regression is a check that got worse](decisions/D019-a-regression-is-a-check-that-got-worse.md) | decided | 2026-09-26 |
| 🟢 | D021 | [Symbols carry their console generation](decisions/D021-symbols-carry-their-console-generation.md) | decided | 2026-09-26 |
| 🟢 | D026 | [The tooling is Rust, in one binary](decisions/D026-the-tooling-is-rust-in-one-binary.md) | decided | 2026-09-26 |
| 🟢 | D032 | [The output channel is chosen by what moves a byte](decisions/D032-the-output-channel-is-chosen-by-what-moves-a-byte.md) | decided | 2026-09-26 |
| 🟢 | D035 | [The entry point ends the process](decisions/D035-the-entry-point-ends-the-process.md) | decided | 2026-09-26 |
| 🟢 | D040 | [Checks can be excluded, and the default list is empty](decisions/D040-checks-can-be-excluded-and-the-default-list-is-empty.md) | decided | 2026-09-26 |
| 🟢 | D041 | [The report is drawn as well as written](decisions/D041-the-report-is-drawn-as-well-as-written.md) | decided | 2026-09-26 |
| 🟢 | D044 | [Every check records where its expectation came from](decisions/D044-every-check-records-where-its-expectation-came-from.md) | decided | 2026-09-26 |
| 🟢 | D046 | [An intermittent failure is reported with its rate](decisions/D046-an-intermittent-failure-is-reported-with-its-rate.md) | decided | 2026-09-26 |
| 🟢 | D047 | [Responsiveness is asked separately from correctness](decisions/D047-responsiveness-is-asked-separately-from-correctness.md) | decided | 2026-09-26 |
| 🟢 | D049 | [`crack`: a hit is proof and a miss is nothing](decisions/D049-crack-hits-are-proof-and-misses-are-nothing.md) | decided | 2026-09-26 |
| 🟢 | D051 | [Value checks use exact answers that a stub cannot give](decisions/D051-value-checks-use-exact-answers.md) | decided | 2026-09-26 |
| 🟢 | D054 | [The emulators live outside the repository, read for facts](decisions/D054-the-emulators-live-outside-the-repository.md) | decided | 2026-09-26 |
| 🟢 | D058 | [A check guards every symbol it calls](decisions/D058-a-check-guards-every-symbol-it-calls.md) | decided | 2026-09-26 |
| 🟢 | D059 | [Measurements assert nothing](decisions/D059-measurements-assert-nothing.md) | decided | 2026-09-26 |
| 🟢 | D062 | [The console generation is a build input; everything else is detected](decisions/D062-the-console-generation-is-a-build-input.md) | decided | 2026-09-26 |
| 🟢 | D067 | [The headline number is a frontier, not a sum](decisions/D067-the-headline-number-is-a-frontier.md) | decided | 2026-09-26 |
| 🟢 | D068 | [Relational checks where no document reaches](decisions/D068-relational-checks-where-no-document-reaches.md) | decided | 2026-09-26 |
| 🟢 | D070 | [Documentation is checked against the tree](decisions/D070-documentation-is-checked-against-the-tree.md) | decided | 2026-09-26 |
| 🟢 | D071 | [The orchestration scripts are `sh`](decisions/D071-the-orchestration-scripts-are-sh.md) | decided | 2026-09-26 |
| 🟢 | D072 | [Consensus is a substitute oracle within one generation](decisions/D072-consensus-is-a-substitute-oracle-within-one-generation.md) | decided | 2026-09-26 |
| 🟢 | D081 | [Layout checks record bytes and interpret nothing](decisions/D081-layout-checks-record-bytes-and-interpret-nothing.md) | decided | 2026-09-26 |
| 🟢 | D084 | [obSCEne may consult other projects; orbistoun may not](decisions/D084-obscene-may-consult-other-projects-and-orbistoun-may-not.md) | decided | 2026-09-26 |
| 🟢 | D090 | [The compatibility table is generated and ranks nothing](decisions/D090-the-compatibility-table-is-generated.md) | decided | 2026-09-26 |
| 🟢 | D094 | [Emulators are built from the source that is read](decisions/D094-emulators-are-built-from-source.md) | decided | 2026-09-26 |
| 🟢 | D096 | [The blind prober calls what it cannot describe, and never on hardware](decisions/D096-the-blind-prober-calls-what-it-cannot-describe.md) | decided | 2026-09-26 |
| 🟢 | D097 | [Blame for a loader failure needs a control build](decisions/D097-blame-needs-a-control-build.md) | decided | 2026-09-26 |
| 🟢 | D102 | [The command protocol is specified first, with transcripts as contract](decisions/D102-the-protocol-is-specified-before-it-is-implemented.md) | decided | 2026-09-26 |
| 🟢 | D104 | [Platform backends are chosen by the source list](decisions/D104-backends-are-chosen-by-the-source-list.md) | decided | 2026-09-26 |
| 🟢 | D108 | [A report and a corpus differ, and only the corpus carries machine origin](decisions/D108-only-a-corpus-carries-machine-origin.md) | decided | 2026-09-26 |
| 🟢 | D109 | [The GPU is probed by executing shaders and reading result bits](decisions/D109-the-gpu-is-probed-by-executing-shaders.md) | decided | 2026-09-26 |
| 🟢 | D114 | [The census walks every symbol; the prober walks a callable list](decisions/D114-the-prober-walks-a-callable-list.md) | decided | 2026-09-26 |
| 🟢 | D116 | [A reference oracle for GPU results](decisions/D116-a-reference-oracle-for-gpu-results.md) | decided | 2026-09-26 |
| 🟢 | D117 | [A GPU ISA surface census](decisions/D117-a-gpu-isa-surface-census.md) | decided | 2026-09-26 |
| 🟢 | D121 | [Console generation is observed, never asserted from presence](decisions/D121-console-generation-is-observed-never-asserted-from-presence.md) | decided | 2026-09-26 |
| 🟢 | D122 | [Code-execution verbs are off unless a build asks for them](decisions/D122-code-execution-verbs-are-off-unless-a-build-asks.md) | decided | 2026-09-26 |
| 🟢 | D125 | [A gate is trusted only after it has rejected something](decisions/D125-a-gate-is-trusted-only-after-it-has-rejected-something.md) | decided | 2026-09-26 |
| 🟢 | D129 | [`call` and `read` invoke and dump without pre-validating](decisions/D129-call-and-read-are-implemented-proven.md) | decided | 2026-09-26 |
| 🟢 | D131 | [GPU kernels are files, and the kernel list is generated](decisions/D131-gpu-kernels-are-files-and-the-list-is-generated.md) | decided | 2026-09-26 |
| 🟢 | D132 | [A serving build listens first and runs the suite on demand](decisions/D132-a-serving-build-listens-first.md) | decided | 2026-09-26 |
| 🟢 | D137 | [GPU diffs are keyed by input, not lane](decisions/D137-gpu-diffs-are-keyed-by-input.md) | decided | 2026-09-26 |
| 🟢 | D138 | [An unnamed identifier is imported with a `$` sigil](decisions/D138-an-unnamed-identifier-is-imported-with-a-sigil.md) | decided | 2026-09-26 |
| 🟢 | D139 | [The HUD shows a value or `unknown`, and is screen-only](decisions/D139-the-hud-shows-a-value-or-unknown.md) | decided | 2026-09-26 |
| 🟢 | D140 | [The display prefers the older video-out form when both resolve](decisions/D140-the-display-prefers-the-older-video-out-form.md) | decided | 2026-09-26 |
| 🟢 | D143 | [The corpus records what it was mined from](decisions/D143-the-corpus-records-what-it-was-mined-from.md) | decided | 2026-09-26 |
| 🟢 | D144 | [A timeout is told from a hang by doubling the budget](decisions/D144-a-timeout-is-told-from-a-hang-by-doubling-the-budget.md) | decided | 2026-09-26 |
| 🟢 | D164 | [The prober's classifier keeps emulator knowledge out](decisions/D164-the-prober-classifier-stays-shape-only.md) | decided | 2026-09-26 |
| 🟢 | D165 | [The command socket takes a secret generated per startup](decisions/D165-the-command-socket-takes-a-per-startup-secret.md) | decided | 2026-09-26 |
| 🟢 | D170 | [A document may anchor a claim to the source](decisions/D170-a-document-may-anchor-a-claim-to-the-source.md) | decided | 2026-09-26 |
| 🟢 | D172 | [Exclusions are learned at run time from the previous report](decisions/D172-exclusions-are-learned-at-run-time-from-the-previous-report.md) | decided | 2026-09-26 |
| 🟡 | D176 | [A patched loader's report never occupies that loader's row](decisions/D176-a-patched-loader-measures-the-patch.md) | assumed | 2026-09-26 |
| 🟢 | D180 | [Each loading mechanism is its own artifact](decisions/D180-each-loading-mechanism-is-an-artifact.md) | decided | 2026-09-26 |
| 🟢 | D183 | [obSCEne does not do what a shell does](decisions/D183-obscene-does-not-do-what-a-shell-does.md) | decided | 2026-09-26 |
| 🟢 | D184 | [Hardware is registered by address; its capabilities are measured on every use](decisions/D184-hardware-capabilities-are-measured-on-every-use.md) | decided | 2026-09-26 |
| 🟢 | D189 | [The console transport is a shared crate, taken by path](decisions/D189-the-console-transport-is-a-shared-crate.md) | decided | 2026-09-26 |
| 🟢 | D192 | [Corroboration is counted per independent claim](decisions/D192-corroboration-is-counted-per-claim.md) | decided | 2026-09-26 |
| 🟢 | D199 | [Scripts reach WSL through one shim](decisions/D199-scripts-reach-wsl-through-one-shim.md) | decided | 2026-09-26 |
| 🟢 | D200 | [The file formats live in selfish; measurements stay here](decisions/D200-the-file-formats-live-in-selfish.md) | decided | 2026-09-26 |
| 🟢 | D202 | [A mined name a section calls is declared in that section](decisions/D202-a-mined-name-a-section-calls-is-declared-in-the-section.md) | decided | 2026-09-26 |
| 🟢 | D213 | [The payload runtime is obSCEne's; its format primitives are selfish's](decisions/D213-the-payload-runtime-is-obscene-and-the-format-is-selfish.md) | decided | 2026-09-26 |
| 🟢 | D220 | [The modules a title bundles are built here, never copied](decisions/D220-bundled-modules-are-built-here.md) | decided | 2026-09-26 |
| 🟢 | D226 | [An eboot requires only the libraries the harness needs; the census probes the rest at run time](decisions/D226-an-eboot-requires-only-what-the-harness-needs.md) | decided | 2026-09-26 |
| 🟢 | D235 | [An unresolved symbol has three explanations, and the report names which](decisions/D235-an-unresolved-symbol-has-three-explanations.md) | decided | 2026-09-26 |
| 🟢 | D248 | [Imports are encoded with global binding](decisions/D248-imports-are-encoded-with-global-binding.md) | decided | 2026-09-26 |
| 🟢 | D260 | [Each source compiles to its own object, keyed by a flags sentinel](decisions/D260-each-source-compiles-to-its-own-object.md) | decided | 2026-09-26 |
| 🟢 | D269 | [One verb per hardware task, on `bin/obscene`](decisions/D269-one-verb-per-hardware-task.md) | decided | 2026-09-26 |
| 🟢 | D278 | [A separate payload loads the probe into a native process](decisions/D278-a-separate-payload-loads-the-probe-natively.md) | decided | 2026-09-26 |
| 🟢 | D296 | [Privilege tier and application category are independent build inputs](decisions/D296-privilege-and-category-are-independent-build-inputs.md) | decided | 2026-09-26 |
| 🟢 | D297 | [SDK versions come from an external dictionary and are validated per generation](decisions/D297-sdk-versions-come-from-an-external-dictionary.md) | decided | 2026-09-26 |
| 🟢 | D298 | [A native title carries a process parameter block and compliant bundled modules](decisions/D298-a-native-title-carries-a-procparam-and-bundled-modules.md) | decided | 2026-09-26 |
| 🟢 | D302 | [A hardware capture is named for the launch shape, and hardware is the authority](decisions/D302-captures-are-named-for-the-launch-shape.md) | decided | 2026-09-26 |
| 🟢 | D303 | [A scalar out-parameter is poisoned, not zeroed](decisions/D303-a-scalar-out-parameter-is-poisoned-not-zeroed.md) | decided | 2026-09-26 |
| 🟢 | D314 | [The decision gate checks the split log, and finding nothing fails](decisions/D314-the-decision-gate-checks-the-split-log.md) | decided | 2026-09-26 |
| 🟢 | D315 | [A document beside a Makefile names that Makefile's rules](decisions/D315-a-document-beside-a-makefile-means-that-makefile.md) | decided | 2026-09-26 |
| 🟢 | D316 | [Porthole drives a virtual controller device](decisions/D316-porthole-drives-a-virtual-controller-device.md) | decided | 2026-09-26 |
| 🟢 | D317 | [Porthole reuses oops-sdk for display and memory](decisions/D317-porthole-reuses-oops-sdk-for-display-and-memory.md) | decided | 2026-09-26 |
| 🟢 | D325 | [A fault guard catches a crashing check so the suite continues](decisions/D325-a-fault-guard-catches-a-crashing-check.md) | decided | 2026-09-26 |
| 🟢 | D327 | [The payload zeroes its own .bss](decisions/D327-the-payload-zeroes-its-own-bss.md) | decided | 2026-09-26 |
| 🟢 | D328 | [A pending status for a check that is waiting for its input](decisions/D328-a-pending-status-for-a-check-awaiting-input.md) | decided | 2026-09-26 |
| 🟢 | D331 | [A borrowed value is a hypothesis a check can refute, not an expectation](decisions/D331-a-borrowed-value-is-a-hypothesis-a-check-can-refute.md) | decided | 2026-09-26 |
| 🟢 | D334 | [The default suite runs only checks that retire reliably](decisions/D334-the-default-suite-runs-only-checks-that-retire-reliably.md) | decided | 2026-09-26 |

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
