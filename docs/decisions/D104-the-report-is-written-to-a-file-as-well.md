# D104 - The report is written to a file as well, not instead. And the backend is chosen by which file is compiled


Status: derived - the sink is exercised by `make host` and writes a complete report.

### A second destination, not a fifth channel

`runtime.c` picks one of four text channels and sticks with it; the rest are never tried
again. The file is not among them. It receives every record **in addition** to whichever
channel was chosen, because the two answer opposite questions: a text channel answers "can
this system talk to a terminal I am watching", and a file answers "can this system leave
something behind". On a console the first is often no and the second is what matters once
the run is over.

Written before the channel loop rather than after, so a text channel that hangs or ends the
process cannot cost the record on disk. Same reasoning as announce-before-attempting: the
durable write goes ahead of the risky one.

**Never buffered.** A record reaches the file at the moment it is produced. Buffering would
be faster and would destroy the property the whole program rests on - a report that stops
mid-record names the call that ended the run. A crash discards a buffer, and the crash is
the finding.

### The path is discovered and the discovery is reported

Four candidates, tried in order. Which directory an unsigned module may write to differs by
platform and by how it was launched, so the list is a set of guesses - and the `sink` record
naming the winner turns guessing into measurement. On the host it falls through three
absent paths to a bare filename and says so.

A null path still emits `sink|none` rather than no record. A missing record is
indistinguishable from an older build that never had a sink; a word is a fact about this
run.

### Source lists, not preprocessor branches

`sink.c` holds the path list, the write loop and the close-on-failure rule. The three
platform calls live in `sink_target.c` and `sink_host.c`, and the Makefile compiles exactly
one.

The alternative was `#if defined(OBSCENE_HOST_BUILD)` inside `sink.c`. That puts two
targets' code in one file where only one is ever compiled, so the untaken branch is never
parsed and rots unnoticed. A list in the Makefile says which backend a target gets, in one
place, visibly - and adding a target becomes a file plus a line rather than another
condition threaded through code that already works.

It also resolved a genuine conflict rather than compromising on it. The host stubs report
`sceKernelOpen` as not implemented, which is the **correct** answer for the `040-file`
checks that measure it and the wrong one for a sink that has to actually write. Two backends
let the checks keep seeing a stub while the sink gets tested, instead of bending one to suit
the other.

This is the structure the socket layer will use for the same reason.

