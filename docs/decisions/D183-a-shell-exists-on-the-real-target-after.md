# D183 - A shell exists on the real target after all, and the protocol survives it for a better reason


`docs/PROTOCOL.md` justified not building on a shell by saying the real target has no shell to
attach to. That was wrong. `shsrv` puts a telnet-like shell on port 2323 of a jailbroken
console, and it was in the same payload repository as `elfldr`, which this project already
depends on. It is now installed on the console here.

A justification resting on a missing capability stops being one the moment somebody ships it,
so the conclusion had to be re-argued rather than defended.

### The distinction that actually holds

**A shell operates the machine from outside a guest process; this interrogates the ABI from
inside one.**

`shsrv` can list a directory, start a process, read a file. It cannot answer *"what does
`sceKernelAllocateDirectMemory` return for these arguments, in a process loaded and relocated
the way a title is"*, because answering that means **being** a loaded guest, calling the
function, and reporting what came back. A shell answers it only by compiling and running a
program, and that program is this one.

The overlap is real but small: a few `sysinfo` facts - memory size, firmware version - that a
shell could also produce. Everything behavioural is in-process only.

### What it changes, which is the useful half

The boundary now says what **not** to build. Anything a shell already does - browsing the
filesystem, listing processes, launching payloads - is out of scope and stays out. Before
this, "obSCEne could grow a file browser" was an open question nobody had closed. It is closed.

### Three channels, and they answer different questions

Reading `elfldr` settled where a report goes on hardware, and it is not where this project
assumed:

```c
if(stdio > 0) {                              // stdio is the connection fd
    pt_dup2(pid, stdio, STDOUT_FILENO);
    pt_dup2(pid, stdio, STDERR_FILENO);
}
```

**The payload's stdout and stderr are the socket the ELF arrived on**, unless the sender asks
for `pipe=0`. So `sceKernelWrite(1, ...)` streams the report back to whoever sent it, live and
in order - the same channel shadPS4 provides, on real hardware, with announce-before-attempting
preserved on the wire. FTP retrieval is a fallback, not the mechanism.

| channel | answers |
|---|---|
| elfldr socket (fd 1/2) | what the probe said |
| `klogsrv` :3232 | what the *system* thought - `elfldr` alone has 54 klog sites |
| `/data/obscene-report.txt` | the durable copy, surviving a closed socket |

obSCEne writes to klog nowhere, and should not start: the report is the report, and mixing it
into the kernel log would put this project's output into a stream it does not control and
cannot parse back. The two stay separate and are read together. When `elfldr` died silently
earlier today, those 54 messages existed and nothing was listening - which is the whole
argument for the second channel.

Status: **decided** - the capability was measured on the console, not assumed.

