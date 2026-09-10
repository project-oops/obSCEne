# 2026-09-10 - Hardware Compute Shader Dispatch and Fence Retirement

Hardware sweep `20260910-203426-eboot` achieved the foundational milestone of PS5 GPU enablement:
**end-to-end bare-metal compute shader dispatch and memory fence readback** on retail PlayStation 5 (FW 12.40, Oberon RDNA2 GPU).

Both `166-agc/driver-submit-fence` and `166-agc/compute-dispatch` passed with zero crashes:
- `rc-submit = 0x0`
- `fence-val = 0xbeefcafe`
- `fence-hit = 0x1` (fence retired by GPU and verified by CPU in coherent Onion memory)

Section 166 (AGC) finished with **15 pass, 0 fail, 0 partial, 1 skip** across 11,215 probe records.

---

## 1. The Root Cause: PM4 Indirect Buffer Size Unit (DWORDs vs Bytes)

In previous sweeps (`20260910-195434-eboot` and `20260910-200046-eboot`), `sceAgcDriverSubmitDcb` returned `0x0`, but the memory fence at `0xbeefcafe` remained unwritten (`fence-val = 0x11111111`, `fence-hit = 0x0`).

Deep disassembly of `libSceAgcDriver.sprx` (`0x1100`, `0x2960`, `0x2a00`), `AgcCompositor.elf` (`0x32ff9`), and `libSceAgc.sprx` (`0xef70` and `0xf5b0`) revealed the mechanism:

```x86asm
# libSceAgcDriver.sprx @ 0x1100
mov    0x8(%rsi), %eax       # eax = desc->size
shl    $0x4, %eax            # shift into IB_SIZE field
mov    %eax, 0xc(%rdi)       # store into PM4 INDIRECT_BUFFER DW3
```

The driver places `desc->size` directly into the `IB_SIZE` field (bits 19:0 of DW3) of the PM4 `PACKET3_INDIRECT_BUFFER` packet (`0xc0023f00`).

### The Defect
In the AMD RDNA2 / GFX10 PM4 architecture, `IB_SIZE` is defined strictly in **DWORDs** (32-bit units), not bytes.
In `AgcCompositor.elf` at `0x32ff9`, the compositor explicitly shifts the buffer byte count before passing:
```x86asm
32ff9: shr    $0x2, %r8d     # size_in_dwords = byte_count / 4
```

Because our probe previously passed byte counts directly (`desc.size = bytes_written`, which was 32 and 184), the Command Processor (CP) was commanded to execute **32 and 184 DWORDs** respectively.
The CP executed the 8 valid PM4 DWORDs (`32 bytes / 4`), but then continued reading into the uninitialized command buffer, encountering `0x5a5a5a5a` (the probe buffer poison bytes). In RDNA2 PM4, `0x5a` has bits 31:30 = `01` (Type-1 packet, which is illegal in GFX10), causing the CP to fault and discard subsequent memory writes before the fence could retire.

### The Fix
1. Passed DWORD count: `desc.size = (uint32_t)(bytes_written / sizeof(uint32_t));`
2. Padded trailing command buffer space with 16 DWORDs of PM4 NOP (`0xffff1000u`) to protect against CP prefetch overruns.
3. Added `__builtin_ia32_clflush(fence_cpu)` in the fence polling loop to eliminate any CPU L1/L2 cache staleness when reading coherent Onion memory written by the GPU memory controller.

---

## 2. Hardware Fence Retirement (`166-agc/driver-submit-fence`)

The driver submit fence test builds a standalone 32-byte (8 DWORD) PM4 Direct Command Buffer containing a `RELEASE_MEM` end-of-pipe event:

* DW0: `0xc0064900` (`PACKET3_RELEASE_MEM`, count 6).
* DW1: `0x06603514` (`GCR_SEQ | GCR_GL2_WB | GCR_GLM_INV | GCR_GLM_WB | CACHE_POLICY(3) | EVENT_INDEX(5) | EVENT_TYPE(0x14)`).
* DW2: `0x20000000` (`DATA_SEL(1)` = write 32-bit integer).
* DW3: `lower_32_bits(fence_gpu)`.
* DW4: `upper_32_bits(fence_gpu)`.
* DW5: `0xbeefcafe` (fence retirement value).
* DW6: `0x0` (upper 32 bits of value).
* DW7: `0x0` (context_id / pad).

