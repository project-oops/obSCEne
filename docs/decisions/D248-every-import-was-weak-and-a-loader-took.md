# D248 - Every import was weak, and a loader took that at its word


*status: measured*

**The defect.** This program declares its platform functions `OBS_WEAK`, so a symbol the
platform does not have is null rather than a link error. That is a compile-time need and a
sound one. What it left behind was a dynamic symbol table in which all 203 imports were
`STB_WEAK`, undefined - and to a loader that means exactly "if resolving this costs anything,
do not bother."

It does not bother. Measured, before:

```text
ours   WEAK   FUNC   x203
real   GLOBAL FUNC   x126   GLOBAL OBJECT x13     (a title that launches: no weak imports)
```

`libkernel` and `libSceLibcInternal` bound every import because they are resident in the
process already and resolving against them is free. Every other declared library was **mapped**
- address range and fingerprint in the system log - and not one symbol bound. Fourteen imports
stayed null whose symbols the same process could find by name moments later.

**The fix**, in `selfish`: rebuild the binding as `STB_GLOBAL` when re-encoding an import, for
the same reason the type is already rebuilt as `STT_FUNC`. The weak binding belongs in the C,
where the reason for it is, and it does not travel into the module.

**The result**, same console, one flag of difference:

| | before | after |
|---|---|---|
| imports bound | 127 | **141** |
| the platform has it and we did not bind it | **14** | **0** |
| the platform does not offer it | 6 | 6 |
| checks skipped for an unresolved symbol | 24 | **10** |
| tally | 220/75/192/41 | **232/75/192/29** |

Whole sections came back: `100-input`, `090-audio`, `165-gnm` all pass, `080-video`,
`070-user` and `130-layout` run and report. **Three behavioural findings became visible that
no run had ever reached** - the user service refusing to initialise (`0x80960003`), the main
video output refusing to open (`0x80290009`), and the layout size argument not being validated.

**The risk that did not materialise.** Six imports are for symbols this platform does not have,
and a `GLOBAL` undefined symbol that cannot resolve is a thing a loader may refuse a module
over. It loads. Those six stay null and their ten checks skip, which is the honest outcome and
the same one weak was chosen to produce.

### How it was found, which is the part worth keeping

Four hypotheses were tested and eliminated first - the import-library attribute, the SDK
version, the `.prx` filename, and whether the tables were malformed - three of them against the
oracle and one by A/B on the console.

None of them was the answer, and all four were about fields somebody had **chosen**. The answer
was in a field nobody chose: `st_info`, written by the compiler, passed through untouched, and
never printed by any probe in this repository. The library table was inspected repeatedly and
looked flawless every time, because every probe showed the fields it knew how to name.

That is the same shape as D241 one level down. **Print the byte, not the interpretation** - and
when four deliberate choices have all been eliminated, start looking at what was never decided.

