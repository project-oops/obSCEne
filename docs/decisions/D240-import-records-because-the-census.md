# D240 - `import` records, because the census cannot see this program's own imports


*status: decided*

A null import has two causes needing opposite repairs - the platform lacks the symbol, or this
module failed to ask for it correctly - and from inside a check they are identical. D235 made
the skip message stop claiming to know which; it did not make anything else able to tell.

The census would be the natural place to answer it and **structurally cannot**. A censused name
is declared as `const char` so the type system forbids calling it (D008); this program's own
imports are declared as functions in `platform.h`. A name cannot be in both places, so the
symbols whose status matters most are exactly the ones the census is blind to.

`061-imports` walks the check registry instead - every check row already carries
`(library, symbol, address)` - and emits one `import` record per symbol on two axes:

```text
OBS|import|libScePad|scePadOpen|unlinked|resolvable
```

On hardware that reads: **the platform has this symbol, under this name, in this library, and
our import did not bind.** Eleven symbols across five libraries said so. That is a defect here,
not a finding about the console, and it had been reported as the latter for three sessions.

The walk is restricted to libraries this module imports from. It was not, at first, and the
census's three hundred and fifty libraries went through it - opening every one, before the
census ran. The tally went 218/74/190/40 to 115/7/362/41 and the section written to explain a
measurement had replaced it. The overflow counter is the only reason that run was readable.

