# D184 - Hardware is registered, and its capabilities are measured on every use rather than stored


The emulators are paths on this machine: present or absent, and running one costs nothing. A
console is none of those things. It is an address on a network, what it can do depends on which
payloads happen to be loaded, and **that set does not survive a power cycle** - a jailbreak is
re-applied by hand, and what comes back depends on a text file somebody edited weeks ago.

So `obscene-tool hw` keeps an address and a name, and nothing else. Not "this console has a
shell", not "this one runs klogsrv" - those expire without notice, which is the stale-exclusion
mistake in a new place. `hw check` connects to all five ports every time it is asked, and the
first thing it ever did was correctly report a console mid-reboot: ping answering, every
service down.

### The table says what a port buys, not just that it is open

```
  DOWN elfldr    :9021  send a payload to the console and run it
  --   klogsrv   :3232  read the system's own log - why a payload died, not just that it did
```

A reader who has to remember that 3232 is the kernel log has been handed a worse tool than one
who is told. Required and optional are separated too, because they fail differently: without
`elfldr` nothing can run; without `klogsrv` things run and you cannot see why they stopped -
which is the failure that cost an afternoon here and looked like nothing at all.

Slow answers are printed. A port that takes 1500ms to refuse and one that refuses instantly
mean different things - a busy console or a poor network, against a payload simply not loaded -
and they are indistinguishable in a column of up and down.

### `hw send` refuses the wrong shape

Two bytes, checked before anything reaches the console:

```rust
if e_type == 0xFE10 || e_type == 0xFE18 {
    return Err(... "is a vendor-format module, which is for emulators" ...);
}
```

A vendor module and a plain ELF share their first four bytes, so `elfldr`'s own sanity check
passes either - and then it maps a module whose entry expects thirty-five thousand resolved
imports and goes down with it. That happened here and cost a reboot. The loader cannot tell
them apart cheaply; this can, so this does.

### What the socket is, and what it must never become

`elfldr` duplicates the connection onto the payload's descriptors 1 and 2, so a probe sent this
way streams its report back over the same socket. That is genuinely useful and `hw send` reads
it.

**It is not the report channel, and building on it would have been a design error.** A probe
installed as a package and launched from the home screen has no such socket; its descriptors go
to `/dev/deci_stdout` and its report goes wherever the sink puts it. A mechanism that only works
when the probe was launched one particular way is not a mechanism, it is a coincidence - and
obSCEne already solved this properly by trying candidate sinks and recording which answered.
`hw send` says so out loud when the socket is quiet, rather than reporting failure.

### And the gate that let this happen

Two sessions worked in this repository at once, both appended to this log, and one reused a
number the other had taken. `obscene-tool decisions` regenerated the index over the top without
a word - right total, both entries listed, one anchor silently lost. A gate that renders a
duplicate as though it were fine is precisely the failure this log keeps cataloguing, occurring
in the tool built to catch drift.

It now refuses, in both modes, with a negative test that fails if it ever goes quiet again.
Rewriting an index over a duplicated log is worse than not rewriting it, because the result
looks authoritative.

Status: **decided** - measured against a real console, including while it was rebooting.

