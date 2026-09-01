# Vendor segment automated; two loader gates down, one named


The dynamic segment is now built by the tool rather than assembled by hand, the
derivation that produced its constants runs on every build, and two further loader
rejections were found and one of them fixed.

**What changed**

- `mkmodule` always builds the vendor segment. The `--dynlib` flag is gone (D027).
- All fourteen tags are emitted, and the tables are laid out in the order that makes
  the derivation's sums close: strtab, symtab, jmprel, rela, hash.
- `obscene-tool derive` re-runs the derivation against any module. `mkmodule` calls it
  on its own output, so the build fails if a constant drifts from the layout (D028).
- The linker script declares `PT_SCE_DYNLIBDATA` instead of the tool repurposing a
  `GNU_STACK` header. The script had already taken over segment declaration, and the
  two mechanisms had started to conflict - the repurposing failed outright because
  there was no longer a spare header to take.
- Three loadable segments rather than four (D029).
- Tag names are printed from the constants rather than a second table in the printer.
  The printer's table had gone stale and was reporting seven assigned tags as unknown,
  which reads as an open question that had in fact been answered.

**Surprises**

*The tidy segment layout was the wrong one.* Giving the headers and link tables their
own read-only segment is what a linker script wants to do and it produced a module a
loader refused on segment count. Vendor modules have exactly three.

*Removing the reference removed the ability to re-derive - until the derivation was
pointed at our own output.* This looked like a loss when the toolchain was deleted. It
is not, and the result is better: a check against a file nobody has any more would
never run, whereas this one runs on every build.

*The old error was hiding two more.* `mkmodule` had been failing before the emulator
ever saw the module, so neither the segment-count refusal nor the import-table failure
below had ever been reached.

**Where it stops, precisely**

The module now loads. All three segments map, the entry point is resolved and reported,
and then:

    Unable to find library and module

The cause is visible in our own string table. Every symbol is encoded

    <nid>#A#A

where the two suffixes are a library id and a module id - both zero. They are indices
into import tables that the module does not declare, so the loader looks up library 0
in an empty table and gives up. The name strings for the libraries we import appear
nowhere in the file.

Four more tags are needed: the module's own identity, one entry per imported module,
one per imported library, and the library attributes. Their values are not known, and
unlike the fourteen above they cannot be derived by arithmetic on a layout - they
encode an id and a name offset in the value itself rather than pointing at a table.
With the reference gone, the routes are a probe campaign (the infrastructure exists)
or another reference module.

