# D109 - The GPU is probed by executing shaders and reading result bits

**Status:** decided
**Date:** 2026-09-26

`160-gpu` dispatches compute kernels over known inputs and records the output bits per lane. Every
run emits a `gpudev` record naming the backend, device and device type, and the checks keep
`derived` provenance because whether a result is silicon is a run-time fact. Device selection takes
the first non-CPU device and falls back to a software rasteriser only when nothing else exists.

**Why:** what an instruction computes is what an emulator's shader translation must match and
cannot answer about itself. The same code on a software device and on silicon must never be
confused, so the device is the provenance.

**Rejected:** probing only the graphics API calls - says nothing about instruction results. Taking
the first enumerated device - can silently measure a CPU.
