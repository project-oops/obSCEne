# D103 - The hash is confirmed against a second, independent corpus - 281 of 281. And obSCEne will not discover a symbol it has never heard of, ever


Status: derived - by comparison against an implementation written independently of this
project.

PS5PCEM publishes its export tables as name and identifier together:

```zig
.{ .name = "sceVideoOutSubmitFlip", .function = ..., .expect_id = "U46NwOiJpys" },
```

1401 pairs, extracted and compared. **281 overlap obSCEne's surface, and all 281
identifiers match what `obscene-tool nid` computes. None differ.**

That is worth more than the `ps4libdoc` agreement already recorded, for one reason: this
corpus is current-generation and covers `sceAgc*`, which a previous-generation corpus
cannot. The suffix and the truncation are now confirmed on both generations by two sources
that share no code with each other or with this project.

### The other half, which is less comfortable

| | |
|---|---|
| PS5PCEM implements | 1401 |
| obSCEne knows | 564 (189 called, 375 censused) |
| both | 281 |
| **obSCEne has never heard of** | **1120** |

Across all three current-generation emulators the figure is **1444**.

**Nothing about running obSCEne will ever find those.** It resolves by an identifier
computed from a name this project supplies: a symbol absent from `surface.h` is not
censused, a symbol absent from `platform.h` is not imported, and a symbol that is neither
cannot appear in any report. It is a blind spot by construction and not a
not-yet-encountered case, which is the opposite of how a probe usually fails.

The only thing that closes it is reading export tables and diffing - `scripts/ps5-gap.py`,
now extended to parse the Zig tables alongside the C# attributes. That script is therefore
not a convenience. **It is the sole mechanism by which this program's surface grows**, and
a session that runs the suite without running the gap analysis has learned nothing new
about what exists.

