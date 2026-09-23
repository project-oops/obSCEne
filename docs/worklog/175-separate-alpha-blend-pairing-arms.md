# 175 - separate-alpha blend: positional-pairing arms (arm13/14/15)

**2026-09-23**

Took bus REQ-20260923T0100Z-9c31 (from oops-sdk/oops-gl). The `-6ba1` arm12a result was
read as "green binds to the alpha blend pipeline when SEPARATE_ALPHA_BLEND=1"; re-reading
the per-byte rows, it is positional - `R,B` take `COLOR_COMB_FCN`, `G,A` take
`ALPHA_COMB_FCN` - i.e. the colour block treating the target as two 32-bit words of two
channels each. Three new arms on `166-agc/compiled-ps` to settle it. All build on arm12a's
fixture (same 0.25f export, dst prefill 0x80808080, BLEND_BYPASS cleared).

- **arm13-swap-the-equations** (mode 16) - the discriminator. Swaps arm12a's two combines
  and keeps SEPARATE. Decoded the register against the field list rather than the filer's
  shorthand: `0x61010181` is `COLOR_COMB_FCN[7:5]=4` (DST_MINUS_SRC),
  `ALPHA_COMB_FCN[23:21]=0` (DST_PLUS_SRC); swapping them is **`0x61810101`** (COLOR add,
  ALPHA reverse-subtract), *not* `0x41010181` (which is arm12a with SEPARATE merely re-set
  and would repeat it). Positional -> pixel inverts to `0x40c040c0`; green-follows-alpha ->
  `0xc040c040`.
- **arm14-what-the-block-was-told** (mode 17) - reports the export/surface registers as the
  fixture programs them: `spi-shader-col-format=0x9` (32_ABGR), `cb-color0-info=0x88a8`,
  target/shader mask, blend control, and `sx-ps-downconvert-control=0xff` (0x1d4).
  `SX_PS_DOWNCONVERT` (0x1d5) and `SX_MRT0_BLEND_OPT` (0x1d8) are **not** emitted by
  agc_emit_ngg_context, so they report `not-programmed` - any pairing is not from a
  fixture-set downconvert. A COPY_DATA read-back of the inherited SX_* state is the deeper
  follow-up if the pixel arms implicate the export format.
- **arm15** (modes 18-21) - arm12a's blend with `CB_TARGET_MASK` restricted to one channel,
  run four times (`arm15-mask-{r,g,b,a}`); `cb_shader_mask` stays 0xf. If the pairing
  survives masking it is real; if masking changes which combine a channel gets, it is a
  packing artefact on the way out.

Surgical, additive edits to `agc.c` (another session is actively editing that file):
prefill and shader-export conditions widened to modes 14-21, blend-control switch extended,
a target-mask override for 18-21, the arm14 register rows, and the dispatcher invocations.
The arms report the per-byte rows the acceptance asked for. Host build clean under
`-Werror -Wconversion`; the arms are hardware-only (on host the check bails at arm10a before
reaching them). Staged to the console; 9c31 stays OPEN pending the run.
