# D246 - A placeholder in a symbol field is not a symbol


*status: decided*

The census writes `(census)` where a symbol name would go, because its checks are about a
library rather than about any one name. Those rows reached the import walk and reported
`linked`, since the address in the row is the census check's own guard.

Every library then appeared to bind two imports. `libScePad` showed five rows - three real and
unbound, two placeholders and "bound" - and that reads as *some symbols of this library bind
and some do not*, which is a per-symbol defect and a completely different search from a
per-library one. It cost a wrong hypothesis about NID hashing.

Told apart by the ABI's own rule rather than by a section id: a symbol name is a C identifier,
so a bracket or a space in that field means it is prose. A section-id test would silently stop
working the day a section was renamed.

With placeholders gone the finding is unambiguous: **`libkernel` and `libSceLibcInternal` bind
every import; every other declared library binds none.**

