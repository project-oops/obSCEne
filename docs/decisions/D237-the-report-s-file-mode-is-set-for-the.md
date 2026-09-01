# D237 - The report's file mode is set for the reader, and the reader is never the writer


*status: decided*

The sink created the report `0600`. The comment above it justified the choice against the wrong
alternative - "a mode of zero produces a file nothing can open afterwards" - having reasoned
entirely about the process doing the writing.

The process doing the writing is a title, inside a sandbox, running as whatever user the
platform runs titles as. **Everything that exists to retrieve the report is somebody else**: the
shell server, the file-transfer server, a later payload. All of them got `Permission denied` on
a file they could see in a directory listing.

That is the worst available failure shape. The file is plainly there, no tool on the machine can
open it, and it reads as a broken retrieval path - which is how it was read here, through an
FTP failure and a shell failure, before the mode was even looked at.

It is `0666` now. There is nothing to protect: the file is a list of which system functions
answered, on a machine the operator owns, truncated and rewritten on every run.

The lesson generalises past this file. A permission is a statement about a *reader*, and it was
chosen by thinking only about the writer. The same reasoning produced the same class of bug in
D232, D233 and D235 - an observation reported as the thing it was evidence for - and this is its
permissions-shaped cousin: a decision made from the only side that was in view.

