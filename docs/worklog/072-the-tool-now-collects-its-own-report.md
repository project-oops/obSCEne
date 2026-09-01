# The tool now collects its own report


`hw ls` and `hw pull`, over the file service the crate brought with it. `hw send` no longer
ends by telling somebody to go and fetch the report; it names the command that does. (D190)

Proved against a Python file service that advertises `10.0.0.1` for its data connections
while listening on loopback: a listing with sizes and a marked unparseable line, a report
fetched, a 256-byte file containing every byte value returned byte for byte, and a missing
file refused with the server's own words and exit 1.

### The surprise was the shell, not the code

`hw pull /data/notes.bin` failed with *550 no such file* while a hand-typed `RETR
/data/notes.bin` against the same server succeeded. Git Bash had rewritten the argument into
a Windows path before the tool saw it, so the console was asked for something that had never
existed and answered correctly. `MSYS_NO_PATHCONV=1`.

This file already warns about that for multipass. It is worth writing down a second time,
because the failure arrived wearing the clothes of a client bug and the client was right.

