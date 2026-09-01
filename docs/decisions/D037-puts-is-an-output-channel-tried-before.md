# D037 - `puts` is an output channel, tried before `write`


Status: decided, on evidence read from a loader.

The three original channels were chosen on the assumption that a channel either works
or reports failure. One emulator disproves it: its `write(1, ...)` returns the byte
count and discards the bytes. A channel that reports success and prints nothing is the
one failure the selection cannot detect, and it would have silently swallowed every
report on that platform.

The same emulator implements `puts` properly, routing it to its guest-output stream, so
`puts` is tried first of the two. It is safe despite appending a newline because every
record is written in exactly one call and already ends in one - `line_end` is the only
producer - so the channel drops ours and lets `puts` supply it. It refuses any chunk not
ending in a newline rather than risk splitting a record, which would produce output that
parses and is wrong.

Also learned there, and worth knowing beyond this: that loader resolves an unresolved
weak import to a **valid allocated page**, not to null. `obs_address_is_callable`
rejects null and the first page and would accept this, so a probe cannot tell it from a
real function. The census control - a name chosen because nothing can define it -
already catches the resulting inflation and invalidates the section, which is the
argument for having had it.

