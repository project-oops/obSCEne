# D190 - The tool fetches the report off the console itself, rather than telling somebody to go and get it


`hw send` used to end by printing *check `/data/obscene-report.txt` over FTP*. That sentence
was there because the tool could not do it. It can now - the file service is part of the
crate taken in D189 - so `hw pull` fetches it and `hw ls` lists what is there.

A tool that can see a problem and cannot act on it has left the interesting half undone. This
is the smallest example of that available and it was sitting in the output of the command it
follows.

### Why it writes the file rather than printing for a redirect

`hw logs` and `hw sh` print, and composing with a shell redirect would have been consistent.
It is the wrong consistency here: **a redirect on Windows chooses the encoding itself**, and
this project's own environment writes a byte-order mark at the front of a redirected file.
`verify` would then be parsing a report whose first line begins with three bytes nobody put
there. The program writes the bytes it received, unaltered, and says where they went.

### Where the paths live

`/data/obscene-report.txt` and `/data` are defaults in this tool, not constants in the crate.
`pros_link` talks to any console and has no business knowing what this project writes.

### Proved against a second stand-in, and what it caught

A file service written in Python, advertising `10.0.0.1` as its data address while listening
on loopback. `hw ls` showed the sizes, kept the header line it could not parse and marked it,
and did not truncate a name containing a space; `hw pull` fetched the report; a 256-byte file
containing every byte value came back **byte for byte**, which is what binary mode is for; a
file that was not there produced a refusal quoting the server, and exit 1.

**What briefly looked like a bug in the client was Git Bash.** A `/data/notes.bin` argument
was rewritten into a Windows path before the tool ever saw it, so the console was asked for a
file that had never existed and correctly said so. `MSYS_NO_PATHCONV=1` is the fix and this
file already warned about it for multipass invocations - the same trap, a different tool, and
a reminder that the shell is a participant in any test that passes a path.

Status: **decided** - built and proved; the console path itself is still unmeasured against
real hardware.

