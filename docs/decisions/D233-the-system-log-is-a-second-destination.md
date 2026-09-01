# D233 - The system log is a second destination, not a candidate channel


D225 put `sceKernelDebugOutText` at the head of the output-channel selection. That was wrong in
a way the file it edited already warned about, two comments below the line that was changed.

The selection works by asking each candidate to move bytes and believing the first that says it
did. `sceKernelDebugOutText` returns a **status, not a count**, so it cannot answer that
question - and the implementation returned `len` regardless, with a comment calling that a
deliberate trade. It is not a trade. It is a channel that always claims success, and it is
selected on any loader that stub-resolves the symbol, after which the whole report goes behind
it.

Kyty does exactly that, and says so:

```text
Unresolved import stub called [15]: symbol=9JYNqN6jAKI[libkernel_v1][libkernel_v1.1][Func]
```

`9JYNqN6jAKI` is `sceKernelDebugOutText`. **Records went from some to none**, and back to 14
with the fix - the whole header block, followed by Kyty dying in its own `videoOut.cpp`.

The existing comment on the candidate order describes this precisely: *"one emulator implements
`write` by returning the byte count and discarding the bytes - a channel that reports success
and prints nothing, which is the one failure this selection cannot detect."* A guard was written
against a hazard, and then the hazard was reintroduced above the guard.

So the system log is written **unconditionally, before the selection**, exactly as the file sink
already is and for the same stated reason: the case it exists for is the one where none of the
channels works. Nothing is inferred from the call, so nothing can be inferred wrongly. It costs
a duplicate where both work, which is what the sink costs.

### Both of this session's off-hardware regressions were found the same way

D232 and this one. Neither is findable by `bin/obscene check` - the host build takes neither
path - and both would have reached the console, where a lost report and a false report are
respectively an hour and a wrong conclusion. Three loaders now cover the changes: shadPS4
(previous generation, linked), PS5PCEM (current generation, no module resolution), Kyty (stubs
unresolved imports and returns). Each found something the others did not.

