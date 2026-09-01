# 5. The positive-to-negative ratio is still wrong


`035-libc` and `037-math` moved it substantially: 27 checks that ask whether something
*works* rather than whether it rejects nonsense. Every other section is still mostly
negative, and an implementation that fails everything passes all of those (D007).

Every confirmed signature is an opportunity to convert a negative check into a
positive one. That conversion is worth more than adding new negative checks.

**`018-relational` is the other route, and it is not gated on signatures at all.** A relation
compares two results to each other rather than to an expected value, so it needs no oracle
and no struct layout - which is why the section could be built while §2 was still blocked.
It now carries seventeen checks, seven of them added on 2026-08-24: three asking whether one
object's state is visible on another, two needing a second thread (mutual exclusion and
thread identity cannot be measured from one), one on overlapping allocations and one on file
position arithmetic. Every one is `spec` rather than `assumed`, because "a counting semaphore
counts" and "two live objects do not share one handle" are not this project's opinions.

What remains here is genuinely blocked on the same thing §2 is: the relations available
without a layout are largely spent, and the next ones (a directory listing that matches what
was written into it, a mapping that reads back what another mapping wrote) need structures
this program will not guess at.

