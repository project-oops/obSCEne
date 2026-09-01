# D035 - The entry point ends the process. It does not return


Status: decided, on evidence - reversing an earlier decision.

`obscene_start` returned, deliberately: a loader calls the entry as a function, so a
probe should hand control back rather than terminate, and stay runnable twice in one
session.

That is not what happens. The module is an executable and its entry is where the
process *starts*. Returning pops whatever the loader left on the stack:

    Unhandled Exception code 0xc0000005 at 0x1

The expensive part is where that lands. It arrives after a complete run, immediately
after the last check's platform call, so it reads as that check crashing the process.
Time went into the last check and into the census after it before it became clear that
every section had already run and the fault was in leaving.

It now calls `exit`, and spins if there is none. Both are honest and they are
distinguishable - exited means the run ended on purpose; hung means the platform has no
`exit`, and the report was complete either way. Returning is the one outcome that
cannot be told apart from having crashed.

