# The HUD readout is now in the report, not only on screen (`sysinfo` records)


Everything the status bar drew - memory, VRAM, generation, the socket address - lived only on
the drawn screen. A driver reading the report over the socket saw `net|listening` come up and
not a byte of the rest. Added one `sysinfo` record per field, carrying `field | state | value`,
so the report mirrors the HUD.

The record keeps the **state tier**, which is the whole point and the reason `measure` was the
wrong vehicle (it is numeric-only and carries no tier): `known` / `unconfirmed` / `absent` all
render `unknown` as a value, and they mean different things - an emulator gap versus our own
unfinished wiring versus a real reading. Collapsing them would throw away the one distinction
worth diffing across platforms. `generation` follows the section's rule: a console number only
when exactly one graphics driver resolves, `unknown` otherwise (see D121).

New record kind, so nothing bumps the format version - a parser keys on the second field and
ignores kinds it does not know (`docs/OUTPUT.md`). Emitted with the report header (after `sink`,
`src/harness.c`) and again from a serving build the moment it is listening (`src/start.c`), so a
driver that connects before requesting a run can still read the machine's own account of itself.
`obs_report_sysinfo` in `report.c`, the emit loop in `sysinfo.c`, contract in `OUTPUT.md`.

Verified both builds: host emits the block after `sink` (`generation|absent`, `vram|known|0M`
from the stub); shadPS4 emits `memory|known|441M`, `vram|known|4608M`, `generation|unconfirmed|
unknown`, matching the drawn HUD exactly. `make check` green (117); shadPS4 run clean, no crash.

### Surprise worth keeping

- **The state tier is the payload, not the value.** The instinct was to emit the numbers. The
  numbers a stub-happy loader gives are the least trustworthy part; what a consumer actually
  needs is *which kind of unknown* a blank field is - and that is exactly what a bare value, or
  a `measure` record, cannot say.

---

More execution breadth (D124). Three kernels: `bfe`/`bfi` (bitfieldExtract/Insert with a fixed
[8,16) field, so they fit the unary/arity-2 sweep and match how RDNA encodes v_bfe_u32/v_bfi_b32),
and `ftz` (x*0.5, a flush-to-zero probe - the complement of `double`, matches the IEEE denormal
on llvmpipe, diverges only where a device flushes). Plus `obs_gpu_bits`, a bit-pattern input
vector the bit kernels now use (bitcount/findmsb/findlsb/bitreverse/bfe) via a name predicate in
sweep_kernel - no schema change. bitcount now sees 0xffffffff (32) and 0xaaaaaaaa (16), patterns
the float vector never gave. 45 kernels; all match the reference bit-for-bit. verify: ok.

### Notes
- Golden re-bless done the D123 way: inspected the diff first (233 changes, all accounted for -
  3 new kernels + 4 bit kernels' input switch, no float kernel moved), then --capture.
- Two stale-binary traps hit and fixed: gpu-golden.sh skipped its rebuild when the GPU binary
  merely existed (blessed a stale binary; now always builds, make is incremental), and the tool
  debug binary was stale for gpuref until an explicit `cargo build` (clippy/test don't produce
  it) - the new kernels showed all-diverged against the reference until then.
- Deferred: pack/unpack conversions need value-range operands the edge set can't supply, so a
  per-kernel input scheme is the next step - named in D124 so it is not mistaken for done.

Handover doc (`docs/HANDOVER-ORBISTOUN-NET.md`) updated for the consumer: a `sysinfo` section
(field/state/value, read the state tier, observations never machine identity, the `generation`
rule), and the end-to-end example and checklist extended for it. Correcting a doc-drift in the
same pass: the doc still said `report` returns only `ack`+`done` and the records reach the file
alone - stale since the report tee landed. `report` streams the whole run down the socket
between `ack` and `done` now (net.c installs `obs_net_tee` around `obs_run_all`), additive with
the file sink, so the sysinfo header arrives on the wire. The section, the session example and
checklist item 8 now say so.

