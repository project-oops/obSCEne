# 2026-08-27 - obSCEne runs on real PS5 hardware, first report captured


The breakthrough session. From "the payload faults at address zero" to a full 19-record report
with hardware provenance, in one sitting.

The wall was never import resolution - it was three separate things, each found by measurement:
16 KB pages (execution never arrived without them), word 0 being `getpid` not a resolver
(elfldr resolves nothing, only R_X86_64_RELATIVE), and libkernel being callable-but-not-readable
from the sandbox. The escape was not a kernel exploit but selfish: pull the real
libkernel_sys.sprx over FTP (it arrives decrypted), read its 1,867 export vaddrs, and compute
every function as `(word0 - getpid_vaddr) + fn_vaddr`. Call it. Output.

First hardware values: TSC 1,596,300,187 Hz; sceKernelGetSystemSwVersion writes bytes 8-39 only
(0-7 and 40-47 stay poison), version string "13.090.001". The last one contradicts the assumed
"12.40" - reported as raw bytes, the operator's to reconcile.

Artifacts: reports/hardware/first-report.txt, reports/hardware/libs/libkernel_sys.exports.txt,
reports/hardware/probes/{boot,hwreport}.c, reports/hardware/klog-min-crash.txt. (D209)

**Next: the real suite.** A crt0 that resolves all of obSCEne's imports from getpid + a baked
per-firmware vaddr table (or via sceKernelDlsym, whose vaddr is now known), then runs the full
harness. And "rendering" - the video report - needs sceVideoOut resolved and a flip, a larger
surface than the socket report but the same bootstrap.

