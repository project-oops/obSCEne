# D074 - `derived` is a fifth provenance, because the FreeBSD upgrade was not the one the backlog described


Status: decided, narrower than the task it came from.

The backlog said: 44 assumed checks, many settled by a public FreeBSD man page, upgrade
them to `documented`. Doing that would have been wrong twice over.

**`sceKernelClose` is not `close`.** The name says it is a rename of a POSIX call and the
target kernel is FreeBSD-derived, so POSIX almost certainly settles what it does - but
"almost certainly" is an inference about the renaming, not a document about this function.
Marking it `spec` or `documented` claims a document that never mentions the symbol.

**And a rule would have swept up the wrong checks.** `040-file/open-rejects-null` looks
exactly like its neighbours: same library, same function, same shape. POSIX declines to
answer it - passing a null path is undefined behaviour, not a required failure - so the
analogue being exact does not make the expectation settled. It stays `assumed`, and it is
the reason each upgrade was named individually rather than matched by pattern.

`OBS_FROM_DERIVED` says what is actually true: the kernel derives from a documented system
and that system's specification settles this specific case. Wrong only if the vendor
changed a behaviour while keeping the name - a narrow, checkable claim, and a much better
position than a guess.

Seven checks upgraded: `write` returning its count and rejecting a bad descriptor, `close`
/ `read` / `lseek` rejecting a bad descriptor, `open` rejecting a missing path, and
`usleep` sleeping at least as long as asked.

**Thirty-eight remain assumed and most of them should.** Video, audio, input, the clocks,
module enumeration, the machine-kind query - these are vendor-specific with no public
document to appeal to, and calling them anything else would be the overclaiming this field
exists to prevent. The honest way to move them is a console, which is what the zero in the
`hardware` column is there to keep visible.

