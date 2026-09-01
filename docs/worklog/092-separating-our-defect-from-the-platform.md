# 2026-08-30 - separating our defect from the platform's


The complete suite runs on the console without crashing: `OBS|tally|220|75|192|41`, 528 checks,
30 sections, `suite complete`, console healthy afterwards and repeatable.

**The unit of work was a diagnosis, not a fix.** Twenty-four checks skip because their symbol is
null, and that had been recorded as a platform gap. It is not, for most of them.

`061-imports` reports every import on two axes - did the loader bind it, and does a run-time
lookup find the same name in the same library. Fourteen come back `unlinked|resolvable`: the
console has the symbol, under the name and library this module declares, and our import did not
bind. Six are genuinely not offered.

**Three previously-recorded findings were wrong, all in one shape.**

- `HARDWARE.md` said the loader mapped five of our twelve libraries. It maps **ten of twelve**,
  and 165 in total, with address ranges and fingerprints in the system log. The claim had been
  inferred from which *symbols* resolved.
- `crt.c` said a launching title declares SDK version zero. It declares `0x08008011`. The claim
  came from a kernel log line that was **this program's own run**.
- `import_libs` showed our library table as flawless. It was showing only the fields it knew how
  to name; the attribute word, which it did not print, was wrong.

Each is an observation recorded as the thing it was evidence for - D232, D233, D235 again, and
now D242 and D241. The distinguishing feature every time is that a second source existed and
was not consulted. There is one in this repository: `~/oracle/uroot/eboot.bin`, a title that
launches, and every question asked of it this session answered in one command.

**Eliminated by measurement rather than argument:** SDK version (A/B, identical builds, no
effect on binding - and it stops `sceKernelDlsym` answering entirely); the import-library
attribute; `.prx` versus `.sprx`; malformed identity tables; and whether an unbound import is
callable anyway - it is not, the process dies, so the gating is correct and there is no cheap
repair hiding behind a wrong test.

**Surprises worth keeping.**

*The section written to explain D235 committed D235.* It opens libraries to ask the resolver,
and walked into one of the ten that end the process - one library after `libScePad`, which is
`libSceVideoRecording`. Written immediately after describing the mistake. `EXCLUDE` could not
have caught it, because that matches check ids and this is one check opening many libraries.

*A placeholder cost a wrong hypothesis.* The census writes `(census)` where a symbol name goes;
those rows read as `linked`, so every library appeared to bind two imports and `libScePad`
looked like a per-symbol failure. It is per-library: `libkernel` and `libSceLibcInternal` bind
everything, every other library binds nothing.

*The report was unreadable by everything that fetches it.* Mode `0600`, written by a title,
retrieved by the shell server and the file server running as somebody else. The mode had been
chosen by reasoning about the writer.

*`/download0` does not outlive the title.* The mount is torn down on exit, so retrieval happens
during a run or through the system log. Second reason the log is written unconditionally.

**Still open:** why `libkernel` and `libSceLibcInternal` bind and nothing else does. Both are
resident before this module is looked at, which is the one property that separates them from
the ten that are mapped and unbound.

