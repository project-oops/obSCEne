# GPU corpus diff (gpudiff)


Brief item 4's actionable half (D137). `obscene-tool gpudiff golden fresh` parses the
gpu/gpuop/gpudev records from two corpora and prints, per (kernel, operands), where the
output bits differ - exit non-zero on any divergence, both device provenances up front, a
lane on only one side flagged rather than dropped. Built ahead of hardware on purpose: it is
the machinery that turns a Deck/console run straight into a gap list. Proven on llvmpipe -
identical corpora agree over 1187 lanes, a one-ULP mutation to rcp(1/3) is caught and named -
plus five unit tests. verify: ok.

### Judgement recorded
Building unverifiable-until-hardware code (the console Gnm submission guts) is the one place
"do it ahead" does not hold - single-source, D008 territory - so that stays a refusing seam.
Everything in the observation-and-diff path is verifiable now and was built out.

