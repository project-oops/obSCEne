# 2026-08-30 (later) - the answer was in a field nobody chose


**Every import was `STB_WEAK`, undefined.** A loader reads that as permission not to bother, and
this one does: it bound the two libraries already resident in the process and left every library
it would have had to load mapped-but-unbound. Re-binding imports `STB_GLOBAL` in `selfish` - the
same re-encode that already rewrites the type - fixed it.

```text
                                        before      after
imports bound                              127        141
platform has it, we did not bind it         14          0
checks skipped for an unresolved symbol      24         10
tally                            220/75/192/41   232/75/192/29
```

`100-input`, `090-audio` and `165-gnm` pass outright now. `080-video`, `070-user` and
`130-layout` run for the first time and found three things no run had ever reached: the user
service refusing to initialise (`0x80960003`), the main video output refusing to open
(`0x80290009`), and the layout size argument not being validated.

The weak binding stays in the C, where the reason for it is - an absent symbol should be null,
not a link error. It just does not travel into the module any more.

**The method is the part worth keeping.** Four candidates were tested and eliminated first: the
import-library attribute, the SDK version, the `.prx` filename, and whether the identity tables
were malformed. Three were settled against `~/oracle/uroot/eboot.bin` and one by an A/B on the
console with a build flag.

Every one of those was a field somebody had **chosen**. The answer was in `st_info` - written by
the compiler, passed through untouched, and printed by no probe in this repository. The library
table was inspected four times and looked flawless every time, because each probe showed the
fields it knew how to name.

So: print the byte, not the interpretation (D241), and when every deliberate choice has been
eliminated, go and look at what was never decided.

**Cost accounting, honestly.** Roughly a dozen hardware round trips. Three of them were spent on
defects in the diagnostic section itself - it opened a library that ends the process (D245), it
counted census placeholders as bound imports and turned a per-library finding into a per-symbol
one (D246), and it needed a build-id flag before the crash-resume would stop hiding it. The
section still repaid that several times over, but a probe written to explain a mistake is not
exempt from making it.

