# D032 - The report tries more than one way out, and says which one it used


Status: decided, on evidence.

`obs_write` called `sceKernelWrite` and nothing else. An emulator was found stubbing
it - returning zero and discarding every byte - so the probe ran its whole suite and
reported nothing at all, including the fact that `sceKernelWrite` is unimplemented.

A conformance probe that cannot report unless a particular function works has made its
own output a check, and a failing one takes everything with it.

Three channels are tried in order - `sceKernelWrite`, `write`, then `putchar` a
character at a time - and the first that moves a byte is kept. `puts` is deliberately
not among them: it appends a newline to every chunk, and the report is line-oriented,
so it would produce something that parses and is wrong. That is worse than nothing.

Which channel was used is appended to the `end` record. It is a result in its own
right: a run that fell back to one character at a time has said something about the
platform before any check has run.

