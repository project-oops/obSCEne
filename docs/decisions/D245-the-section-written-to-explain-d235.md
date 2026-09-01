# D245 - The section written to explain D235 committed D235


*status: decided*

`061-imports` asks the resolver whether the platform has a symbol, and asking means loading the
symbol's library. Ten libraries end the process when loaded (D236). The section opened them.

The first full run stops mid-walk, one library after `libScePad`; the next library in registry
order is `libSceVideoRecording`. Twenty-eight sections' worth of measurement lost, to a library
whose behaviour was already written down in this repository.

`EXCLUDE` could not have caught it. That mechanism matches *check ids*, and this is one check
opening many libraries - so the thing that makes the announce-before-attempt property work for
every other check does not apply here. Two changes make it apply:

- The library is named with a `module` record **before** it is opened, so a death names it.
- Libraries in `data/hardware/crashers.txt` are not opened at all. The Makefile derives the
  names from the record's check ids and passes them as `OBS_UNSAFE_LIBRARIES`.

**A library that was not asked about emits no record.** Writing `unresolvable` for it would say
"the platform does not have this symbol" where the truth is "we did not ask", which is the
precise conflation this whole section exists to undo. The count goes in the verdict, so the
silence is accounted for rather than merely quiet.

Worth recording plainly: this was written immediately after reading D235, by someone who had
just described the mistake, and it was made anyway. The rule "do not load a library as a side
effect" is not hard to state and it is apparently easy to walk past when loading is the point
of the check rather than an accident of it.

