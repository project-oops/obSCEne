# D025 - Unknown format constants are measured from a loader, not read from a parser

**decided** · 2026-08-19

`tools/probe-tags.py` builds a minimal module whose dynamic table carries candidate
tag values. A loader reports the ones it does not recognise, and **names** the ones it
does. Sweeping the range maps the tag space.

The vendor dynamic tag values are not in any documentation this project could find.
The obvious source is another project's ELF parser, which D018 rules out. The choice
was not between reading source and guessing: a loader's diagnostics are published
behaviour, in the same category as a compiler error message, and they produce
something a citation cannot - a result that can be re-run when a loader changes.

**It worked immediately.** `DT_SCE_FINGERPRINT = 0x61000007`, named by the loader
itself, sitting between two values it reported as unsupported.

**One probe per tag, not one probe for the range.** The first attempt put all 65
candidates in one table; the loader acted on each recognised tag's value, and the
third one it acted on faulted - discarding every later verdict. The batched version
looked like it had recognised 57 tags when it had simply stopped. A probe that stops
early and a probe that finds nothing produce the same silence, which is the same
ambiguity the census control exists to resolve, in a third place.

Values are all zero. A plausible-looking offset would send the loader reading a table
that is not there; zero limits it to whatever a null does, and the probe only needs
each tag *classified*, not successfully used.

