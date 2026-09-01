# D168 - A comment inside a table row's braces made a check invisible to every gate, and nothing failed


D166 downgraded `015-sync/event-flag-round-trip` from `DOCUMENTED` to `ASSUMED` and put the
reasoning in a comment placed **between the capability fields and the runner**:

```c
{"015-sync/event-flag-round-trip", "libkernel", "sceKernelPollEventFlag",
 OBS_CAP_NONE, OBS_CAP_NONE, (const void *)&sceKernelPollEventFlag,
 /* ... eleven lines of explanation ... */
 check_event_flag_round_trip, OBS_FROM_ASSUMED},
```

`sections::rows_in` finds the runner by taking the field before `OBS_FROM_*` and requiring it
to be an identifier. With a block comment in that position the row stops matching and
**disappears from `guards`, `caps` and `counts` simultaneously**, because all three read the
tables through that one parser.

The check still compiled and still ran. Nothing reported anything. It was found by hand-
counting rows while writing a summary - `guards` said 146 that morning and 145 that evening,
and only the coincidence of both numbers being in view caught it.

### This is the failure `sections.rs` says cannot be gated

Its module documentation is explicit, and was right at the time:

> Every one of those failed by producing a *smaller number*, which no gate can detect, because
> a gate compares against nothing.

There *is* something to compare against, and it was there all along: **a report**. The harness
walks the tables at run time and emits a result per check, so a report states what actually
ran rather than what a regular expression believes is present. `obscene-tool rows` diffs the
two.

Reading `res` records rather than `try`, deliberately: a skipped check emits no `try` (the
announce-before-attempting rule) so a `try`-based reading would under-report by exactly the
skipped set - the same failure, committed by the instrument built to catch it.

### Telling generated rows from missing ones

`900-surface` writes two rows by hand and builds 368 more by expanding
`OBS_SURFACE_LIBRARIES(OBS_GROUP_ROW)` over the census. Those exist only after the
preprocessor runs and no text parser can see them. **A gate that cries wolf 368 times is a
gate somebody switches off**, so they have to be tolerated - without also tolerating a
hand-written row that went missing.

The first rule tried was "a section with no literal rows is generated", and it was wrong for
precisely the case in hand: `900-surface` has *both* kinds, so every generated row was
reported as missed. The discrimination is per **file** - does this section's source contain a
table-macro expansion - not per row, and it is read from the source rather than from a list of
exempt section names, so it stops applying the moment the macro does.

Proven both ways: 514 checks accounted for on a clean tree, and with a comment injected into
an unrelated row it names that row and fails.

### The shape, again

That is the fourth instance this week of a mechanism that stopped working while reporting
something reasonable - D158 (a check that skipped for years while a document called it a
pass), D163 (an index with no denominator), the `PROTOCOL.md` prose that survived its own
revert, and this. Three of the four were caught by comparing two independent readings of the
same fact. The one that was not is still not gated: prose that *describes behaviour* has
nothing to diff against, which is why it sits at the top of the backlog.

