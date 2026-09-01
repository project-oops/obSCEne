# Real hardware, and the tooling it turned out to need


A jailbroken console appeared on the network at the point the emulator work had run its course.
Everything below came out of one evening of finding out what that actually changes.

### The first send was the wrong file, and it cost the loader

`elfldr` on port 9021 takes a payload. It was handed `obscene-module.elf` - the **vendor-format**
build, the shape emulators want because they emulate the system loader - and it died. Not
loudly: it accepted the connection, closed without a word, and stopped listening.

The information needed to avoid that was already in the repository, in the Makefile, beside the
rule:

> The plain-ELF build. **A homebrew ELF loader running on the console takes this**… it uses
> neither the vendor linker script nor mkmodule, because both exist solely to produce the shape
> this one must not have.

Read afterwards, as usual. `hw send` now checks `e_type` and refuses, because `elfldr`'s own
sanity check cannot - a vendor module and a plain ELF share their first four bytes.

### There is no way back from a dead elfldr, and that is structural

`pldmgr` has a web dashboard, it was still up on 8084, and it was completely inert. Its log:

```
GET /loadpayload:/data/pldmgr/payloads/elfldr/elfldr_v0.24.elf
[PLDMGR] Sending ELF to local loader: .../elfldr_v0.24.elf
[PLDMGR] !!! Connection to elfldr (port 9021) failed. Is it running?
```

`ps5_launcher.c` sends every payload to 9021. **Loading anything requires the thing that died,
including itself.** Every payload in that log fails identically. Only re-running the jailbreak
entry point recovers it.

The fix is redundancy, and it is configuration rather than code: `klogsrv` (3232) and `shsrv`
(2323) were both in the same repository as the loader, both absent from the chain, and are now
installed and in `autoload.txt` immediately after `elfldr` - so logging and a shell come up
before anything else can go wrong.

### Surprise: the report already has a channel nobody had noticed

```c
if(stdio > 0) {                       // stdio is the connection fd
    pt_dup2(pid, stdio, STDOUT_FILENO);
    pt_dup2(pid, stdio, STDERR_FILENO);
}
```

`elfldr` duplicates the socket onto the payload's descriptors. A probe sent that way reports
back over the same connection, live, in order - announce-before-attempting preserved on the
wire, on real hardware.

The instinct was to build the hardware workflow on it. That would have been wrong, and the
correction came from outside: a probe installed as a package and launched from the home screen
has no socket, and its descriptors go to `/dev/deci_stdout`. A channel that exists only when the
probe was launched one particular way is a coincidence, not a mechanism. obSCEne already
handles this correctly by trying candidate sinks and recording which answered; the socket is a
bonus that `hw send` reads and never depends on. (D183, D184)

### Surprise: the blind prober would have switched the console off

`910-bulk` calls every resolvable symbol with six zero arguments. Auditing it against a real
console rather than an emulator, the list it walks contains `sceSystemServiceRequestPowerOff`,
`sceLncUtilSystemShutdown`, `sceShellCoreUtilRequestShutdown` and ten more of that kind.

A blocklist cannot fix it: **50,344 of the 67,053 entries are unnamed NIDs**. Three quarters of
the sweep is unscreenable because nobody knows what those functions are.

Two things already prevented disaster, and only one was deliberate. `BULK` defaults to empty -
a convention. And census symbols are declared `extern const char`, so calling one does not
compile - a structural guarantee, and the reason a probe that imports
`sceSystemServiceRequestPowerOff` still cannot call it. `HARDWARE=1` now makes the combination a
compile error rather than a thing to remember. (D179)

### The forks are colliding

`D180`-`D182` appeared in the decision log without this session writing them, including one
about runtime resume losing measurements - the same fragility found here independently when a
convergence loop was killed mid-run and the accumulated skip list went with it. Two sessions,
one file, duplicate numbering, and `obscene-tool decisions` regenerated the index over the top
without noticing. It refuses now, with a negative test.

### State

`obscene-tool hw` registers a console and measures what it can answer, every time. Its first
real run correctly reported a console mid-reboot: ping answering, all five services down.

`obscene-min.elf` is built - 3,848 bytes, three imports, writes `/data/obscene-report.txt` and
exits rather than spinning. It is the transport test, and it has not run yet.

