# D299 - Reproducible Process Parameter Audit in 048-selfaudit

**Status**: [decided]
**Date**: 2026-09-02
**Context**: Canonizing runtime inspection of `PT_SCE_PROCPARAM` and target SDK versions in obSCEne's on-console test suite.

---

## Context

During diagnosis of `SCE_KERNEL_ERROR_ESDKVERSION` and libc startup faults, we inspected `PT_SCE_PROCPARAM` headers through ad-hoc host scripts and manual file dumps. In keeping with the project rule that *all measurements and probes must be reproducible as part of obSCEne rather than left in scratch files*, the process parameter inspection was canonized into the probe section `048-selfaudit`.

## Decision

1. **Extend `048-selfaudit/container-structure`**:
   - Locate the ELF header following the container segment entries (`0x20 + segment_count * 0x20` for SELF, or `0` for raw ELF).
   - Iterate through ELF program headers to find `PT_SCE_PROCPARAM` (`p_type == 0x61000001`).
   - Read the 0x60-byte parameter block, verifying the `"ORBI"` magic (`0x4942524F`).
   - Extract and report:
     - `structure/procparam/ps4_sdk`: PS4 target SDK version at offset `0x10`.
     - `structure/procparam/ppr_sdk`: Prospero target SDK version at offset `0x14`.
     - `structure/procparam/libc_param`: Libc parameter pointer at offset `0x38`.
     - `structure/procparam/mem_param`: Memory parameter pointer at offset `0x40`.
     - `structure/procparam/third_param`: Third parameter pointer at offset `0x48`.

2. **Self-Contained On-Console Execution**:
   - Relies strictly on standard platform calls (`sceKernelOpen`, `sceKernelLseek`, `sceKernelRead`, `sceKernelClose`).
   - Operates on both real retail containers discovered in `/system/vsh/app` or `/user/app` and the running binary at `/app0/eboot.bin`.

## Consequences

* Provides reproducible, automated reporting of target SDK and libc parameters directly in `host-report.txt` and hardware run logs.
* Eliminates the need for external ad-hoc python scripts to verify container and procparam stamping.

