# D209 - obSCEne runs on a real PS5 and produces a report with hardware provenance - the first in the project's history


The culmination of the session. A payload sent through elfldr, bootstrapped entirely from what
the loader passes, emitted a full OBS| report over the loader socket. Nineteen records, every
one `hardware`.

### The chain, each link measured not assumed

1. **16 KB pages** (D206) - without them execution never reaches the entry.
2. **word 0 is `getpid`, not a resolver** - the parallel investigation into John Törnblom's
   `ps5-payloads` elfldr confirmed what the hardware said: `args[0] = getpid`, and elfldr
   applies only `R_X86_64_RELATIVE` relocations, resolving no imports at all. A payload is on
   its own.
3. **libkernel is callable but not readable** from the sandbox (`0xa0020328` on every read),
   so the export table cannot be walked at runtime.
4. **selfish read the vaddrs from the real file.** `/system/common/lib/libkernel_sys.sprx`,
   pulled over FTP, arrived as a plain decrypted ELF and `selfish_elf::dynamic::symbols` gave
   1,867 exports with their vaddrs. `getpid` sits at `0x5b0`, which is exactly `word0 -
   0x800000000` - so the runtime base is `word0 - 0x5b0`, and every other export is
   `base + vaddr`. No SDK, no guessed constant: the real file as oracle, read with the project's
   own format library. (D008 satisfied - the platform's own file drew every number.)

### The report

```
OBS|meta|1|hardware|first
OBS|measure|000-hw/tsc-frequency|sceKernelGetTscFrequency|hz|1596300187|hz
OBS|measure|000-hw/proc-time|sceKernelGetProcessTime|delta-us|2418|us
OBS|bytes|000-hw/sw-version|sceKernelGetSystemSwVersion|ver|0|0xc7c7c7c7c7c7c7c7   <- untouched
OBS|bytes|000-hw/sw-version|sceKernelGetSystemSwVersion|ver|8|0x302e3039302e3331   <- "13.090.0"
OBS|bytes|000-hw/sw-version|sceKernelGetSystemSwVersion|ver|16|0x0000000000003130  <- "01"
OBS|bytes|000-hw/sw-version|sceKernelGetSystemSwVersion|ver|32|0x1309000100000000  <- 0x13090001
OBS|end|sceKernelWrite
```

Three findings no emulator could supply:

- **TSC frequency 1,596,300,187 Hz.** A true value from silicon.
- **`sceKernelGetSystemSwVersion` leaves bytes 0-7 and 40-47 untouched** (still the `0xc7`
  poison), writing only 8-39. The poison-differential from D206 earned its place on first
  hardware contact: a zeroed buffer would have reported those as written-zero.
- **The version string reads `13.090.001`, binary `0x13090001`** - not the "12.40" this session
  had been labelling from the operator's statement. Reported as raw bytes, not interpreted, so
  the discrepancy is the operator's to reconcile. This is precisely why the report records
  hex, not meaning (OUTPUT.md).

### Every crash was survivable, and the last run did not crash the host

Across ~30 payloads the console stayed up. The final reports exit on `int3` (a clean SIGTRAP
the system handles), and a plain `ret` resumes the hijacked host process outright - elfldr
pushes the host's original rip as the return address. The loop's core bet held through an
entire session of driving real hardware.

Status: **hardware** - run on a console, 2026-08-27.

