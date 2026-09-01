# D197 - A consensus across generations is not a consensus, and mixing them destroyed the signal


Four loaders complete the full suite as of today, which made `obscene-tool consensus` worth
running for the first time - it is the substitute oracle for a project with no hardware, and it
had never had four complete reports to work with. The first run said:

```text
# implementations shadPS4, PS5PCEM, fpPS4-patched, Kyty-patched
# agreed          142
# disagreed       360
```

and named PS5PCEM as the lone dissenter **328 times out of 358**, 273 of those saying `fail`.

That reads as a damning result about PS5PCEM and is not one. The sweep builds a module per
loader generation, so PS5PCEM ran a **GEN=5** module and the other three ran **GEN=4**. The
reports say so themselves: `generation|known|5 (agc)` against `known|both`. The comparison was
between two different builds of obSCEne, and most of the 360 disagreements were that.

Within a generation, the same tool over the same three GEN=4 reports:

```text
# implementations shadPS4, fpPS4-patched, Kyty-patched
# agreed          469
# disagreed       32
```

**469 of 515.** The outliers are balanced too - Kyty 15, shadPS4 8, fpPS4 5 - where the mixed
run had put 92% of them on one loader. Nearly every "disagreement" in the first run was an
artefact of the comparison, and it pointed hard at a specific project.

### The limitation this exposes, which is the real finding

The working oracle is **three previous-generation emulators**. For the generation this project
actually targets there is no cohort at all: PS5PCEM completes, prosper is partial, orbistoun is
the sibling project and not independent evidence. So agreement is currently evidence about the
*previous* platform, and the count of implementations is not the thing to look at - their
generation is.

`consensus` already warns that these projects read each other's source and agreement is not
four witnesses. This is the same caution one level down: they must at least be answering the
same question.

### A second oracle, internal to the report

`007-responsive` and `035-libc` reach the same functions by different routes - one asks whether
a function reads its arguments at all, the other tests whether it gets the right answer. Across
five string functions and three loaders they corroborate on every case, including the one that
looks like a miss:

| loader | `strlen` | `strchr` | `strstr` |
|---|---|---|---|
| shadPS4 | responds / pass | responds / pass | **silent / fail** |
| fpPS4 | responds / pass | **silent / fail** | **responds / partial** |
| Kyty | **silent / fail** | **silent / fail** | **silent / fail** |

fpPS4's `strstr` responds and only partially passes: implemented, and wrong. That is not the
sections disagreeing, it is the distinction they exist to draw, and it needs different work from
an absence. Kyty is silent on all five, which is what its fifteen outliers are - it implements
essentially no libc string functions, and the patch did not cause that, it made it visible.


### Measured the same day: two is not a cohort either

prosper converged to a complete run - 36,550 records, 27 of 27 sections, 478 pass / 7 partial /
18 fail / 12 skip, with only the four semaphore checks skipped. That is the highest pass count
of any loader here and it is a **GEN=5** run, so it looked like the gap above had closed.

It has not. Consensus over the two current-generation reports:

```text
# implementations PS5PCEM, prosper
# agreed          115
# disagreed       329
```

and **all 329 are `SPLIT`, none is `OUTLIER`**. With two implementations there is no majority,
so the tool cannot name a loser - only report that they differ. An oracle needs **three**: two
gives a diff, three gives a verdict. The three-loader GEN=4 cohort produces 358 outliers, each
naming a specific loader on a specific check; the two-loader GEN=5 cohort produces none.

So the count of implementations does matter after all, alongside their generation, and the
threshold is three rather than two.

### And the second member may not be evidence

prosper's own `005-generation/detect` result is `partial`, reporting generation **`both`** - not
`5`. That is obSCEne's own caveat firing verbatim: *"real back-compat, or a stub-everything
loader answering for free"*. prosper links all 35,518 imports as stub slots, so both
generations' exclusive symbols resolve for nothing.

Its 478 passes are the highest here partly because returning zero is frequently the right
answer, and a loader that answers everything agrees with nothing in particular. The check that
says so was written for exactly this and is doing its job; the number to distrust is the
flattering one.


Status: **derived** - three cohorts measured with the same tool over reports produced the
same day.