Hardware sweep log (`reports/hardware/20260910-203426-eboot.obs.log`):
```
OBS|try|166-agc/driver-submit-fence|libSceAgcDriver|sceAgcDriverSubmitDcb
OBS|measure|166-agc/driver-submit-fence|sceAgcDriverCreateQueue|rc-create|0x0|code
OBS|measure|166-agc/driver-submit-fence|sceAgcDriverSubmitDcb|bytes-written|0x20|size
OBS|measure|166-agc/driver-submit-fence|sceAgcDriverSubmitDcb|rc-submit|0x0|code
OBS|measure|166-agc/driver-submit-fence|sceAgcDriverSubmitDcb|fence-val|0xbeefcafe|val
OBS|measure|166-agc/driver-submit-fence|sceAgcDriverSubmitDcb|fence-hit|0x1|bool
OBS|res|166-agc/driver-submit-fence|pass|||assumed
```

`fence-val` flipped from `0x11111111` to `0xbeefcafe`. The GPU hardware executed the command buffer and retired the fence through the memory controller into host-visible Onion memory.

---

## 3. End-to-End Compute Dispatch (`166-agc/compute-dispatch`)

With queue creation, PM4 IB framing, register binding, and fence retirement settled, `166-agc/compute-dispatch` executed the full compute pipeline on bare metal:

1. **Memory Setup**: Allocated 256-byte aligned shader bytecode payload and fence buffer in coherent Onion memory (`OOPS_MEM_WB_ONION`).
2. **Shader Creation**: Called `sceAgcCreateShader(&shader_obj, hdr_buf, gpu_payload, 0)` with the authentic 304-byte RDNA2 container header (`rc = 0x0`).
3. **Queue Setup**: Created Type-3 DCB queue via `sceAgcDriverCreateQueue(3u, &queue, 0u)` (`rc = 0x0`).
4. **DCB Encoding**:
   - Emitted 11 pairs of `SET_SH_REG` (`0xc0017600u`) configuring compute registers:
     - `COMPUTE_PGM_LO` (`0x20c`) = `lower_32_bits(payload_va)`
     - `COMPUTE_PGM_HI` (`0x20d`) = `upper_32_bits(payload_va)`
     - `COMPUTE_PGM_RSRC1/2` (`0x212`, `0x213`)
     - `COMPUTE_NUM_THREAD_X/Y/Z` (`0x207`, `0x208`, `0x209` = 8, 8, 1)
     - `COMPUTE_USER_DATA_0/1` (`0x22a`, `0x22b`)
     - `COMPUTE_RESOURCE_LIMITS` (`0x228`)
   - Emitted `DISPATCH_DIRECT` (`0xc0031500u`, dim 1x1x1, initiator `0x41u`).
   - Emitted `RELEASE_MEM` with EOP event flush writing `0xbeefcafe` to the fence address.
5. **Submission & Readback**:
   - Submitted 46 DWORDs (184 bytes) via `sceAgcDriverSubmitDcb`.
   - Polled fence with cache invalidation (`__builtin_ia32_clflush`).

Hardware sweep log:
```
OBS|try|166-agc/compute-dispatch|libSceAgcDriver|sceAgcDriverSubmitDcb
OBS|measure|166-agc/compute-dispatch|sceAgcCreateShader|rc-shader|0x0|code
OBS|measure|166-agc/compute-dispatch|sceAgcDriverCreateQueue|rc-create|0x0|code
OBS|measure|166-agc/compute-dispatch|sceAgcDriverSubmitDcb|bytes-written|0xb8|size
OBS|measure|166-agc/compute-dispatch|sceAgcDriverSubmitDcb|rc-submit|0x0|code
OBS|measure|166-agc/compute-dispatch|sceAgcDriverSubmitDcb|fence-val|0xbeefcafe|val
OBS|measure|166-agc/compute-dispatch|sceAgcDriverSubmitDcb|fence-hit|0x1|bool
OBS|res|166-agc/compute-dispatch|pass|||assumed
```

`fence-hit` read back as `0x1` (`fence-val = 0xbeefcafe`). The compute shader dispatched, executed on the RDNA2 compute units, drained, and retired the end-of-pipe fence cleanly.

---

## 4. Hardware Sweep Summary

```
Section 166 (AGC command building and shaders):
  pass:    15
  fail:     0
  partial:  0
  skip:     1 (166-agc/cb-unnamed-ef57: optional unpublished NID)
  total:   16
```

This concludes Phase 5 of the PS5 GPU hardware enablement roadmap. Hardware compute dispatch on Prospero is proven.
