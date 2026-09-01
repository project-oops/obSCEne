# D123 - A golden GPU corpus and a regression check, so a change to a kernel's output is caught even where the reference stays silent


Status: derived - `reports/gpu-golden.txt` is a blessed snapshot of the build VM's llvmpipe,
`scripts/gpu-golden.sh` captures and checks it, and the check is gated in `verify.sh`. Proven
to pass clean on the matching device and to reject a one-bit mutation.

The reference oracle judges the exact operations device-independently, but it cannot judge the
transcendentals - it is a strong baseline, not a value. So nothing pinned `sin` or `rcp` to a
result, and a change that altered one silently - a shader edit, a dispatch bug, a widened input
vector - would pass every gate. The golden pins them: it records what this device actually
produced, and the check diffs a fresh run against it. Together the two answer the whole
question - "still exact where it must be" from the reference, "changed at all" from the golden.

Two choices that keep it from being the fragile dependency the GPU run is otherwise kept out of
the gate to avoid:

- **It skips on a different device, asserts only on a match.** The golden is one device's
  numbers; on another rasteriser the transcendentals differ legitimately, so the check compares
  the `gpudev` line first and skips when it does not match (and when no device produced records
  at all). It never fails on a difference that is not a regression - only on a real one, on the
  device the golden was blessed from. That is what let it join `verify.sh` without breaking the
  compile-only rule the GPU step otherwise keeps.
- **Re-blessing is a deliberate act.** `--capture` rewrites the golden; the check never does. A
  legitimate change to the numbers - a new kernel, a wider input vector, a mesa update - is a
  human running `--capture` and seeing the diff first, not the gate quietly adopting whatever
  the device said today. The mutation test proved the rejection works: corrupting one `rcp`
  lane to `0xdeadbeef` made the check name it and exit 1, and re-capturing restored it.

The golden is 1478 records - every kernel across the section's input vector - and lives in
`reports/` beside the other captured corpora, the largest of which dwarf it.

