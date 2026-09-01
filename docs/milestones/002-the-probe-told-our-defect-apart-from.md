# 2026-08-30 - the probe told our defect apart from the platform's


```text
OBS|import|libScePad|scePadOpen|unlinked|resolvable
```

**Evidence:** [`data/hardware/ps5-imports.txt`](../../data/hardware/ps5-imports.txt).
**Build:** `BUILD_ID=global1`, `GEN=4`, `PROC_SDK=0`.

Twenty-four checks had been skipping with "the loader did not resolve this symbol", recorded as
gaps in the console. Fourteen of them were ours: **every import was `STB_WEAK`, undefined**,
which a loader reads as permission not to bother - so it bound the two libraries already
resident in the process and left every library it would have had to *load* mapped but unbound.

Rebinding imports `STB_GLOBAL` in `selfish` took the repairable count from fourteen to zero.
`100-input`, `090-audio` and `165-gnm` pass outright now; `080-video`, `070-user` and
`130-layout` run for the first time.

**What makes this a first rather than a fix:** before it, a null symbol had one reading and it
was the wrong one. `061-imports` and the `import` record made the two causes separable, and
three previously-recorded "hardware findings" turned out to be about this program. (D240, D248)

---

