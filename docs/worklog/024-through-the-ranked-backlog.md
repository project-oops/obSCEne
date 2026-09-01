# Through the ranked backlog


**§11 relational checks: five to ten.** Two opens of one path must give two descriptors,
closing twice must not succeed twice, a clock must never go backwards, two live mutexes
must not share a handle, a held default mutex must refuse a second lock.

`descriptors-distinct` has the worst failure mode in the section: an implementation
returning the same descriptor twice passes every check in `040-file` - each open succeeds,
each read works - and a caller that closes one has silently closed the other's.

Eight pass under shadPS4, two skip. The skips are informative: the file relations need
`OBS_CAP_FILE`, which is established by a check excluded for ending the process. **Excluding
a crashing check also withholds the capability it provides**, which is correct and has
reach beyond the checks named in the exclusion list.

**§12 consensus, built - and its first run showed the design was wrong.** 80 disagreements
of 126, almost all "this platform has the function and that one does not". A skip is not an
opinion; an implementation that lacks a function has said nothing about how it should
behave. Counting skips separately took it to 61 real behavioural differences (D072).

The backlog said this needed a second *emulator* reporting. It does not: the host build is
a real implementation of the POSIX and C library surface, so disagreement against it is
disagreement with something known to work.

**FreeBSD provenance, narrower than the task assumed.** A mass relabel would have been
wrong twice: `sceKernelClose` is not `close`, so POSIX settling `close` is an inference
about the renaming; and a pattern rule would have swept up `open-rejects-null`, which POSIX
explicitly leaves undefined. `OBS_FROM_DERIVED` says what is true, seven checks upgraded,
38 still assumed and most of them rightly (D074).

**The graphics interface, censused.** 87 names with libraries from public sources, every
one verified against our own hash - which rejected five, two of them placeholders with the
identifier embedded in the name (D075).

Then all 87 reported **present** under shadPS4, a previous-generation emulator with no
implementation of the interface at all. The clearest evidence yet that presence measures a
loader's stubbing policy: an entire console generation's graphics interface, reported
present by a census working exactly as designed. The `availability` field carries
`current` on those records, which is what a reader needs to discount them.

**The drift gate earned itself twice**, failing the build when checks were added and the
documentation had not followed. That is the whole point of it.

### State

778 records under shadPS4. 111 checks - 65 `spec`, 7 `derived`, 38 `assumed`, 1
`documented`, **0 `hardware`**. Census 399 symbols across 17 libraries: 296 shared, 87
current-generation, 17 previous.

