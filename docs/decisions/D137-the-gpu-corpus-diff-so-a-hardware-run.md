# D137 - The GPU corpus diff, so a hardware run ends in a gap list rather than a pile of records


Status: derived - `obscene-tool gpudiff` compares two corpora and is proven on identical and
mutated inputs.

The observation corpus is inert on its own. `gpudiff` is what makes it pay: it parses the
`gpu`/`gpuop`/`gpudev` records out of two runs and reports the lanes whose output bits
differ. The moment real RDNA2 records exist, one command turns them into "here is exactly
where the emulator disagrees, by operand" - which is the whole reason the GPU probe exists.

Three choices that matter:

- **Keyed by input, not lane.** A lane number is a position; the operands are the identity.
  Keying on `(kernel, inputs)` lines two corpora up regardless of lane order and names a
  divergence by the operands that caused it, which is what someone fixing the emulator needs.
- **Both provenances printed first.** Diffing llvmpipe against gfx1033 is the intended use;
  diffing a device against itself measures nondeterminism, not a gap. The tool states each
  side's `gpudev` and warns when they are the same device, so the two are never confused.
- **A lane on only one side is a divergence, not a silent drop.** It means the runs did not
  cover the same ground - a different kernel set or input vector - which is a real difference
  worth seeing.

Built ahead of hardware deliberately (the "do it now" call): it is verifiable now against
llvmpipe corpora, and having it ready is what makes the hardware day productive the instant
it happens rather than a fortnight later. Proven by construction - identical corpora agree,
a one-ULP mutation to `rcp(1/3)` is caught and named - and five unit tests lock the shapes.

